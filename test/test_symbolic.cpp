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
#include "dd/DDDefinitions.hpp"
#include "ir/QuantumComputation.hpp"
#include "ir/operations/Control.hpp"
#include "ir/operations/Expression.hpp"

#include <gtest/gtest.h>
#include <stdexcept>

using namespace qc;
using namespace sym;

namespace {

class SymbolicTest : public ::testing::Test {
public:
  Variable x = Variable("x");

  Symbolic xMonom = Symbolic{Term<dd::fp>{x}};
  Symbolic xMonomNeg = Symbolic{-Term<dd::fp>{x}};

  QuantumComputation symQc1 = QuantumComputation(1);
  QuantumComputation symQc2 = QuantumComputation(1);
};

} // namespace

TEST_F(SymbolicTest, SymbolicEqu) {
  symQc1.rx(xMonom, 0);

  symQc2.h(0);
  symQc2.barrier(0);
  symQc2.rz(xMonom, 0);
  symQc2.barrier(0);
  symQc2.h(0);

  auto ecm = ec::EquivalenceCheckingManager(symQc1, symQc2);
  ecm.run();

  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}

TEST_F(SymbolicTest, SymbolicNonEqu) {
  symQc1.rx(xMonom, 0);

  symQc2.h(0);
  symQc2.rx(xMonomNeg, 0);
  symQc2.h(0);

  auto ecm = ec::EquivalenceCheckingManager(symQc1, symQc2);
  ecm.run();

  EXPECT_FALSE(ecm.getResults().consideredEquivalent());
}

TEST_F(SymbolicTest, Timeout) {
  // construct large circuit
  constexpr auto numLayers = 100000;
  symQc1 = qc::QuantumComputation(2);
  symQc2 = qc::QuantumComputation(2);
  for (auto i = 0; i < numLayers; ++i) {
    symQc1.cx(1_pc, 0);
    symQc1.rx(xMonom, 0);

    symQc2.cx(1_pc, 0);
    symQc2.rx(xMonom, 0);
  }
  ec::Configuration config{};
  config.execution.timeout = 0.1;
  auto ecm = ec::EquivalenceCheckingManager(symQc1, symQc2, config);

  ecm.run();
  EXPECT_EQ(ecm.getResults().equivalence,
            ec::EquivalenceCriterion::NoInformation);
}

TEST_F(SymbolicTest, InvalidCircuit) {
  auto qc = qc::QuantumComputation(4);
  qc.mcy({1_pc, 2_pc, 3_pc}, 0);
  qc.rx(xMonom, 0);
  auto ecm = ec::EquivalenceCheckingManager(qc, qc);
  ecm.run();

  EXPECT_EQ(ecm.getResults().equivalence,
            ec::EquivalenceCriterion::NoInformation);
}

TEST_F(SymbolicTest, NormalizeRouting) {
  qc::QuantumComputation circuit(2);
  circuit.rz(xMonom, 0);
  circuit.cx(0, 1);
  circuit.cx(1, 0);
  circuit.cx(0, 1);
  const auto ecm = ec::EquivalenceCheckingManager(circuit, circuit);
  const auto& optimized = ecm.getFirstCircuit();
  ASSERT_EQ(optimized.size(), 1U);
  EXPECT_TRUE(optimized.front()->isSymbolicOperation());
  EXPECT_EQ(optimized.outputPermutation.at(0), 1U);
}

TEST_F(SymbolicTest, RejectSymbolicDynamicTransformation) {
  symQc1.rx(xMonom, 0);
  symQc1.reset(0);
  ec::Configuration config{};
  config.optimizations.transformDynamicCircuit = true;
  EXPECT_THROW(ec::EquivalenceCheckingManager(symQc1, symQc1, config),
               std::invalid_argument);
}

TEST_F(SymbolicTest, TransformNumericCircuitAlongsideSymbolicCircuit) {
  symQc1.rx(xMonom, 0);
  symQc2.reset(0);
  symQc2.h(0);
  ec::Configuration config{};
  config.optimizations.transformDynamicCircuit = true;
  const auto ecm = ec::EquivalenceCheckingManager(symQc1, symQc2, config);
  EXPECT_FALSE(ecm.getFirstCircuit().isVariableFree());
  EXPECT_FALSE(ecm.getSecondCircuit().isDynamic());
}

TEST_F(SymbolicTest, PreserveGatesInMeasuredCompound) {
  qc::QuantumComputation body(1, 1);
  body.h(0);
  body.rz(xMonom, 0);
  body.measure(0, 0);
  const auto reference = body;
  qc::QuantumComputation circuit(1, 1);
  circuit.emplace_back(body.asCompoundOperation());
  auto ecm = ec::EquivalenceCheckingManager(circuit, reference);
  EXPECT_FALSE(ecm.getFirstCircuit().empty());
  EXPECT_FALSE(ecm.getFirstCircuit().isVariableFree());
  ecm.run();
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}
