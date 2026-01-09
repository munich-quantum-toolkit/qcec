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
#include "checker/dd/simulation/StateType.hpp"
#include "ir/QuantumComputation.hpp"
#include "qasm3/Importer.hpp"

#include <gtest/gtest.h>
#include <iostream>

class SimulationTest : public ::testing::Test {
protected:
  qc::QuantumComputation qcOriginal;
  qc::QuantumComputation qcAlternative;
  ec::Configuration config{};

  void SetUp() override {
    config.execution.runAlternatingChecker = false;
    config.execution.runConstructionChecker = false;
    config.execution.runSimulationChecker = true;
    config.execution.runZXChecker = false;
    config.execution.parallel = false;

    config.simulation.maxSims = 8U;
    config.simulation.seed = 12345U;
  }
};

TEST_F(SimulationTest, Consistency) {
  qcOriginal = qasm3::Importer::importf("./circuits/test/test_original.qasm");
  qcAlternative =
      qasm3::Importer::importf("./circuits/test/test_erroneous.qasm");

  ec::EquivalenceCheckingManager ecm(qcOriginal, qcAlternative, config);
  ecm.run();

  ec::EquivalenceCheckingManager ecm2(qcOriginal, qcAlternative, config);
  ecm2.run();

  EXPECT_EQ(ecm.getResults().performedSimulations,
            ecm2.getResults().performedSimulations);
}

TEST_F(SimulationTest, ClassicalStimuli) {
  qcOriginal = qasm3::Importer::importf("./circuits/test/test_original.qasm");
  qcAlternative =
      qasm3::Importer::importf("./circuits/test/test_alternative.qasm");

  config.simulation.stateType = ec::StateType::ComputationalBasis;
  ec::EquivalenceCheckingManager ecm(qcOriginal, qcAlternative, config);
  ecm.run();
  std::cout << "Configuration:\n" << ecm.getConfiguration() << '\n';
  std::cout << "Results:\n" << ecm.getResults() << '\n';
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());

  qcAlternative =
      qasm3::Importer::importf("./circuits/test/test_erroneous.qasm");
  ec::EquivalenceCheckingManager ecm2(qcOriginal, qcAlternative, config);
  ecm2.run();
  std::cout << "Results (expected non-equivalent):\n"
            << ecm.getResults() << '\n';
  EXPECT_FALSE(ecm2.getResults().consideredEquivalent());
}

TEST_F(SimulationTest, LocalStimuli) {
  qcOriginal = qasm3::Importer::importf("./circuits/test/test_original.qasm");
  qcAlternative =
      qasm3::Importer::importf("./circuits/test/test_alternative.qasm");

  config.simulation.stateType = ec::StateType::Random1QBasis;
  ec::EquivalenceCheckingManager ecm(qcOriginal, qcAlternative, config);
  ecm.run();
  std::cout << "Configuration:\n" << ecm.getConfiguration() << '\n';
  std::cout << "Results:\n" << ecm.getResults() << '\n';
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());

  qcAlternative =
      qasm3::Importer::importf("./circuits/test/test_erroneous.qasm");
  ec::EquivalenceCheckingManager ecm2(qcOriginal, qcAlternative, config);
  ecm2.run();
  std::cout << "Results (expected non-equivalent):\n"
            << ecm.getResults() << '\n';
  EXPECT_FALSE(ecm2.getResults().consideredEquivalent());
}

TEST_F(SimulationTest, GlobalStimuli) {
  qcOriginal = qasm3::Importer::importf("./circuits/test/test_original.qasm");
  qcAlternative =
      qasm3::Importer::importf("./circuits/test/test_alternative.qasm");

  config.simulation.stateType = ec::StateType::Stabilizer;
  ec::EquivalenceCheckingManager ecm(qcOriginal, qcAlternative, config);
  ecm.run();
  std::cout << "Configuration:\n" << ecm.getConfiguration() << '\n';
  std::cout << "Results:\n" << ecm.getResults() << '\n';
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());

  qcAlternative =
      qasm3::Importer::importf("./circuits/test/test_erroneous.qasm");
  ec::EquivalenceCheckingManager ecm2(qcOriginal, qcAlternative, config);
  ecm2.run();
  std::cout << "Results (expected non-equivalent):\n"
            << ecm.getResults() << '\n';
  EXPECT_FALSE(ecm2.getResults().consideredEquivalent());
}

