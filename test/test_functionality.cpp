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
#include "checker/dd/applicationscheme/ApplicationScheme.hpp"
#include "checker/dd/simulation/StateType.hpp"
#include "ir/QuantumComputation.hpp"
#include "qasm3/Importer.hpp"

#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

class FunctionalityTest : public testing::TestWithParam<std::string> {
protected:
  qc::QuantumComputation qcOriginal;
  qc::QuantumComputation qcAlternative;
  ec::Configuration config{};

  std::string testOriginal = "./circuits/test/test.qasm";
  std::string testAlternativeDir = "./circuits/test/";

  void SetUp() override {
    qcOriginal = qasm3::Importer::importf(testOriginal);
    if (::testing::UnitTest::GetInstance()
            ->current_test_info()
            ->value_param() != nullptr) {
      qcAlternative = qasm3::Importer::importf(testAlternativeDir + "test_" +
                                               GetParam() + ".qasm");
    }

    config.execution.parallel = false;
    config.execution.runConstructionChecker = false;
    config.execution.runAlternatingChecker = false;
    config.execution.runSimulationChecker = false;
    config.execution.runZXChecker = false;
    config.simulation.maxSims = 16;
    config.application.constructionScheme =
        ec::ApplicationSchemeType::Sequential;
    config.application.simulationScheme = ec::ApplicationSchemeType::Sequential;
    config.application.alternatingScheme =
        ec::ApplicationSchemeType::Sequential;
  }
};

INSTANTIATE_TEST_SUITE_P(
    TestCircuits, FunctionalityTest,
    testing::Values("inputperm", "ancilla", "ancilla_inputperm", "swap",
                    "outputperm", "ancilla_inputperm_outputperm",
                    "optimizedswap",
                    "ancilla_inputperm_outputperm_optimizedswap",
                    "ancilla_inputperm_outputperm_optimizedswap2"),
    [](const testing::TestParamInfo<FunctionalityTest::ParamType>& inf) {
      std::stringstream ss{};
      ss << inf.param;
      return ss.str();
    });

TEST_P(FunctionalityTest, Reference) {
  config.execution.runConstructionChecker = true;

  ec::EquivalenceCheckingManager ecm(qcOriginal, qcAlternative, config);
  ecm.run();
  std::cout << ecm.getResults() << "\n";
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}

TEST_P(FunctionalityTest, Proportional) {
  config.execution.runAlternatingChecker = true;
  config.application.alternatingScheme =
      ec::ApplicationSchemeType::Proportional;

  ec::EquivalenceCheckingManager ecm(qcOriginal, qcAlternative, config);
  ecm.run();
  std::cout << ecm.getResults() << "\n";
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}

TEST_P(FunctionalityTest, Lookahead) {
  config.execution.runAlternatingChecker = true;
  config.application.alternatingScheme = ec::ApplicationSchemeType::Lookahead;

  ec::EquivalenceCheckingManager ecm(qcOriginal, qcAlternative, config);
  ecm.run();
  std::cout << ecm.getResults() << "\n";
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}

TEST_P(FunctionalityTest, Naive) {
  config.execution.runAlternatingChecker = true;
  config.application.alternatingScheme = ec::ApplicationSchemeType::OneToOne;

  ec::EquivalenceCheckingManager ecm(qcOriginal, qcAlternative, config);
  ecm.run();
  std::cout << ecm.getResults() << "\n";
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}

TEST_P(FunctionalityTest, Simulation) {
  config.execution.runSimulationChecker = true;

  ec::EquivalenceCheckingManager ecm(qcOriginal, qcAlternative, config);
  ecm.run();
  std::cout << ecm.getResults() << "\n";
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}

TEST_P(FunctionalityTest, SimulationRandom1QBasis) {
  config.execution.runSimulationChecker = true;
  config.simulation.stateType = ec::StateType::Random1QBasis;

  ec::EquivalenceCheckingManager ecm(qcOriginal, qcAlternative, config);
  ecm.run();
  std::cout << ecm.getResults() << "\n";
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}

TEST_P(FunctionalityTest, SimulationStabilizer) {
  config.execution.runSimulationChecker = true;
  config.simulation.stateType = ec::StateType::Stabilizer;

  ec::EquivalenceCheckingManager ecm(qcOriginal, qcAlternative, config);
  ecm.run();
  std::cout << ecm.getResults() << "\n";
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}

TEST_P(FunctionalityTest, SimulationParallel) {
  config.execution.runSimulationChecker = true;
  config.execution.parallel = true;
  config.execution.nthreads = std::thread::hardware_concurrency() + 1U;
  config.simulation.maxSims = std::thread::hardware_concurrency() + 1U;

  ec::EquivalenceCheckingManager ecm(qcOriginal, qcAlternative, config);
  ecm.run();
  std::cout << ecm.getResults() << "\n";
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}

TEST_F(FunctionalityTest, test2) {
  testOriginal = "./circuits/test/test2.qasm";
  qcOriginal = qasm3::Importer::importf(testOriginal);
  qcAlternative =
      qasm3::Importer::importf(testAlternativeDir + "test2_optimized.qasm");

  config.execution.runConstructionChecker = true;

  ec::EquivalenceCheckingManager ecm(qcOriginal, qcAlternative, config);
  ecm.run();
  std::cout << ecm.getResults() << "\n";
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());

  config.execution.runConstructionChecker = false;
  config.execution.runAlternatingChecker = true;
  config.application.alternatingScheme =
      ec::ApplicationSchemeType::Proportional;

  ec::EquivalenceCheckingManager ecm2(qcOriginal, qcAlternative, config);
  ecm2.run();
  std::cout << ecm2.getResults() << "\n";
  EXPECT_TRUE(ecm2.getResults().consideredEquivalent());

  config.application.alternatingScheme = ec::ApplicationSchemeType::Lookahead;

  ec::EquivalenceCheckingManager ecm3(qcOriginal, qcAlternative, config);
  ecm3.run();
  std::cout << ecm3.getResults() << "\n";
  EXPECT_TRUE(ecm3.getResults().consideredEquivalent());

  config.application.alternatingScheme = ec::ApplicationSchemeType::OneToOne;

  ec::EquivalenceCheckingManager ecm4(qcOriginal, qcAlternative, config);
  ecm4.run();
  std::cout << ecm4.getResults() << "\n";
  EXPECT_TRUE(ecm4.getResults().consideredEquivalent());
}
