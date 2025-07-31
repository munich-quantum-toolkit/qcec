/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "Configuration.hpp"
#include "EquivalenceCheckingManager.hpp"
#include "EquivalenceCriterion.hpp"
#include "checker/dd/applicationscheme/ApplicationScheme.hpp"
#include "ir/QuantumComputation.hpp"
#include "qasm3/Importer.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <iostream>
#include <iterator>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <tuple>

class JournalTestNonEQ
    : public testing::TestWithParam<std::tuple<std::string, std::uint16_t>> {
protected:
  qc::QuantumComputation qcOriginal;
  qc::QuantumComputation qcTranspiled;
  ec::Configuration config;

  std::string testOriginalDir = "./circuits/original/";
  std::string testTranspiledDir = "./circuits/transpiled/";

  std::uint16_t tries = 10U;

  double minTime = 0.;
  double maxTime = 0.;
  double avgTime = 0.;
  std::size_t minSims = 0U;
  std::size_t maxSims = 0U;
  double avgSims = 0.;

  std::string transpiledFile;

  std::mt19937_64 mt;
  std::uniform_int_distribution<std::uint64_t> distribution;
  std::function<std::uint64_t()> rng;

  std::uint16_t gatesToRemove = 0U;
  std::set<std::set<std::uint64_t>> alreadyRemoved;

  std::uint16_t successes = 0U;

  static bool setExists(const std::set<std::set<std::uint64_t>>& all,
                        const std::set<std::uint64_t>& test) {
    // first entry
    if (all.empty()) {
      return false;
    }

    for (const auto& set : all) {
      bool equals = true;
      for (auto setit = set.begin(), testit = test.begin(); setit != set.end();
           ++setit, ++testit) {
        if (*setit != *testit) {
          equals = false;
          break;
        }
      }
      if (equals) {
        return true;
      }
    }
    return false;
  }

  void SetUp() override {
    std::stringstream ss{};
    ss << testTranspiledDir << std::get<0>(GetParam()) << "_transpiled.qasm";
    transpiledFile = ss.str();

    std::array<std::mt19937_64::result_type, std::mt19937_64::state_size>
        randomData{};
    std::random_device rd;
    std::ranges::generate(randomData, [&]() { return rd(); });
    std::seed_seq seeds(begin(randomData), end(randomData));
    mt.seed(seeds);
    distribution = decltype(distribution)(0U, qcTranspiled.getNops() - 1U);
    rng = [this]() { return distribution(mt); };

    gatesToRemove = std::get<1>(GetParam());

    minTime = 0.;
    maxTime = 0.;
    avgTime = 0.;
    minSims = 0U;
    maxSims = 0U;
    avgSims = 0.;
  }

  void addToStatistics(std::uint16_t tryCount, double time, std::size_t nsims) {
    if (tryCount == 0U) {
      minTime = time;
      maxTime = time;
      avgTime = time;
      minSims = nsims;
      maxSims = nsims;
      avgSims = static_cast<double>(nsims);
    } else {
      minTime = std::min(minTime, time);
      minSims = std::min(minSims, nsims);
      maxTime = std::max(maxTime, time);
      maxSims = std::max(maxSims, nsims);
      avgTime =
          (avgTime * tryCount + time) / static_cast<double>(tryCount + 1U);
      avgSims = (avgSims * tryCount + static_cast<double>(nsims)) /
                static_cast<double>(tryCount + 1U);
    }
  }
};

INSTANTIATE_TEST_SUITE_P(
    Journal, JournalTestNonEQ,
    testing::Combine(
        testing::Values(
            "dk27_225", "pcler8_248", "5xp1_194", "alu1_198",
            //				                         "mlp4_245",
            "dk17_224",
            //				                         "add6_196",
            //				                         "C7552_205",
            "cu_219",
            //				                         "example2_231",
            "c2_181", "rd73_312",
            //				                         "cm150a_210",
            "cm163a_213", "c2_182", "sym9_317", "mod5adder_306", "rd84_313"
            //				                         "cm151a_211",
            //				                         "apla_203"
            ),
        testing::Range(static_cast<unsigned short>(1U),
                       static_cast<unsigned short>(4U), 2)),
    [](const testing::TestParamInfo<JournalTestNonEQ::ParamType>& inf) {
      std::string name = std::get<0>(inf.param);
      const unsigned short gatesToRemove = std::get<1>(inf.param);
      std::ranges::replace(name, '-', '_');
      std::stringstream ss{};
      ss << name << "_removed_" << gatesToRemove;
      return ss.str();
    });

