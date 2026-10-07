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
#include "checker/dd/DDEquivalenceChecker.hpp"
#include "dd/Complex.hpp"
#include "dd/ComplexNumbers.hpp"
#include "dd/DDDefinitions.hpp"
#include "dd/Node.hpp"
#include "dd/Package.hpp"
#include "dd/RealNumber.hpp"
#include "dd/StateGeneration.hpp"
#include "ir/QuantumComputation.hpp"

#include <gtest/gtest.h>
#include <type_traits>

namespace {

template <class DDType>
class NumericalFailureChecker : public ec::DDEquivalenceChecker<DDType> {
public:
  using ec::DDEquivalenceChecker<DDType>::DDEquivalenceChecker;
  bool zeroFirst = false;
  bool zeroSecond = false;
  bool zeroTerminal = true;

protected:
  void initialize() override {
    DDType nonzero{};
    if constexpr (std::is_same_v<DDType, dd::MatrixDD>) {
      nonzero = zeroTerminal ? dd::Package::makeIdent()
                             : this->dd->createInitialMatrix({true});
    } else {
      nonzero = dd::makeZeroState(this->nqubits, *this->dd);
    }
    auto zero = zeroTerminal ? DDType::zero() : nonzero;
    zero.w = dd::Complex::zero();
    this->taskManager1.setInternalState(zeroFirst ? zero : nonzero);
    this->taskManager2.setInternalState(zeroSecond ? zero : nonzero);
    this->taskManager1.incRef();
    this->taskManager2.incRef();
    this->dd->decRef(nonzero);
  }

  ec::EquivalenceCriterion checkEquivalence() override {
    const auto result = ec::DDEquivalenceChecker<DDType>::checkEquivalence();
    this->taskManager1.decRef();
    this->taskManager2.decRef();
    return result;
  }
};

template <class DDType> class NumericalFailureTest : public testing::Test {};
using DDTypes = testing::Types<dd::MatrixDD, dd::VectorDD>;
TYPED_TEST_SUITE(NumericalFailureTest, DDTypes);

class NumericalFailureFlowTest : public testing::Test {
protected:
  void TearDown() override { dd::ComplexNumbers::setTolerance(tolerance); }

  const dd::fp tolerance = dd::RealNumber::eps;
};

TYPED_TEST(NumericalFailureTest, ZeroStateIsInconclusive) {
  const auto circuit = qc::QuantumComputation(1);
  for (const auto partial : {false, true}) {
    auto config = ec::Configuration{};
    config.functionality.checkPartialEquivalence = partial;
    for (const auto zeroFirst : {false, true}) {
      for (const auto zeroSecond : {false, true}) {
        for (const auto zeroTerminal : {false, true}) {
          auto checker =
              NumericalFailureChecker<TypeParam>(circuit, circuit, config);
          checker.zeroFirst = zeroFirst;
          checker.zeroSecond = zeroSecond;
          checker.zeroTerminal = zeroTerminal;
          EXPECT_EQ(checker.run(), zeroFirst || zeroSecond
                                       ? ec::EquivalenceCriterion::NoInformation
                                       : ec::EquivalenceCriterion::Equivalent);
        }
      }
    }
  }
}

TEST_F(NumericalFailureFlowTest, InconclusiveCheckersFinish) {
  auto first = qc::QuantumComputation(1);
  first.h(0);
  auto second = qc::QuantumComputation(1);
  second.x(0);
  for (const auto parallel : {false, true}) {
    auto config = ec::Configuration{};
    config.execution.parallel = parallel;
    config.execution.nthreads = 2;
    config.execution.numericalTolerance = 0.75;
    config.execution.runConstructionChecker = true;
    config.execution.runSimulationChecker = false;
    config.execution.runZXChecker = false;
    auto manager = ec::EquivalenceCheckingManager(first, second, config);
    manager.run();
    EXPECT_EQ(manager.equivalence(), ec::EquivalenceCriterion::NoInformation);
  }
}

TEST_F(NumericalFailureFlowTest, InconclusiveSimulationAllowsZX) {
  auto circuit = qc::QuantumComputation(1);
  circuit.h(0);
  for (const auto parallel : {false, true}) {
    auto config = ec::Configuration{};
    config.execution.parallel = parallel;
    config.execution.nthreads = 2;
    config.execution.numericalTolerance = 0.75;
    config.execution.runAlternatingChecker = false;
    config.simulation.maxSims = 1;
    auto manager = ec::EquivalenceCheckingManager(circuit, circuit, config);
    manager.run();
    EXPECT_TRUE(manager.getResults().consideredEquivalent());

    config.execution.runZXChecker = false;
    config.simulation.maxSims = 2;
    auto simulationOnly =
        ec::EquivalenceCheckingManager(circuit, circuit, config);
    simulationOnly.run();
    EXPECT_EQ(simulationOnly.equivalence(),
              ec::EquivalenceCriterion::NoInformation);
  }
}

} // namespace
