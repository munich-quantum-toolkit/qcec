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
#include "qasm3/Importer.hpp"

#include <algorithm>
#include <array>
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

namespace {

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

    qpe = ec::test::createQPE(precision);

    // extract lambda from QPE global phase
    const auto lambda = qpe.getGlobalPhase();

    iqpe = ec::test::createIterativeQPE(lambda, precision);

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

    qpe = ec::test::createQPE(precision);

    // extract lambda from QPE global phase
    const auto lambda = qpe.getGlobalPhase();

    iqpe = ec::test::createIterativeQPE(lambda, precision);

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

    bv = ec::test::createBernsteinVazirani(bitwidth);

    const auto expected = bv.getName().substr(3);
    dbv = ec::test::createIterativeBernsteinVazirani(
        ec::test::BVBitString(expected), bitwidth);

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

    qft = ec::test::createQFT(precision);

    dqft = ec::test::createIterativeQFT(precision);

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
  constexpr auto s = ec::test::BVBitString(15U);
  const auto bv = ec::test::createBernsteinVazirani(s);
  const auto dbv = ec::test::createIterativeBernsteinVazirani(s);

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

} // namespace

TEST(DynamicCircuitTest, BarriersDoNotPreventFinalMeasurements) {
  qc::QuantumComputation reference(2, 1);
  reference.h(0);
  reference.x(1);
  reference.measure(0, 0);
  reference.initializeIOMapping();
  for (const auto nested : {false, true}) {
    qc::QuantumComputation circuit(2, 1);
    circuit.h(0);
    circuit.measure(0, 0);
    if (nested) {
      qc::QuantumComputation tail(2);
      qc::QuantumComputation barrier(2);
      barrier.barrier({0, 1});
      tail.emplace_back(barrier.asCompoundOperation());
      tail.x(1);
      tail.barrier({0, 1});
      circuit.emplace_back(tail.asCompoundOperation());
    } else {
      circuit.barrier({0, 1});
      circuit.x(1);
    }
    circuit.initializeIOMapping();
    for (const auto transform : {false, true}) {
      SCOPED_TRACE(testing::Message()
                   << "nested=" << nested << ", transform=" << transform);
      ec::Configuration config;
      config.optimizations.transformDynamicCircuit = transform;
      config.execution.runSimulationChecker = false;
      config.execution.runZXChecker = false;
      ec::EquivalenceCheckingManager ecm(reference, circuit, config);
      ecm.run();
      EXPECT_TRUE(ecm.getResults().consideredEquivalent());
    }
  }
}

TEST(DynamicCircuitTest, RouteLogicalOutputsThroughOneReadoutQubit) {
  ec::Configuration config;
  config.optimizations.transformDynamicCircuit = true;
  config.functionality.checkPartialEquivalence = true;
  config.execution.runSimulationChecker = false;
  config.execution.runZXChecker = false;
  config.execution.runAlternatingChecker = false;
  config.execution.runConstructionChecker = true;
  std::array<qc::Qubit, 3> layout{0, 1, 2};
  for (auto moreLayouts = true; moreLayouts;
       moreLayouts = std::ranges::next_permutation(layout).found) {
    for (const auto decompose : {false, true}) {
      for (const qc::Qubit outputs : {2U, 3U}) {
        qc::QuantumComputation reference(3, outputs);
        reference.h(0);
        reference.cx(0, 1);
        reference.cx(1, 2);
        for (qc::Qubit q = 0; q < outputs; ++q) {
          reference.measure(q, q);
        }
        reference.initializeIOMapping();
        for (const auto error : {false, true}) {
          SCOPED_TRACE(testing::Message()
                       << "layout=" << layout[0] << layout[1] << layout[2]
                       << ", decompose=" << decompose << ", outputs=" << outputs
                       << ", error=" << error);
          qc::QuantumComputation routed(3, outputs);
          for (qc::Qubit q = 0; q < 3; ++q) {
            routed.initialLayout[layout.at(q)] = q;
          }
          routed.h(layout[0]);
          routed.cx(layout[0], layout[1]);
          routed.cx(layout[1], layout[2]);
          if (error) {
            routed.x(layout[1]);
          }
          routed.measure(layout[0], 0);
          for (qc::Qubit q = 1; q < outputs; ++q) {
            if (decompose) {
              routed.cx(layout[0], layout.at(q));
              routed.cx(layout.at(q), layout[0]);
              routed.cx(layout[0], layout.at(q));
            } else {
              routed.swap(layout[0], layout.at(q));
            }
            routed.measure(layout[0], q);
          }
          routed.initializeIOMapping();
          ec::EquivalenceCheckingManager ecm(reference, routed, config);
          ecm.run();
          if (error) {
            EXPECT_EQ(ecm.equivalence(),
                      ec::EquivalenceCriterion::NotEquivalent);
          } else {
            EXPECT_TRUE(ecm.getResults().consideredEquivalent());
          }
        }
      }
    }
  }
}

TEST(DynamicCircuitTest, SteaneRoutedReadout) {
  qc::QuantumComputation reference(10, 3);
  reference.x(5);
  for (const qc::Qubit q : {0U, 2U, 4U, 6U}) {
    reference.cx(q, 7);
  }
  for (const qc::Qubit q : {1U, 2U, 5U, 6U}) {
    reference.cx(q, 8);
  }
  for (const qc::Qubit q : {3U, 4U, 5U, 6U}) {
    reference.cx(q, 9);
  }
  reference.measure({7, 8, 9}, {0, 1, 2});
  reference.initializeIOMapping();
  const auto routed =
      qasm3::Importer::importf("circuits/test/steane_routed_readout.qasm");
  ec::Configuration config;
  config.optimizations.transformDynamicCircuit = true;
  config.execution.runSimulationChecker = false;
  config.execution.runZXChecker = false;
  for (const auto backpropagate : {false, true}) {
    config.optimizations.backpropagateOutputPermutation = backpropagate;
    ec::EquivalenceCheckingManager ecm(reference, routed, config);
    ecm.run();
    EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::NotEquivalent);
  }
}