TEST_P(JournalTestNonEQ, PowerOfSimulation) {
  config.execution.runAlternatingChecker = false;
  config.execution.runConstructionChecker = false;
  config.execution.runSimulationChecker = true;
  config.execution.runZXChecker = false;
  config.execution.parallel = false;
  config.execution.timeout = 60.;
  config.simulation.maxSims = 16U;
  config.functionality.checkPartialEquivalence = true;

  for (std::uint16_t i = 0U; i < tries; ++i) {
    qcOriginal = qasm3::Importer::importf(testOriginalDir +
                                          std::get<0>(GetParam()) + ".qasm");

    // generate non-eq circuit
    std::set<std::uint64_t> removed{};
    do { // NOLINT(cppcoreguidelines-avoid-do-while)
      qcTranspiled = qasm3::Importer::importf(transpiledFile);
      removed.clear();
      for (std::uint16_t j = 0U; j < gatesToRemove; ++j) {
        auto gateToRemove = rng() % qcTranspiled.getNops();
        while (removed.contains(gateToRemove)) {
          gateToRemove = rng() % qcTranspiled.getNops();
        }
        removed.insert(gateToRemove);
        auto it = qcTranspiled.begin();
        std::advance(it, gateToRemove);
        if (it == qcTranspiled.end()) {
          continue;
        }

        qcTranspiled.erase(it);
      }
    } while (setExists(alreadyRemoved, removed));
    alreadyRemoved.insert(removed);
    ec::EquivalenceCheckingManager ecm(qcOriginal, qcTranspiled, config);
    ecm.run();
    auto results = ecm.getResults();
    std::cout << "[" << i << "] ";
    std::cout << toString(results.equivalence) << '\n';
    addToStatistics(i, results.checkTime + results.preprocessingTime,
                    results.performedSimulations);
    if (results.equivalence == ec::EquivalenceCriterion::NotEquivalent) {
      successes++;
    }
  }
  std::cout << qcOriginal.getName() << ";" << qcOriginal.getNqubits() << ";"
            << qcOriginal.getNops() << ";" << qcTranspiled.getNops() << ";"
            << tries << ";" << minSims << ";" << maxSims << ";" << avgSims
            << ";" << minTime << ";" << maxTime << ";" << avgTime << ";"
            << static_cast<double>(successes) / static_cast<double>(tries)
            << ";" << '\n';
}

TEST_P(JournalTestNonEQ, PowerOfSimulationParallel) {
  config.execution.runAlternatingChecker = false;
  config.execution.runConstructionChecker = false;
  config.execution.runZXChecker = false;
  config.execution.runSimulationChecker = true;
  config.execution.parallel = true;
  config.execution.timeout = 60.;
  config.simulation.maxSims = 16U;
  config.functionality.checkPartialEquivalence = true;

  for (std::uint16_t i = 0; i < tries; ++i) {
    qcOriginal = qasm3::Importer::importf(testOriginalDir +
                                          std::get<0>(GetParam()) + ".qasm");

    // generate non-eq circuit
    std::set<std::uint64_t> removed{};
    do { // NOLINT(cppcoreguidelines-avoid-do-while)
      qcTranspiled = qasm3::Importer::importf(transpiledFile);
      removed.clear();
      for (std::uint16_t j = 0U; j < gatesToRemove; ++j) {
        auto gateToRemove = rng() % qcTranspiled.getNops();
        while (removed.contains(gateToRemove)) {
          gateToRemove = rng() % qcTranspiled.getNops();
        }
        removed.insert(gateToRemove);
        auto it = qcTranspiled.begin();
        std::advance(it, gateToRemove);
        if (it == qcTranspiled.end()) {
          continue;
        }

        qcTranspiled.erase(it);
      }
    } while (setExists(alreadyRemoved, removed));
    alreadyRemoved.insert(removed);
    ec::EquivalenceCheckingManager ecm(qcOriginal, qcTranspiled, config);
    ecm.run();
    auto results = ecm.getResults();
    std::cout << "[" << i << "] ";
    std::cout << toString(results.equivalence) << '\n';
    addToStatistics(i, results.checkTime + results.preprocessingTime,
                    results.performedSimulations);
    if (results.equivalence == ec::EquivalenceCriterion::NotEquivalent) {
      successes++;
    }
  }
  std::cout << qcOriginal.getName() << ";" << qcOriginal.getNqubits() << ";"
            << qcOriginal.getNops() << ";" << qcTranspiled.getNops() << ";"
            << tries << ";" << minSims << ";" << maxSims << ";" << avgSims
            << ";" << minTime << ";" << maxTime << ";" << avgTime << ";"
            << static_cast<double>(successes) / static_cast<double>(tries)
            << ";" << '\n';
}

