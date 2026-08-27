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
#include "checker/dd/TaskManager.hpp"
#include "checker/dd/applicationscheme/ApplicationScheme.hpp"
#include "checker/dd/applicationscheme/GateCostApplicationScheme.hpp"
#include "dd/Package.hpp"
#include "ir/QuantumComputation.hpp"
#include "ir/operations/OpType.hpp"
#include "qasm3/Importer.hpp"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>

namespace ec {
namespace {
[[nodiscard]] std::size_t
legacyCostFunction(const GateCostLookupTableKeyType& key) noexcept {
  const auto [gate, nc] = key;

  if (nc == 0U) {
    switch (gate) {
    case qc::I:
    case qc::H:
    case qc::X:
    case qc::Y:
    case qc::Z:
    case qc::S:
    case qc::Sdg:
    case qc::T:
    case qc::Tdg:
    case qc::P:
    case qc::U2:
    case qc::U:
    case qc::SX:
    case qc::SXdg:
    case qc::V:
    case qc::Vdg:
    case qc::RX:
    case qc::RY:
    case qc::RZ:
    // the following are merely placeholders so that the check can advance
    case qc::Compound:
    case qc::Measure:
    case qc::Reset:
    case qc::Barrier:
    case qc::IfElse:
      return 1U;
    default:
      break;
    }
  }

  // special treatment for CNOT
  if ((gate == qc::X) && (nc == 1U)) {
    return 1U;
  }

  switch (gate) {
  case qc::X:
    return (2UL * (nc - 2UL) *
            ((2UL * legacyCostFunction({qc::P, 0})) +
             (2UL * legacyCostFunction({qc::U2, 0})) +
             (3UL * legacyCostFunction({qc::X, 1})))) +
           (6UL * legacyCostFunction({qc::X, 1})) +
           (8UL * legacyCostFunction({qc::U, 0}));
  case qc::U:
  case qc::U2:
  case qc::V:
  case qc::Vdg:
  case qc::RX:
  case qc::RY:
  case qc::H:
  case qc::SX:
  case qc::SXdg:
    // heuristic
    return (2U * legacyCostFunction({qc::X, nc})) +
           (4U * legacyCostFunction({qc::U, 0}));
  case qc::P:
  case qc::S:
  case qc::Sdg:
  case qc::T:
  case qc::Tdg:
  case qc::RZ:
    // heuristic
    return (2U * legacyCostFunction({qc::X, nc})) +
           (3U * legacyCostFunction({qc::P, 0}));
  case qc::Y:
  case qc::Z:
    return legacyCostFunction({qc::X, nc}) +
           (2U * legacyCostFunction({qc::U, 0}));
  case qc::SWAP:
    return legacyCostFunction({qc::X, nc}) +
           (2U * legacyCostFunction({qc::X, 1}));
  default:
    return 1U;
  }
}
} // namespace

class CompilationFlowTest : public testing::TestWithParam<std::string> {
protected:
  qc::QuantumComputation qcOriginal;
  qc::QuantumComputation qcTranspiled;

  std::string testOriginalDir = "./circuits/original/";
  std::string testTranspiledDir = "./circuits/transpiled/";

  Configuration configuration{};

  void SetUp() override {
    qcOriginal =
        qasm3::Importer::importf(testOriginalDir + GetParam() + ".qasm");
    qcTranspiled = qasm3::Importer::importf(testTranspiledDir + GetParam() +
                                            "_transpiled.qasm");

    configuration.execution.runAlternatingChecker = true;
    configuration.execution.runConstructionChecker = false;
    configuration.execution.runSimulationChecker = false;
    configuration.execution.runZXChecker = false;

    configuration.application.alternatingScheme =
        ApplicationSchemeType::GateCost;
    configuration.application.costFunction = legacyCostFunction;
  }
};

INSTANTIATE_TEST_SUITE_P(
    CompilationFlowTest, CompilationFlowTest,
    testing::Values("dk27_225", "pcler8_248", "5xp1_194", "alu1_198",
                    "mlp4_245", "dk17_224", "add6_196", "C7552_205", "cu_219",
                    "example2_231", "c2_181", "rd73_312", "cm150a_210",
                    "cm163a_213", "c2_182", "sym9_317", "cm151a_211",
                    "apla_203"),
    [](const testing::TestParamInfo<CompilationFlowTest::ParamType>& inf) {
      auto s = inf.param;
      std::ranges::replace(s, '-', '_');
      return s;
    });

TEST_P(CompilationFlowTest, EquivalenceCompilationFlow) {
  EquivalenceCheckingManager ecm(qcOriginal, qcTranspiled, configuration);
  ecm.run();
  std::cout << ecm.getResults() << "\n";
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}

TEST_P(CompilationFlowTest, EquivalenceCompilationFlowParallel) {
  configuration.execution.runSimulationChecker = true;
  configuration.execution.parallel = true;

  EquivalenceCheckingManager ecm(qcOriginal, qcTranspiled, configuration);
  ecm.run();
  std::cout << ecm.getResults() << "\n";
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}

TEST(CompilationFlowTest, CompilationFlowProfileFromFile) {
  // clang-tidy is having an aneurysm here and suggests to use const for some of
  // the variables, which does not make sense in this context.
  // NOLINTBEGIN(misc-const-correctness)
  // create a simple profile that specifies a Toffoli corresponds to 15 gates.
  const std::string filename = "simple.profile";
  std::ofstream ofs(filename);
  ofs << "x 2 15\n";
  ofs.close();

  // apply Toffoli gate
  auto qc = qc::QuantumComputation(3, 3);
  qc.mcx({1, 2}, 0);

  auto dd = std::make_unique<dd::Package>(3);
  auto tm = TaskManager<dd::MatrixDD>(qc, *dd);

  auto scheme = GateCostApplicationScheme(tm, tm, filename, false);

  const auto [left, right] = scheme();

  EXPECT_EQ(left, 1U);
  EXPECT_EQ(right, 15U);

  Configuration config{};
  config.application.profile = filename;
  config.application.alternatingScheme = ApplicationSchemeType::GateCost;
  EquivalenceCheckingManager ecm(qc, qc, config);
  ecm.run();
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
  std::cout << ecm.getResults() << "\n";

  const auto json = ecm.getConfiguration().json();
  EXPECT_EQ(json["application"]["profile"], filename);
  EXPECT_EQ(json["application"]["alternating"],
            toString(ApplicationSchemeType::GateCost));

  ecm.reset();
  ecm.getConfiguration().application.profile = "";
  ecm.run();
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());

  const auto json2 = ecm.getConfiguration().json();
  EXPECT_EQ(json2["application"]["profile"], "cost_function");
  EXPECT_EQ(json["application"]["alternating"],
            toString(ApplicationSchemeType::GateCost));
  // NOLINTEND(misc-const-correctness)
}

} // namespace ec
