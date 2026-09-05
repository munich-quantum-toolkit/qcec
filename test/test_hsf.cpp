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
#include "checker/dd/DDHybridSchrodingerFeynmanChecker.hpp"
#include "ir/Permutation.hpp"
#include "ir/QuantumComputation.hpp"
#include "ir/operations/Control.hpp"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <gtest/gtest.h>
#include <stdexcept>
#include <tuple>

namespace {

ec::Configuration hsfConfiguration(const double threshold = 0.) {
  ec::Configuration config{};
  config.execution.parallel = false;
  config.execution.nthreads = 1U;
  config.execution.runConstructionChecker = false;
  config.execution.runAlternatingChecker = false;
  config.execution.runSimulationChecker = false;
  config.execution.runZXChecker = false;
  config.execution.runHSFChecker = true;
  config.functionality.checkApproximateEquivalence = true;
  config.functionality.approximateCheckingThreshold = threshold;
  return config;
}

TEST(HybridSchrodingerFeynmanTest, RunsAsExclusiveSequentialManagerChecker) {
  auto qc1 = qc::QuantumComputation(2);
  auto qc2 = qc::QuantumComputation(2);
  qc1.h(0);
  qc1.cx(0, 1);
  qc2.h(0);
  qc2.cx(0, 1);

  auto config = ec::Configuration{};
  config.execution.nthreads = 1U;
  config.execution.runHSFChecker = true;
  config.functionality.checkApproximateEquivalence = true;
  config.functionality.approximateCheckingThreshold = 0.;
  ASSERT_TRUE(config.execution.parallel);
  ASSERT_TRUE(config.execution.runAlternatingChecker);
  ASSERT_TRUE(config.execution.runSimulationChecker);
  ASSERT_TRUE(config.execution.runZXChecker);
  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, config);
  manager.run();

  EXPECT_EQ(manager.equivalence(), ec::EquivalenceCriterion::Equivalent);
  EXPECT_FALSE(manager.getConfiguration().execution.parallel);
  EXPECT_FALSE(manager.getConfiguration().execution.runConstructionChecker);
  EXPECT_FALSE(manager.getConfiguration().execution.runAlternatingChecker);
  EXPECT_FALSE(manager.getConfiguration().execution.runSimulationChecker);
  EXPECT_FALSE(manager.getConfiguration().execution.runZXChecker);
  EXPECT_TRUE(manager.getConfiguration().execution.runHSFChecker);
  EXPECT_EQ(manager.getConfiguration().execution.nthreads, 1U);
}

TEST(HybridSchrodingerFeynmanTest, PreservesExactGlobalPhaseResult) {
  auto qc1 = qc::QuantumComputation(2);
  auto qc2 = qc::QuantumComputation(2);
  qc1.h(0);
  qc1.x(1);
  qc2.h(0);
  qc2.x(1);
  // ZXZX = -I.
  qc2.z(0);
  qc2.x(0);
  qc2.z(0);
  qc2.x(0);

  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, hsfConfiguration());
  manager.run();

  EXPECT_EQ(manager.equivalence(),
            ec::EquivalenceCriterion::EquivalentUpToGlobalPhase);
}

class CrossCutControlTest
    : public testing::TestWithParam<
          std::tuple<qc::Qubit, qc::Qubit, qc::Control::Type>> {};

TEST_P(CrossCutControlTest, MatchesAnalyticDecomposition) {
  const auto [control, target, type] = GetParam();
  const auto oppositeType = type == qc::Control::Type::Pos
                                ? qc::Control::Type::Neg
                                : qc::Control::Type::Pos;
  auto qc1 = qc::QuantumComputation(2);
  auto qc2 = qc::QuantumComputation(2);
  qc1.cx(qc::Control{control, type}, target);
  // C_t(X) = X_control C_not-t(X) X_control.
  qc2.x(control);
  qc2.cx(qc::Control{control, oppositeType}, target);
  qc2.x(control);

  auto config = hsfConfiguration();
  config.execution.nthreads = 2U;
  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, config);
  manager.run();

  EXPECT_EQ(manager.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST(HybridSchrodingerFeynmanTest, ExposesConfigurationContracts) {
  auto config = ec::Configuration{};
  config.execution.runAlternatingChecker = false;
  config.execution.runConstructionChecker = false;
  config.execution.runSimulationChecker = false;
  config.execution.runZXChecker = false;
  config.execution.runHSFChecker = true;

  EXPECT_TRUE(config.anythingToExecute());
  EXPECT_TRUE(config.onlySingleTask());
  EXPECT_TRUE(config.json()["execution"]["run_hsf_checker"]);

  const auto qc1 = qc::QuantumComputation(2);
  const auto qc2 = qc::QuantumComputation(2);
  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, config);
  manager.disableAllCheckers();
  EXPECT_FALSE(manager.getConfiguration().execution.runHSFChecker);
}