class JournalTestEQ : public testing::TestWithParam<std::string> {
protected:
  qc::QuantumComputation qcOriginal;
  qc::QuantumComputation qcTranspiled;
  ec::Configuration config{};

  std::string testOriginalDir = "./circuits/original/";
  std::string testTranspiledDir = "./circuits/transpiled/";

  std::string transpiledFile;

  void SetUp() override {
    config.execution.parallel = false;
    config.execution.runConstructionChecker = false;
    config.execution.runAlternatingChecker = false;
    config.execution.runZXChecker = false;
    config.execution.runSimulationChecker = false;
    config.simulation.maxSims = 16;
    config.application.simulationScheme = ec::ApplicationSchemeType::OneToOne;
    config.execution.timeout = 60.;

    std::stringstream ss{};
    ss << testTranspiledDir << GetParam() << "_transpiled.qasm";
    transpiledFile = ss.str();

    qcOriginal =
        qasm3::Importer::importf(testOriginalDir + GetParam() + ".qasm");
    qcTranspiled = qasm3::Importer::importf(transpiledFile);
  }
};

INSTANTIATE_TEST_SUITE_P(
    Journal, JournalTestEQ,
    testing::Values("dk27_225", "pcler8_248", "5xp1_194", "alu1_198"
                    //"mlp4_245"
                    //"dk17_224",
                    //"add6_196",
                    //"C7552_205",
                    //"cu_219",
                    //"example2_231",
                    //"c2_181",
                    //"rd73_312",
                    //"cm150a_210",
                    //"cm163a_213",
                    //"c2_182",
                    //"sym9_317",
                    //"mod5adder_306",
                    //"rd84_313",
                    //"cm151a_211",
                    //"apla_203"
                    ),
    [](const testing::TestParamInfo<JournalTestEQ::ParamType>& inf) {
      std::string name = inf.param;
      std::ranges::replace(name, '-', '_');
      std::stringstream ss{};
      ss << name;
      return ss.str();
    });

TEST_P(JournalTestEQ, EQReference) {
  config.execution.runConstructionChecker = true;
  config.application.constructionScheme = ec::ApplicationSchemeType::OneToOne;

  ec::EquivalenceCheckingManager ecm(qcOriginal, qcTranspiled, config);
  ecm.run();
  std::cout << "Configuration:\n" << ecm.getConfiguration() << '\n';
  std::cout << "Results:\n" << ecm.getResults() << '\n';
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}

TEST_P(JournalTestEQ, EQNaive) {
  config.execution.runAlternatingChecker = true;
  config.application.alternatingScheme = ec::ApplicationSchemeType::OneToOne;

  ec::EquivalenceCheckingManager ecm(qcOriginal, qcTranspiled, config);
  ecm.run();
  std::cout << "Configuration:\n" << ecm.getConfiguration() << '\n';
  std::cout << "Results:\n" << ecm.getResults() << '\n';
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}

