/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "Configuration.hpp"
#include "EquivalenceCheckingManager.hpp"
#include "EquivalenceCriterion.hpp"
#include "algorithms/BernsteinVazirani.hpp"
#include "algorithms/QFT.hpp"
#include "algorithms/QPE.hpp"
#include "dd/DDDefinitions.hpp"
#include "dd/Package.hpp"
#include "ir/Definitions.hpp"
#include "ir/QuantumComputation.hpp"

#include <bitset>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

class DynamicCircuitTestExactQPE : public testing::TestWithParam<qc::Qubit> {
protected:
  qc::Qubit precision{};
  qc::fp theta{};
  std::size_t expectedResult{};
  std::string expectedResultRepresentation;
  qc::QuantumComputation qpe;
  qc::QuantumComputation iqpe;
  std::unique_ptr<dd::Package> dd;
  std::ofstream ofs;

  ec::Configuration config{};

  void TearDown() override {}
  void SetUp() override {
    precision = GetParam();

    dd = std::make_unique<dd::Package>(precision + 1);

    qpe = qc::createQPE(precision);

    // extract lambda from QPE global phase
    const auto lambda = qpe.getGlobalPhase();

    iqpe = qc::createIterativeQPE(lambda, precision);

    std::cout << "Estimating lambda = " << lambda << "π up to " << precision
              << "-bit precision.\n";

    theta = lambda / 2;

    std::cout << "Expected theta=" << theta << "\n";
    std::bitset<64> binaryExpansion{};
    dd::fp expansion = theta * 2;
    std::size_t index = 0;
    while (std::abs(expansion) > 1e-8) {
      if (expansion >= 1.) {
        binaryExpansion.set(index);
        expansion -= 1.0;
      }
      index++;
      expansion *= 2;
    }

    expectedResult = 0ULL;
    for (std::size_t i = 0; i < precision; ++i) {
      if (binaryExpansion.test(i)) {
        expectedResult |= (1ULL << (precision - 1 - i));
      }
    }
    std::stringstream ss{};
    for (auto i = static_cast<int>(precision - 1); i >= 0; --i) {
      if ((expectedResult & (1ULL << i)) != 0) {
        ss << 1;
      } else {
        ss << 0;
      }
    }
    expectedResultRepresentation = ss.str();

    std::cout << "Theta is exactly representable using " << precision
              << " bits.\n";
    std::cout << "The expected output state is |"
              << expectedResultRepresentation << ">.\n";

    config.optimizations.transformDynamicCircuit = true;
    config.optimizations.backpropagateOutputPermutation = true;
  }
};

INSTANTIATE_TEST_SUITE_P(
    Eval, DynamicCircuitTestExactQPE, testing::Range<qc::Qubit>(1U, 64U, 5U),
    [](const testing::TestParamInfo<DynamicCircuitTestExactQPE::ParamType>&
           inf) {
      const auto nqubits = inf.param;
      std::stringstream ss{};
      ss << nqubits;
      if (nqubits == 1) {
        ss << "_qubit";
      } else {
        ss << "_qubits";
      }
      return ss.str();
    });

