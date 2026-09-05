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
#include "ir/operations/Expression.hpp"

#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

enum class DDChecker : std::uint8_t { Alternating, Construction };

std::string checkerName(const testing::TestParamInfo<DDChecker>& info) {
  return info.param == DDChecker::Alternating ? "Alternating" : "Construction";
}

class ApproximateEquivalenceTest : public testing::TestWithParam<DDChecker> {
protected:
  [[nodiscard]] static ec::Configuration configuration(const double threshold) {
    ec::Configuration config{};
    config.execution.parallel = false;
    config.execution.runSimulationChecker = false;
    config.execution.runAlternatingChecker =
        GetParam() == DDChecker::Alternating;
    config.execution.runConstructionChecker =
        GetParam() == DDChecker::Construction;
    config.execution.runZXChecker = false;
    config.functionality.checkApproximateEquivalence = true;
    config.functionality.approximateCheckingThreshold = threshold;
    return config;
  }
};

TEST_P(ApproximateEquivalenceTest, AcceptsDistanceBelowThreshold) {
  auto qc1 = qc::QuantumComputation(1);
  auto qc2 = qc::QuantumComputation(1);
  qc1.x(0);
  qc2.x(0);
  qc2.ry(dd::PI / 3., 0);

  // D_HS(X, RY(pi/3) X) = sin(pi/6) = 0.5.
  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, configuration(0.51));
  manager.run();

  EXPECT_EQ(manager.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST_P(ApproximateEquivalenceTest, RejectsDistanceAboveThreshold) {
  auto qc1 = qc::QuantumComputation(1);
  auto qc2 = qc::QuantumComputation(1);
  qc1.x(0);
  qc2.x(0);
  qc2.ry(dd::PI / 3., 0);

  // D_HS(X, RY(pi/3) X) = sin(pi/6) = 0.5.
  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, configuration(0.49));
  manager.run();

  EXPECT_EQ(manager.equivalence(), ec::EquivalenceCriterion::NotEquivalent);
}

TEST_P(ApproximateEquivalenceTest, IncludesMaximumDistanceBoundary) {
  auto qc1 = qc::QuantumComputation(1);
  auto qc2 = qc::QuantumComputation(1);
  qc1.x(0);
  qc2.z(0);

  // D_HS(X, Z) = 1, which is the inclusive upper threshold boundary.
  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, configuration(1.));
  manager.run();

  EXPECT_EQ(manager.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST_P(ApproximateEquivalenceTest, IncludesExactZeroBoundary) {
  auto qc1 = qc::QuantumComputation(1);
  auto qc2 = qc::QuantumComputation(1);
  qc1.h(0);
  qc2.h(0);

  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, configuration(0.));
  manager.run();

  EXPECT_EQ(manager.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST_P(ApproximateEquivalenceTest, PreservesExactGlobalPhaseResult) {
  auto qc1 = qc::QuantumComputation(1);
  auto qc2 = qc::QuantumComputation(1);
  qc1.x(0);
  qc2.x(0);
  qc2.z(0);
  qc2.x(0);
  qc2.z(0);
  qc2.x(0);

  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, configuration(0.));
  manager.run();

  EXPECT_EQ(manager.equivalence(),
            ec::EquivalenceCriterion::EquivalentUpToGlobalPhase);
}

INSTANTIATE_TEST_SUITE_P(DecisionDiagramCheckers, ApproximateEquivalenceTest,
                         testing::Values(DDChecker::Alternating,
                                         DDChecker::Construction),
                         checkerName);

class ApproximateConfigurationTest : public testing::Test {
protected:
  static ec::Configuration validConfiguration() {
    ec::Configuration config{};
    config.execution.parallel = false;
    config.execution.runSimulationChecker = false;
    config.execution.runAlternatingChecker = true;
    config.execution.runConstructionChecker = false;
    config.execution.runZXChecker = false;
    config.functionality.checkApproximateEquivalence = true;
    return config;
  }

  qc::QuantumComputation qc1{2};
  qc::QuantumComputation qc2{2};
};

class InvalidApproximateThresholdTest
    : public ApproximateConfigurationTest,
      public testing::WithParamInterface<double> {};

TEST_P(InvalidApproximateThresholdTest, RejectsInvalidThreshold) {
  qc1.x(0);
  qc2.x(0);
  auto config = validConfiguration();
  config.functionality.approximateCheckingThreshold = GetParam();
  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, config);

  EXPECT_THROW(manager.run(), std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(
    InvalidThresholds, InvalidApproximateThresholdTest,
    testing::Values(-0.01, 1.01, std::numeric_limits<double>::quiet_NaN(),
                    std::numeric_limits<double>::infinity(),
                    -std::numeric_limits<double>::infinity()));

TEST_F(ApproximateConfigurationTest, RejectsPartialEquivalenceCombination) {
  qc1.x(0);
  qc2.x(0);
  auto config = validConfiguration();
  config.functionality.checkPartialEquivalence = true;
  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, config);

  EXPECT_THROW(manager.run(), std::invalid_argument);
}

TEST_F(ApproximateConfigurationTest, RejectsActedUponAncillaryQubits) {
  qc1.x(0);
  qc1.x(1);
  qc2.x(0);
  qc2.x(1);
  qc1.setLogicalQubitAncillary(1);
  qc2.setLogicalQubitAncillary(1);
  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, validConfiguration());

  EXPECT_THROW(manager.run(), std::invalid_argument);
}

