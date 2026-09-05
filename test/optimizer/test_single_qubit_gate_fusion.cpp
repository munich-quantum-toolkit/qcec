/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "dd/FunctionalityConstruction.hpp"
#include "dd/Package.hpp"
#include "ir/QuantumComputation.hpp"
#include "optimizer/EquivalenceCheckingOptimizer.hpp"

#include <cstddef>
#include <gtest/gtest.h>

namespace ec::detail {
namespace {
void expectFusionPreservesFunctionality(qc::QuantumComputation& circuit,
                                        const std::size_t expectedOperations) {
  dd::Package package(circuit.getNqubits());
  const auto before = dd::buildFunctionality(circuit, package);

  singleQubitGateFusion(circuit);
  const auto after = dd::buildFunctionality(circuit, package);

  EXPECT_EQ(circuit.getNops(), expectedOperations);
  EXPECT_EQ(before, after);

  package.decRef(before);
  package.decRef(after);
  package.garbageCollect(true);

  const auto [vectors, matrices, realNumbers] = package.computeActiveCounts();
  EXPECT_EQ(vectors, 0U);
  EXPECT_EQ(matrices, 0U);
  EXPECT_EQ(realNumbers, 0U);
}
} // namespace

TEST(SingleQubitGateFusion, CollapseCompoundOperationToStandard) {
  qc::QuantumComputation circuit(1U);
  circuit.x(0);
  circuit.i(0);

  singleQubitGateFusion(circuit);

  ASSERT_EQ(circuit.getNops(), 1U);
  EXPECT_TRUE(circuit.front()->isStandardOperation());
}

TEST(SingleQubitGateFusion, EliminateCompoundOperation) {
  qc::QuantumComputation circuit(1U);
  circuit.i(0);
  circuit.i(0);

  singleQubitGateFusion(circuit);

  EXPECT_TRUE(circuit.empty());
}

TEST(SingleQubitGateFusion, EliminateInverseInCompoundOperation) {
  qc::QuantumComputation circuit(1U);
  circuit.s(0);
  circuit.sdg(0);

  singleQubitGateFusion(circuit);

  EXPECT_TRUE(circuit.empty());
}

TEST(SingleQubitGateFusion, PreserveUnknownInverseInCompoundOperation) {
  qc::QuantumComputation circuit(1U);
  circuit.p(1., 0);
  circuit.p(-1., 0);

  singleQubitGateFusion(circuit);

  EXPECT_EQ(circuit.getNops(), 1U);
}

TEST(SingleQubitGateFusion, RepeatedCancellation) {
  qc::QuantumComputation circuit(1U);
  circuit.x(0);
  circuit.h(0);
  circuit.h(0);
  circuit.x(0);
  circuit.z(0);

  singleQubitGateFusion(circuit);

  EXPECT_EQ(circuit.getNops(), 1U);
}

TEST(SingleQubitGateFusion, RemoveEmptyCompoundOperation) {
  qc::QuantumComputation circuit(1U);
  circuit.x(0);
  circuit.h(0);
  circuit.h(0);
  circuit.x(0);

  singleQubitGateFusion(circuit);

  EXPECT_TRUE(circuit.empty());
}

TEST(SingleQubitGateFusion, PreserveOperationCounts) {
  qc::QuantumComputation circuit(2U, 2U);
  circuit.x(0);
  circuit.h(0);
  circuit.cx(1, 0);
  circuit.z(0);
  circuit.measure(0, 0);

  ASSERT_EQ(circuit.getNops(), 5U);
  ASSERT_EQ(circuit.getNindividualOps(), 5U);
  ASSERT_EQ(circuit.getNsingleQubitOps(), 3U);

  singleQubitGateFusion(circuit);

  EXPECT_EQ(circuit.getNops(), 4U);
  EXPECT_EQ(circuit.getNindividualOps(), 5U);
  EXPECT_EQ(circuit.getNsingleQubitOps(), 3U);
}

TEST(SingleQubitGateFusion, PreserveTwoGateFunctionality) {
  qc::QuantumComputation circuit(1U);
  circuit.x(0);
  circuit.h(0);

  expectFusionPreservesFunctionality(circuit, 1U);
}

TEST(SingleQubitGateFusion, PreserveThreeGateFunctionality) {
  qc::QuantumComputation circuit(1U);
  circuit.x(0);
  circuit.h(0);
  circuit.y(0);

  expectFusionPreservesFunctionality(circuit, 1U);
}

TEST(SingleQubitGateFusion, PreserveSeparatedFunctionality) {
  qc::QuantumComputation circuit(2U);
  circuit.h(0);
  circuit.cx(0, 1);
  circuit.y(0);

  expectFusionPreservesFunctionality(circuit, 3U);
}

TEST(SingleQubitGateFusion, FuseAcrossIndependentGates) {
  qc::QuantumComputation circuit(2U);
  circuit.h(0);
  circuit.z(1);
  circuit.y(0);

  expectFusionPreservesFunctionality(circuit, 2U);
}

} // namespace ec::detail