TEST_P(DynamicCircuitTestExactQPE, UnitaryEquivalence) {
  ec::EquivalenceCheckingManager ecm(qpe, iqpe, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

class DynamicCircuitTestInexactQPE : public testing::TestWithParam<qc::Qubit> {
protected:
  qc::Qubit precision{};
  dd::fp theta{};
  std::size_t expectedResult{};
  std::string expectedResultRepresentation;
  std::size_t secondExpectedResult{};
  std::string secondExpectedResultRepresentation;
  qc::QuantumComputation qpe;
  qc::QuantumComputation iqpe;
  std::unique_ptr<dd::Package> dd;
  std::ofstream ofs;

  ec::Configuration config{};

  void TearDown() override {}
  void SetUp() override {
    precision = GetParam();

    dd = std::make_unique<dd::Package>(precision + 1);

    qpe = qc::createQPE(precision);

    // extract lambda from QPE global phase
    const auto lambda = qpe.getGlobalPhase();

    iqpe = qc::createIterativeQPE(lambda, precision);

    std::cout << "Estimating lambda = " << lambda << "π up to " << precision
              << "-bit precision.\n";

    theta = lambda / 2;

    std::cout << "Expected theta=" << theta << "\n";
    std::bitset<64> binaryExpansion{};
    dd::fp expansion = theta * 2;
    std::size_t index = 0;
    while (std::abs(expansion) > 1e-8) {
      if (expansion >= 1.) {
        binaryExpansion.set(index);
        expansion -= 1.0;
      }
      index++;
      expansion *= 2;
    }

    expectedResult = 0ULL;
    for (std::size_t i = 0; i < precision; ++i) {
      if (binaryExpansion.test(i)) {
        expectedResult |= (1ULL << (precision - 1 - i));
      }
    }
    std::stringstream ss{};
    for (auto i = precision; i > 0; --i) {
      if ((expectedResult & (1ULL << (i - 1))) != 0) {
        ss << 1;
      } else {
        ss << 0;
      }
    }
    expectedResultRepresentation = ss.str();

    secondExpectedResult = expectedResult + 1;
    ss.str("");
    for (auto i = precision; i > 0; --i) {
      if ((secondExpectedResult & (1ULL << (i - 1))) != 0) {
        ss << 1;
      } else {
        ss << 0;
      }
    }
    secondExpectedResultRepresentation = ss.str();

    std::cout << "Theta is not exactly representable using " << precision
              << " bits.\n";
    std::cout << "Most probable output states are |"
              << expectedResultRepresentation << "> and |"
              << secondExpectedResultRepresentation << ">.\n";

    config.optimizations.transformDynamicCircuit = true;
    config.optimizations.backpropagateOutputPermutation = true;
  }
};

INSTANTIATE_TEST_SUITE_P(
    Eval, DynamicCircuitTestInexactQPE, testing::Range<qc::Qubit>(1U, 15U, 3U),
    [](const testing::TestParamInfo<DynamicCircuitTestInexactQPE::ParamType>&
           inf) {
      const auto nqubits = inf.param;
      std::stringstream ss{};
      ss << nqubits;
      if (nqubits == 1) {
        ss << "_qubit";
      } else {
        ss << "_qubits";
      }
      return ss.str();
    });

TEST_P(DynamicCircuitTestInexactQPE, UnitaryEquivalence) {
  ec::EquivalenceCheckingManager ecm(qpe, iqpe, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

class DynamicCircuitTestBV : public testing::TestWithParam<qc::Qubit> {
protected:
  qc::Qubit bitwidth{};
  qc::QuantumComputation bv;
  qc::QuantumComputation dbv;
  std::unique_ptr<dd::Package> dd;
  std::ofstream ofs;

  ec::Configuration config{};

  void TearDown() override {}
  void SetUp() override {
    bitwidth = GetParam();

    dd = std::make_unique<dd::Package>(bitwidth + 1);

    bv = qc::createBernsteinVazirani(bitwidth);

    const auto expected = bv.getName().substr(3);
    dbv = qc::createIterativeBernsteinVazirani(qc::BVBitString(expected),
                                               bitwidth);

    std::cout << "Hidden bitstring: " << expected << " (" << bitwidth
              << " qubits)\n";

    config.optimizations.transformDynamicCircuit = true;
    config.optimizations.backpropagateOutputPermutation = true;
  }
};

INSTANTIATE_TEST_SUITE_P(
    Eval, DynamicCircuitTestBV, testing::Range<qc::Qubit>(1U, 64U, 5U),
    [](const testing::TestParamInfo<DynamicCircuitTestBV::ParamType>& inf) {
      const auto nqubits = inf.param;
      std::stringstream ss{};
      ss << nqubits;
      if (nqubits == 1) {
        ss << "_qubit";
      } else {
        ss << "_qubits";
      }
      return ss.str();
    });

TEST_P(DynamicCircuitTestBV, UnitaryEquivalence) {
  ec::EquivalenceCheckingManager ecm(bv, dbv, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

class DynamicCircuitTestQFT : public testing::TestWithParam<qc::Qubit> {
protected:
  qc::Qubit precision{};
  qc::QuantumComputation qft;
  qc::QuantumComputation dqft;
  std::unique_ptr<dd::Package> dd;
  std::ofstream ofs;

  ec::Configuration config{};

  void TearDown() override {}
  void SetUp() override {
    precision = GetParam();

    dd = std::make_unique<dd::Package>(precision);

    qft = qc::createQFT(precision);

    dqft = qc::createIterativeQFT(precision);

    config.optimizations.transformDynamicCircuit = true;
    config.optimizations.backpropagateOutputPermutation = true;
  }
};

INSTANTIATE_TEST_SUITE_P(
    Eval, DynamicCircuitTestQFT, testing::Range<qc::Qubit>(1U, 65U, 5U),
    [](const testing::TestParamInfo<DynamicCircuitTestQFT::ParamType>& inf) {
      const auto nqubits = inf.param;
      std::stringstream ss{};
      ss << nqubits;
      if (nqubits == 1) {
        ss << "_qubit";
      } else {
        ss << "_qubits";
      }
      return ss.str();
    });

TEST_P(DynamicCircuitTestQFT, UnitaryEquivalence) {
  ec::EquivalenceCheckingManager ecm(qft, dqft, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST(GeneralDynamicCircuitTest, DynamicCircuit) {
  constexpr auto s = qc::BVBitString(15U);
  const auto bv = qc::createBernsteinVazirani(s);
  const auto dbv = qc::createIterativeBernsteinVazirani(s);

  auto config = ec::Configuration{};
  EXPECT_THROW(ec::EquivalenceCheckingManager(bv, dbv, config),
               std::runtime_error);

  config.optimizations.transformDynamicCircuit = true;
  config.optimizations.backpropagateOutputPermutation = true;

  auto ecm = ec::EquivalenceCheckingManager(bv, dbv, config);

  ecm.run();

  EXPECT_TRUE(ecm.getResults().consideredEquivalent());

  std::cout << ecm.getResults() << "\n";

  auto ecm2 = ec::EquivalenceCheckingManager(dbv, dbv, config);

  ecm2.run();

  EXPECT_TRUE(ecm2.getResults().consideredEquivalent());

  std::cout << ecm2.getResults() << "\n";
}