TEST_F(ApproximateConfigurationTest, RejectsActedUponGarbageQubits) {
  qc1.x(0);
  qc1.x(1);
  qc2.x(0);
  qc2.x(1);
  qc1.setLogicalQubitGarbage(1);
  qc2.setLogicalQubitGarbage(1);
  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, validConfiguration());

  EXPECT_THROW(manager.run(), std::invalid_argument);
}

TEST_F(ApproximateConfigurationTest, RejectsSymbolicCircuits) {
  const auto variable = sym::Variable("theta");
  const auto angle = qc::Symbolic{sym::Term<dd::fp>{variable}};
  qc1.rx(angle, 0);
  qc2.rx(angle, 0);
  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, validConfiguration());

  EXPECT_THROW(manager.run(), std::invalid_argument);
}

TEST_F(ApproximateConfigurationTest, RequiresDecisionDiagramChecker) {
  qc1.x(0);
  qc2.x(0);
  auto config = validConfiguration();
  config.execution.runAlternatingChecker = false;
  config.execution.runSimulationChecker = true;
  config.execution.runZXChecker = true;
  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, config);

  EXPECT_THROW(manager.run(), std::invalid_argument);
}

TEST_F(ApproximateConfigurationTest, RejectsConfigurationWithoutCheckers) {
  qc1.x(0);
  qc2.x(0);
  auto config = validConfiguration();
  config.execution.runAlternatingChecker = false;
  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, config);

  EXPECT_THROW(manager.run(), std::invalid_argument);
}

TEST_F(ApproximateConfigurationTest, ValidatesConfigurationChangesOnRun) {
  qc1.x(0);
  qc2.x(0);
  auto config = validConfiguration();
  config.functionality.checkApproximateEquivalence = false;
  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, config);
  manager.getConfiguration().functionality.checkApproximateEquivalence = true;
  manager.getConfiguration().functionality.approximateCheckingThreshold =
      std::numeric_limits<double>::quiet_NaN();

  EXPECT_THROW(manager.run(), std::invalid_argument);
}

class ApproximateDefaultFlowTest : public testing::TestWithParam<bool> {};

TEST_P(ApproximateDefaultFlowTest, DisablesUnsupportedCheckers) {
  auto qc1 = qc::QuantumComputation(1);
  auto qc2 = qc::QuantumComputation(1);
  qc1.x(0);
  qc2.x(0);
  qc2.ry(dd::PI / 3., 0);

  auto config = ec::Configuration{};
  config.execution.parallel = GetParam();
  config.execution.nthreads = 2U;
  // Exercise the actual parallel flow with both supported DD checkers.
  config.execution.runConstructionChecker = GetParam();
  config.functionality.checkApproximateEquivalence = true;
  config.functionality.approximateCheckingThreshold = 0.51;
  ASSERT_TRUE(config.execution.runSimulationChecker);
  ASSERT_TRUE(config.execution.runZXChecker);
  auto manager = ec::EquivalenceCheckingManager(qc1, qc2, config);

  manager.run();

  EXPECT_EQ(manager.equivalence(), ec::EquivalenceCriterion::Equivalent);
  EXPECT_FALSE(manager.getConfiguration().execution.runSimulationChecker);
  EXPECT_FALSE(manager.getConfiguration().execution.runZXChecker);
  EXPECT_EQ(manager.getResults().startedSimulations, 0U);
  EXPECT_EQ(manager.getResults().performedSimulations, 0U);
}

INSTANTIATE_TEST_SUITE_P(SequentialAndParallel, ApproximateDefaultFlowTest,
                         testing::Bool());

TEST(ApproximateConfigurationJsonTest, SerializesApproximateOptions) {
  auto config = ec::Configuration{};
  config.functionality.checkApproximateEquivalence = true;
  config.functionality.approximateCheckingThreshold = 0.125;

  const auto json = config.json();

  EXPECT_TRUE(json["functionality"]["check_approximate_equivalence"]);
  EXPECT_DOUBLE_EQ(json["functionality"]["approximate_checking_threshold"],
                   0.125);
}

} // namespace