TEST(HybridSchrodingerFeynmanTest, RequiresApproximateMode) {
  auto qc1 = qc::QuantumComputation(2);
  auto qc2 = qc::QuantumComputation(2);
  qc1.h(0);
  qc2.h(0);
  auto config = hsfConfiguration();
  config.functionality.checkApproximateEquivalence = false;
  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, config);

  EXPECT_THROW(manager.run(), std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(
    BothOrientationsAndPolarities, CrossCutControlTest,
    testing::Values(
        std::tuple{qc::Qubit{0}, qc::Qubit{1}, qc::Control::Type::Pos},
        std::tuple{qc::Qubit{0}, qc::Qubit{1}, qc::Control::Type::Neg},
        std::tuple{qc::Qubit{1}, qc::Qubit{0}, qc::Control::Type::Pos},
        std::tuple{qc::Qubit{1}, qc::Qubit{0}, qc::Control::Type::Neg}));

class NegativeControlDistanceTest
    : public testing::TestWithParam<std::tuple<qc::Qubit, qc::Qubit>> {};

TEST_P(NegativeControlDistanceTest, UsesProjectiveHilbertSchmidtDistance) {
  const auto [control, target] = GetParam();
  auto controlledX = qc::QuantumComputation(2);
  const auto identity = qc::QuantumComputation(2);
  controlledX.cx(qc::Control{control, qc::Control::Type::Neg}, target);

  // For a two-qubit controlled-X and the identity,
  // D_HS = sqrt(1 - |Tr(CX) / 4|^2) = sqrt(3) / 2.
  constexpr auto delta = 1e-6;
  const auto distance = std::sqrt(3.) / 2.;
  auto acceptChecker = ec::DDHybridSchrodingerFeynmanChecker(
      controlledX, identity, hsfConfiguration(distance + delta));
  auto rejectChecker = ec::DDHybridSchrodingerFeynmanChecker(
      controlledX, identity, hsfConfiguration(distance - delta));

  EXPECT_EQ(acceptChecker.run(), ec::EquivalenceCriterion::Equivalent);
  EXPECT_EQ(rejectChecker.run(), ec::EquivalenceCriterion::NotEquivalent);
}

INSTANTIATE_TEST_SUITE_P(BothCutOrientations, NegativeControlDistanceTest,
                         testing::Values(std::tuple{qc::Qubit{0}, qc::Qubit{1}},
                                         std::tuple{qc::Qubit{1},
                                                    qc::Qubit{0}}));

TEST(HybridSchrodingerFeynmanTest, RejectsSingleQubitCircuits) {
  const auto qc1 = qc::QuantumComputation(1);
  const auto qc2 = qc::QuantumComputation(1);

  EXPECT_FALSE(ec::DDHybridSchrodingerFeynmanChecker::canHandle(qc1, qc2));
  EXPECT_THROW(
      ec::DDHybridSchrodingerFeynmanChecker(qc1, qc2, hsfConfiguration()),
      std::invalid_argument);
}

TEST(HybridSchrodingerFeynmanTest, RejectsIncompletePermutations) {
  auto qc1 = qc::QuantumComputation(2);
  auto qc2 = qc::QuantumComputation(2);
  qc1.h(0);
  qc2.h(0);
  qc1.x(1);
  qc2.x(1);
  qc1.outputPermutation.erase(1);
  auto config = hsfConfiguration();
  config.optimizations.elidePermutations = false;

  EXPECT_FALSE(ec::DDHybridSchrodingerFeynmanChecker::canHandle(qc1, qc2));
  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, config);
  EXPECT_THROW(manager.run(), std::invalid_argument);
}

TEST(HybridSchrodingerFeynmanTest, RejectsTargetsSpanningTheCut) {
  auto qc1 = qc::QuantumComputation(2);
  const auto qc2 = qc::QuantumComputation(2);
  qc1.swap(0, 1);

  EXPECT_FALSE(ec::DDHybridSchrodingerFeynmanChecker::canHandle(qc1, qc2));
  EXPECT_THROW(
      ec::DDHybridSchrodingerFeynmanChecker(qc1, qc2, hsfConfiguration()),
      std::invalid_argument);
}

TEST(HybridSchrodingerFeynmanTest, HandlesResidualOutputPermutation) {
  auto qc1 = qc::QuantumComputation(2);
  auto qc2 = qc::QuantumComputation(2);
  qc1.h(0);
  qc1.x(1);
  qc2.h(0);
  qc2.x(1);
  qc2.outputPermutation = qc::Permutation{{0, 1}, {1, 0}};

  auto config = hsfConfiguration();
  config.optimizations.elidePermutations = false;

  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, config);
  manager.run();
  EXPECT_EQ(manager.equivalence(), ec::EquivalenceCriterion::NotEquivalent);
}

TEST(HybridSchrodingerFeynmanTest, HandlesResidualInitialLayout) {
  auto qc1 = qc::QuantumComputation(2);
  auto qc2 = qc::QuantumComputation(2);
  qc1.h(0);
  qc1.x(1);
  qc2.h(0);
  qc2.x(1);
  qc2.initialLayout = qc::Permutation{{0, 1}, {1, 0}};

  auto config = hsfConfiguration();
  config.optimizations.elidePermutations = false;

  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, config);
  manager.run();
  EXPECT_EQ(manager.equivalence(), ec::EquivalenceCriterion::NotEquivalent);
}

