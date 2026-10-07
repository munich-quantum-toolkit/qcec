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
#include "ir/Definitions.hpp"
#include "ir/Permutation.hpp"
#include "ir/QuantumComputation.hpp"
#include "ir/operations/Expression.hpp"

#include <gtest/gtest.h>
#include <stdexcept>
#include <utility>
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

TEST_P(ClassicalOutputTest, PreservesOutputWiring) {
  for (const bool routed : {false, true}) {
    SCOPED_TRACE(routed);
    qc::QuantumComputation first(2, 4);
    first.x(0);
    auto second = first;
    if (routed) {
      second.swap(0, 1);
    }
    first.measure({0, 1}, {0, 3});
    second.measure({0, 1}, {3, 0});
    first.initializeIOMapping();
    second.initializeIOMapping();
    ec::EquivalenceCheckingManager checker(first, second, config);
    checker.run();
    EXPECT_EQ(checker.getResults().consideredEquivalent(), routed);
  }
}

TEST(ClassicalOutputValidation, RejectsAmbiguousMeasurementDestinations) {
  for (const auto& bits :
       {std::vector<qc::Bit>{0, 1}, std::vector<qc::Bit>{3, 3}}) {
    qc::QuantumComputation first(2, 4);
    auto second = first;
    first.measure({0, 1}, {2, 3});
    second.measure({0, 1}, bits);
    first.initializeIOMapping();
    second.initializeIOMapping();
    EXPECT_THROW((ec::EquivalenceCheckingManager(first, second)),
                 std::invalid_argument);
  }
}

TEST(ClassicalOutputValidation, NormalizesPartialOutputs) {
  for (const auto [bit, logical] : {std::pair{2U, 2U}, std::pair{4U, 1U}}) {
    qc::QuantumComputation circuit(3, 5);
    circuit.h(0);
    circuit.cx(0, 1);
    circuit.x(2);
    circuit.measure({0, 1}, {0, bit});
    circuit.initializeIOMapping();
    ec::Configuration config;
    config.functionality.checkPartialEquivalence = true;
    const ec::EquivalenceCheckingManager checker(circuit, circuit, config);
    EXPECT_EQ(checker.getFirstCircuit().outputPermutation,
              (qc::Permutation{{0, 0}, {1, logical}}));
    EXPECT_EQ(checker.getFirstCircuit().getGarbage(),
              (std::vector<bool>{false, logical != 1, logical != 2}));
  }
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
