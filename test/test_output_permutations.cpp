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
#include "ir/Definitions.hpp"
#include "ir/QuantumComputation.hpp"
#include "ir/operations/Expression.hpp"

#include <gtest/gtest.h>
#include <stdexcept>
#include <vector>

namespace {
class ClassicalOutputTest : public testing::TestWithParam<int> {
protected:
  ec::Configuration config{};

  void SetUp() override {
    config.execution.parallel = false;
    config.execution.runConstructionChecker = GetParam() == 0;
    config.execution.runAlternatingChecker = GetParam() == 1;
    config.execution.runSimulationChecker = GetParam() == 2;
    config.execution.runZXChecker = GetParam() == 3;
    config.simulation.seed = 42;
  }
};
} /* namespace */

INSTANTIATE_TEST_SUITE_P(Checkers, ClassicalOutputTest,
                         testing::Values(0, 1, 2, 3));

TEST_P(ClassicalOutputTest, OffsetAndSparseDestinations) {
  for (const auto& bits : {
           std::vector<qc::Bit>{2, 3},
           std::vector<qc::Bit>{3, 2},
           std::vector<qc::Bit>{0, 3},
       }) {
    qc::QuantumComputation circuit(2, 4);
    circuit.h(0);
    circuit.cx(0, 1);
    circuit.measure({0, 1}, bits);
    circuit.initializeIOMapping();
    const auto original = circuit.outputPermutation;

    ec::EquivalenceCheckingManager checker(circuit, circuit, config);
    EXPECT_EQ(checker.getFirstCircuit().getNgarbageQubits(), 0);
    checker.run();
    EXPECT_TRUE(checker.getResults().consideredEquivalent());
    EXPECT_EQ(circuit.outputPermutation, original);
  }
}

TEST_P(ClassicalOutputTest, PreservesOutputWiring) {
  qc::QuantumComputation first(2, 4);
  first.x(0);
  auto second = first;
  first.measure({0, 1}, {2, 3});
  second.measure({0, 1}, {3, 2});
  first.initializeIOMapping();
  second.initializeIOMapping();
  ec::EquivalenceCheckingManager checker(first, second, config);
  checker.run();
  EXPECT_EQ(checker.equivalence(),
            GetParam() == 3 ? ec::EquivalenceCriterion::ProbablyNotEquivalent
                            : ec::EquivalenceCriterion::NotEquivalent);
}

TEST_P(ClassicalOutputTest, AccountsForRouting) {
  qc::QuantumComputation first(2, 4);
  first.x(0);
  auto second = first;
  first.measure({0, 1}, {2, 3});
  second.swap(0, 1);
  second.measure({0, 1}, {3, 2});
  first.initializeIOMapping();
  second.initializeIOMapping();
  ec::EquivalenceCheckingManager checker(first, second, config);
  checker.run();
  EXPECT_TRUE(checker.getResults().consideredEquivalent());
}

TEST_P(ClassicalOutputTest, RejectsDifferentClassicalLabelsWhenNormalizing) {
  qc::QuantumComputation first(2, 4);
  first.x(0);
  auto second = first;
  first.measure({0, 1}, {2, 3});
  second.measure({0, 1}, {0, 1});
  first.initializeIOMapping();
  second.initializeIOMapping();
  EXPECT_THROW((ec::EquivalenceCheckingManager(first, second, config)),
               std::invalid_argument);
}

TEST_P(ClassicalOutputTest, PreservesInRangeClassicalLabels) {
  qc::QuantumComputation first(3, 3);
  first.x(0);
  auto second = first;
  first.measure({0, 1}, {0, 2});
  second.measure({0, 1}, {1, 2});
  first.initializeIOMapping();
  second.initializeIOMapping();
  ec::EquivalenceCheckingManager checker(first, second, config);
  checker.run();
  EXPECT_FALSE(checker.getResults().consideredEquivalent());
}

TEST_P(ClassicalOutputTest, RejectsOverwrittenClassicalDestination) {
  qc::QuantumComputation circuit(2, 4);
  circuit.x(0);
  circuit.measure({0, 1}, {3, 3});
  circuit.initializeIOMapping();
  EXPECT_THROW((ec::EquivalenceCheckingManager(circuit, circuit, config)),
               std::invalid_argument);
}

TEST_P(ClassicalOutputTest, RecomputesGarbageForPartialOutputs) {
  qc::QuantumComputation circuit(3, 5);
  circuit.h(0);
  circuit.cx(0, 1);
  circuit.x(2);
  circuit.measure({0, 1}, {3, 4});
  circuit.initializeIOMapping();
  config.functionality.checkPartialEquivalence = true;
  ec::EquivalenceCheckingManager checker(circuit, circuit, config);
  EXPECT_EQ(checker.getFirstCircuit().getNgarbageQubits(), 1);
  checker.run();
  if (GetParam() == 3) {
    /// The ZX checker does not support these garbage outputs.
    EXPECT_EQ(checker.equivalence(), ec::EquivalenceCriterion::NoInformation);
  } else {
    EXPECT_TRUE(checker.getResults().consideredEquivalent());
  }
}

TEST(ClassicalOutputValidation, RejectsUnmeasuredOutputLabels) {
  qc::QuantumComputation circuit(2, 4);
  circuit.x(0);
  circuit.outputPermutation.at(0) = 2;
  EXPECT_THROW((ec::EquivalenceCheckingManager(circuit, circuit)),
               std::invalid_argument);
}

TEST(ClassicalOutputValidation, RejectsRepeatedMeasuredQubits) {
  qc::QuantumComputation circuit(2, 4);
  circuit.measure(0, 2);
  circuit.measure(0, 3);
  circuit.initializeIOMapping();
  EXPECT_THROW((ec::EquivalenceCheckingManager(circuit, circuit)),
               std::invalid_argument);
}

TEST(ClassicalOutputValidation, PreservesExplicitlyRemovedOutputs) {
  qc::QuantumComputation circuit(2, 4);
  circuit.measure({0, 1}, {2, 3});
  circuit.initializeIOMapping();
  circuit.outputPermutation.erase(0);
  EXPECT_THROW((ec::EquivalenceCheckingManager(circuit, circuit)),
               std::invalid_argument);
}

TEST(ClassicalOutputValidation, RejectsNestedMeasurementsWithSymbolicGates) {
  qc::QuantumComputation nested(2, 4);
  nested.h(0);
  nested.measure(0, 0);
  qc::QuantumComputation circuit(2, 4);
  circuit.emplace_back(nested.asOperation());
  circuit.rx(qc::Symbolic{sym::Term<qc::fp>{sym::Variable{"theta"}}}, 0);
  circuit.measure({0, 1}, {2, 3});
  circuit.initializeIOMapping();
  EXPECT_THROW((ec::EquivalenceCheckingManager(circuit, circuit)),
               std::invalid_argument);
}