TEST_F(SimulationTest, ClassicalStimuliParallel) {
  config.execution.parallel = true;
  qcOriginal = qasm3::Importer::importf("./circuits/test/test_original.qasm");
  qcAlternative =
      qasm3::Importer::importf("./circuits/test/test_alternative.qasm");

  config.simulation.stateType = ec::StateType::ComputationalBasis;
  ec::EquivalenceCheckingManager ecm(qcOriginal, qcAlternative, config);
  ecm.run();
  std::cout << "Configuration:\n" << ecm.getConfiguration() << '\n';
  std::cout << "Results:\n" << ecm.getResults() << '\n';
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());

  qcAlternative =
      qasm3::Importer::importf("./circuits/test/test_erroneous.qasm");
  ec::EquivalenceCheckingManager ecm2(qcOriginal, qcAlternative, config);
  ecm2.run();
  std::cout << "Results (expected non-equivalent):\n"
            << ecm.getResults() << '\n';
  EXPECT_FALSE(ecm2.getResults().consideredEquivalent());
}

TEST_F(SimulationTest, LocalStimuliParallel) {
  config.execution.parallel = true;
  qcOriginal = qasm3::Importer::importf("./circuits/test/test_original.qasm");
  qcAlternative =
      qasm3::Importer::importf("./circuits/test/test_alternative.qasm");

  config.simulation.stateType = ec::StateType::Random1QBasis;
  ec::EquivalenceCheckingManager ecm(qcOriginal, qcAlternative, config);
  ecm.run();
  std::cout << "Configuration:\n" << ecm.getConfiguration() << '\n';
  std::cout << "Results:\n" << ecm.getResults() << '\n';
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());

  qcAlternative =
      qasm3::Importer::importf("./circuits/test/test_erroneous.qasm");
  ec::EquivalenceCheckingManager ecm2(qcOriginal, qcAlternative, config);
  ecm2.run();
  std::cout << "Results (expected non-equivalent):\n"
            << ecm.getResults() << '\n';
  EXPECT_FALSE(ecm2.getResults().consideredEquivalent());
}

TEST_F(SimulationTest, GlobalStimuliParallel) {
  config.execution.parallel = true;
  qcOriginal = qasm3::Importer::importf("./circuits/test/test_original.qasm");
  qcAlternative =
      qasm3::Importer::importf("./circuits/test/test_alternative.qasm");

  config.simulation.stateType = ec::StateType::Stabilizer;
  ec::EquivalenceCheckingManager ecm(qcOriginal, qcAlternative, config);
  ecm.run();
  std::cout << "Configuration:\n" << ecm.getConfiguration() << '\n';
  std::cout << "Results:\n" << ecm.getResults() << '\n';
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());

  qcAlternative =
      qasm3::Importer::importf("./circuits/test/test_erroneous.qasm");
  ec::EquivalenceCheckingManager ecm2(qcOriginal, qcAlternative, config);
  ecm2.run();
  std::cout << "Results (expected non-equivalent):\n"
            << ecm.getResults() << '\n';
  EXPECT_FALSE(ecm2.getResults().consideredEquivalent());
}

TEST_F(SimulationTest, GlobalStimuliAncillaryQubit) {
  qcOriginal = qc::QuantumComputation(1);
  qcOriginal.addAncillaryRegister(1);
  qcOriginal.x(0);
  qcOriginal.z(1);

  qcAlternative = qc::QuantumComputation(1);
  qcAlternative.addAncillaryRegister(1);
  qcAlternative.x(0);

  config.simulation.stateType = ec::StateType::Stabilizer;
  ec::EquivalenceCheckingManager ecm(qcOriginal, qcAlternative, config);
  ecm.run();
  std::cout << ecm.getResults() << "\n";
  std::cout << ecm.getFirstCircuit() << '\n' << ecm.getSecondCircuit() << '\n';
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}