TEST(HybridSchrodingerFeynmanTest, HandlesReconstructedSwap) {
  auto qc1 = qc::QuantumComputation(2);
  auto qc2 = qc::QuantumComputation(2);
  for (auto* const circuit : {&qc1, &qc2}) {
    circuit->cx(0, 1);
    circuit->cx(1, 0);
    circuit->cx(0, 1);
  }

  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, hsfConfiguration());
  manager.run();

  EXPECT_EQ(manager.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST(HybridSchrodingerFeynmanTest, PreservesControlledSwap) {
  auto controlledSwap = qc::QuantumComputation(3);
  const auto identity = qc::QuantumComputation(3);
  controlledSwap.cswap(0, 1, 2);

  // For a three-qubit controlled-SWAP and the identity,
  // D_HS = sqrt(1 - |Tr(CSWAP) / 8|^2) = sqrt(7) / 4.
  constexpr auto delta = 1e-6;
  const auto distance = std::sqrt(7.) / 4.;
  auto acceptManager = ec::EquivalenceCheckingManager(
      controlledSwap, identity, hsfConfiguration(distance + delta));
  auto rejectManager = ec::EquivalenceCheckingManager(
      controlledSwap, identity, hsfConfiguration(distance - delta));

  acceptManager.run();
  rejectManager.run();

  EXPECT_EQ(acceptManager.equivalence(), ec::EquivalenceCriterion::Equivalent);
  EXPECT_EQ(rejectManager.equivalence(),
            ec::EquivalenceCriterion::NotEquivalent);
}

TEST(HybridSchrodingerFeynmanTest, HandlesEmptyCircuits) {
  const auto qc1 = qc::QuantumComputation(2);
  const auto qc2 = qc::QuantumComputation(2);

  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, hsfConfiguration());
  manager.run();

  EXPECT_EQ(manager.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST(HybridSchrodingerFeynmanTest,
     DistinguishesEmptyCircuitsWithDifferentPermutations) {
  const auto identity = qc::QuantumComputation(3);
  auto permuted = qc::QuantumComputation(3);
  permuted.outputPermutation = qc::Permutation{{0, 1}, {1, 2}, {2, 0}};

  auto manager =
      ec::EquivalenceCheckingManager(identity, permuted, hsfConfiguration());
  manager.run();

  EXPECT_EQ(manager.equivalence(), ec::EquivalenceCriterion::NotEquivalent);
}

TEST(HybridSchrodingerFeynmanTest, HandlesLayoutOnOneEmptyCircuit) {
  auto identity = qc::QuantumComputation(2);
  auto nonIdentity = qc::QuantumComputation(2);
  identity.initialLayout = qc::Permutation{{0, 1}, {1, 0}};
  identity.outputPermutation = identity.initialLayout;
  nonIdentity.x(0);
  nonIdentity.z(1);

  auto manager =
      ec::EquivalenceCheckingManager(identity, nonIdentity, hsfConfiguration());
  manager.run();

  EXPECT_EQ(manager.equivalence(), ec::EquivalenceCriterion::NotEquivalent);
}

TEST(HybridSchrodingerFeynmanTest, HonorsPreexistingCancellation) {
  auto qc1 = qc::QuantumComputation(2);
  const auto qc2 = qc::QuantumComputation(2);
  for (std::size_t i = 0; i < 30U; ++i) {
    qc1.cx(static_cast<qc::Qubit>(i % 2U),
           static_cast<qc::Qubit>((i + 1U) % 2U));
    qc1.h(1);
  }
  auto checker =
      ec::DDHybridSchrodingerFeynmanChecker(qc1, qc2, hsfConfiguration());
  checker.signalDone();

  const auto start = std::chrono::steady_clock::now();
  const auto result = checker.run();
  const auto runtime = std::chrono::steady_clock::now() - start;

  EXPECT_EQ(result, ec::EquivalenceCriterion::NoInformation);
  EXPECT_LT(runtime, std::chrono::seconds(1));
}

TEST(HybridSchrodingerFeynmanTest, HonorsManagerTimeout) {
  auto qc1 = qc::QuantumComputation(2);
  const auto qc2 = qc::QuantumComputation(2);
  for (std::size_t i = 0; i < 30U; ++i) {
    qc1.cx(static_cast<qc::Qubit>(i % 2U),
           static_cast<qc::Qubit>((i + 1U) % 2U));
    qc1.h(1);
  }
  auto config = hsfConfiguration();
  config.execution.timeout = 0.01;
  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, config);

  const auto start = std::chrono::steady_clock::now();
  manager.run();
  const auto runtime = std::chrono::steady_clock::now() - start;

  EXPECT_EQ(manager.equivalence(), ec::EquivalenceCriterion::NoInformation);
  EXPECT_LT(runtime, std::chrono::seconds(2));
}

} // namespace