TEST_P(JournalTestEQ, EQProportional) {
  config.execution.runAlternatingChecker = true;
  config.application.alternatingScheme =
      ec::ApplicationSchemeType::Proportional;

  ec::EquivalenceCheckingManager ecm(qcOriginal, qcTranspiled, config);
  ecm.run();
  std::cout << "Configuration:\n" << ecm.getConfiguration() << '\n';
  std::cout << "Results:\n" << ecm.getResults() << '\n';
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}

TEST_P(JournalTestEQ, EQLookahead) {
  config.execution.runAlternatingChecker = true;
  config.application.alternatingScheme = ec::ApplicationSchemeType::Lookahead;

  ec::EquivalenceCheckingManager ecm(qcOriginal, qcTranspiled, config);
  ecm.run();
  std::cout << "Configuration:\n" << ecm.getConfiguration() << '\n';
  std::cout << "Results:\n" << ecm.getResults() << '\n';
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}

TEST_P(JournalTestEQ, EQPowerOfSimulation) {
  config.execution.runSimulationChecker = true;

  ec::EquivalenceCheckingManager ecm(qcOriginal, qcTranspiled, config);
  ecm.run();
  std::cout << "Configuration:\n" << ecm.getConfiguration() << '\n';
  std::cout << "Results:\n" << ecm.getResults() << '\n';
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}

TEST_P(JournalTestEQ, EQReferenceParallel) {
  config.execution.parallel = true;
  config.execution.runConstructionChecker = true;
  config.execution.runSimulationChecker = true; // additionally run simulations
  config.application.constructionScheme = ec::ApplicationSchemeType::OneToOne;

  ec::EquivalenceCheckingManager ecm(qcOriginal, qcTranspiled, config);
  ecm.run();
  std::cout << "Configuration:\n" << ecm.getConfiguration() << '\n';
  std::cout << "Results:\n" << ecm.getResults() << '\n';
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}

TEST_P(JournalTestEQ, EQNaiveParallel) {
  config.execution.parallel = true;
  config.execution.runAlternatingChecker = true;
  config.execution.runSimulationChecker = true; // additionally run simulations
  config.application.alternatingScheme = ec::ApplicationSchemeType::OneToOne;

  ec::EquivalenceCheckingManager ecm(qcOriginal, qcTranspiled, config);
  ecm.run();
  std::cout << "Configuration:\n" << ecm.getConfiguration() << '\n';
  std::cout << "Results:\n" << ecm.getResults() << '\n';
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}

TEST_P(JournalTestEQ, EQProportionalParallel) {
  config.execution.parallel = true;
  config.execution.runAlternatingChecker = true;
  config.execution.runSimulationChecker = true; // additionally run simulations
  config.application.alternatingScheme =
      ec::ApplicationSchemeType::Proportional;

  ec::EquivalenceCheckingManager ecm(qcOriginal, qcTranspiled, config);
  ecm.run();
  std::cout << "Configuration:\n" << ecm.getConfiguration() << '\n';
  std::cout << "Results:\n" << ecm.getResults() << '\n';
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}

TEST_P(JournalTestEQ, EQLookaheadParallel) {
  config.execution.parallel = true;
  config.execution.runAlternatingChecker = true;
  config.execution.runSimulationChecker = true; // additionally run simulations
  config.application.alternatingScheme = ec::ApplicationSchemeType::Lookahead;

  ec::EquivalenceCheckingManager ecm(qcOriginal, qcTranspiled, config);
  ecm.run();
  std::cout << "Configuration:\n" << ecm.getConfiguration() << '\n';
  std::cout << "Results:\n" << ecm.getResults() << '\n';
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}

TEST_P(JournalTestEQ, EQPowerOfSimulationParallel) {
  config.execution.parallel = true;
  config.execution.runSimulationChecker = true;

  ec::EquivalenceCheckingManager ecm(qcOriginal, qcTranspiled, config);
  ecm.run();
  std::cout << "Configuration:\n" << ecm.getConfiguration() << '\n';
  std::cout << "Results:\n" << ecm.getResults() << '\n';
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());
}
