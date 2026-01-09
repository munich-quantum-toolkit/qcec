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

using namespace qc;
using namespace sym;
class SymbolicTest : public ::testing::Test {
public:
  Variable x = Variable("x");

  Symbolic xMonom = Symbolic{Term<dd::fp>{x}};
  Symbolic xMonomNeg = Symbolic{-Term<dd::fp>{x}};

  QuantumComputation symQc1 = QuantumComputation(1);
  QuantumComputation symQc2 = QuantumComputation(1);
};

TEST_F(SymbolicTest, SymbolicEqu) {
  symQc1.rx(xMonom, 0);

  symQc2.h(0);
  symQc2.rz(xMonom, 0);
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
  qc.mcx({1_pc, 2_pc, 3_pc}, 0);
  qc.rx(xMonom, 0);
  auto ecm = ec::EquivalenceCheckingManager(qc, qc);
  ecm.run();

  EXPECT_EQ(ecm.getResults().equivalence,
            ec::EquivalenceCriterion::NoInformation);
}
