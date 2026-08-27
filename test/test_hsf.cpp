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
#include "ir/QuantumComputation.hpp"
#include "qasm3/Importer.hpp"

#include <gtest/gtest.h>
#include <string>
/**
 * Approximate equivalence checking of an original circuit 'c1' and its
 * approximate version 'c2'. The circuits 'c2' were generated using the BQSKit
 * compiler with synthesis_epsilon=0.3 and error_threshold=0.5.
 *
 */
class HSFTest : public testing::Test {
protected:
  ec::Configuration config{};

  std::string testDir = "./circuits/approximateEquivalenceTest/";

  void SetUp() override {
    config.execution.runConstructionChecker = false;
    config.execution.runAlternatingChecker = false;
    config.execution.runSimulationChecker = false;
    config.execution.runZXChecker = false;
    config.execution.runHSFChecker = true;
    config.functionality.checkApproximateEquivalence = true;
  }
};

TEST_F(HSFTest, approximateEquivalenceCheckingWithHSF) {
  // Test that enabling the approximate equivalence flag is required to use the
  // HSF checker
  const auto c1 =
      qasm3::Importer::importf(testDir + "dj_indep_qiskit_10_no_measure.qasm");
  const auto c2 = qasm3::Importer::importf(
      testDir + "out_dj_indep_qiskit_10_high_error.qasm");
  config.functionality.checkApproximateEquivalence = false;
  ec::EquivalenceCheckingManager ecm(c1, c2, config);
  ecm.run();
}

TEST_F(HSFTest, dj10ParallelChecking) {
  // Test the parallel equivalence checking flow with only the HSF checker
  // enabled
  const auto c1 =
      qasm3::Importer::importf(testDir + "dj_indep_qiskit_10_no_measure.qasm");
  const auto c2 = qasm3::Importer::importf(
      testDir + "out_dj_indep_qiskit_10_high_error.qasm");
  config.functionality.approximateCheckingThreshold = 0.912;
  config.execution.parallel = true;
  ec::EquivalenceCheckingManager ecm(c1, c2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST_F(HSFTest, vqe10ParallelChecking) {
  // Test the parallel equivalence checking flow with only the HSF checker
  // enabled (parallel checking is enabled per default)
  const auto c1 =
      qasm3::Importer::importf(testDir + "vqe_indep_qiskit_10_no_measure.qasm");
  const auto c2 = qasm3::Importer::importf(
      testDir + "out_vqe_indep_qiskit_10_high_error.qasm");
  config.functionality.approximateCheckingThreshold = 0.998;
  ec::EquivalenceCheckingManager ecm(c1, c2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST_F(HSFTest, ghz10ParallelChecking) {
  // Test the parallel equivalence checking flow where multiple checkers are
  // enabled
  const auto c1 = qasm3::Importer::importf(
      testDir + "ghz_nativegates_ibm_qiskit_opt3_10_no_measure.qasm");
  const auto c2 = qasm3::Importer::importf(
      testDir + "out_ghz_nativegates_ibm_qiskit_opt3_10_high_error.qasm");
  config.functionality.approximateCheckingThreshold = 0.956;
  config.execution.runConstructionChecker = true;
  config.execution.runAlternatingChecker = true;
  config.execution.parallel = true;
  ec::EquivalenceCheckingManager ecm(c1, c2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST_F(HSFTest, ghz10SequentialChecking) {
  // Test the sequential equivalence checking flow where multiple checkers are
  // enabled
  const auto c1 = qasm3::Importer::importf(
      testDir + "ghz_nativegates_ibm_qiskit_opt3_10_no_measure.qasm");
  const auto c2 = qasm3::Importer::importf(
      testDir + "out_ghz_nativegates_ibm_qiskit_opt3_10_high_error.qasm");
  config.functionality.approximateCheckingThreshold = 0.956;
  config.execution.runConstructionChecker = true;
  config.execution.runAlternatingChecker = true;
  config.execution.parallel = false;
  ec::EquivalenceCheckingManager ecm(c1, c2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST_F(HSFTest, graphstate10SequentialChecking) {
  // Test the sequential equivalence checking flow with only the HSF checker
  // enabled
  const auto c1 = qasm3::Importer::importf(
      testDir + "graphstate_indep_qiskit_10_no_measure.qasm");
  const auto c2 = qasm3::Importer::importf(
      testDir + "out_graphstate_indep_qiskit_10_high_error.qasm");
  config.functionality.approximateCheckingThreshold = 0.912;
  config.execution.parallel = false;
  ec::EquivalenceCheckingManager ecm(c1, c2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST_F(HSFTest, qftNativegates10DecisionOverflow) {
  // Test the HSF checker for overflow errors, ensuring that if an
  // `std::overflow_error` occurs, the checker falls back to an alternative
  // checker
  const auto c1 = qasm3::Importer::importf(
      testDir + "check/qft_nativegates_ibm_qiskit_opt3_10_no_measure.qasm");
  const auto c2 = qasm3::Importer::importf(
      testDir + "check/out_qft_nativegates_ibm_qiskit_opt3_10_high_error.qasm");
  config.functionality.approximateCheckingThreshold = 0.999;
  ec::EquivalenceCheckingManager ecm(c1, c2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST_F(HSFTest, qaoaIndep10GatesNotSupported) {
  // Test the HSF checker for invalid argument errors, such as unsupported
  // gates, ensuring that if an `std::invalid_argument` occurs, the checker
  // falls back to an alternative checker
  const auto c1 = qasm3::Importer::importf(
      testDir + "qaoa_indep_qiskit_10_no_measure.qasm");
  const auto c2 = qasm3::Importer::importf(
      testDir + "out_qaoa_indep_qiskit_10_high_error.qasm");
  config.functionality.approximateCheckingThreshold = 0.958;
  ec::EquivalenceCheckingManager ecm(c1, c2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);
}
