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

#include "checker/dd/applicationscheme/ApplicationScheme.hpp"
#include "checker/dd/simulation/StateType.hpp"

#include <nlohmann/json.hpp>
#include <ostream>
#include <string_view>

namespace ec {
bool Configuration::shouldRunChecker(
    const std::string_view checker) const noexcept {
  if (execution.method != "auto") {
    return execution.method == checker;
  }
  if (checker == "construction") {
    return execution.runConstructionChecker;
  }
  if (checker == "simulation") {
    return execution.runSimulationChecker;
  }
  if (checker == "alternating") {
    return execution.runAlternatingChecker;
  }
  if (checker == "zx") {
    return execution.runZXChecker;
  }
  return checker == "hsf" && execution.runHSFChecker;
}

bool Configuration::anythingToExecute() const noexcept {
  return (shouldRunChecker("simulation") && simulation.maxSims > 0U) ||
         shouldRunChecker("alternating") || shouldRunChecker("construction") ||
         shouldRunChecker("zx") || shouldRunChecker("hsf");
}

bool Configuration::onlySingleTask() const noexcept {
  const auto nonSimulationTasks =
      static_cast<unsigned>(shouldRunChecker("alternating")) +
      static_cast<unsigned>(shouldRunChecker("construction")) +
      static_cast<unsigned>(shouldRunChecker("zx")) +
      static_cast<unsigned>(shouldRunChecker("hsf"));
  const auto simulations =
      shouldRunChecker("simulation") ? simulation.maxSims : 0U;
  return (nonSimulationTasks == 1U && simulations == 0U) ||
         (nonSimulationTasks == 0U && simulations == 1U);
}

bool Configuration::onlyZXCheckerConfigured() const noexcept {
  return !shouldRunChecker("construction") && !shouldRunChecker("simulation") &&
         !shouldRunChecker("alternating") && shouldRunChecker("zx") &&
         !shouldRunChecker("hsf");
}

bool Configuration::onlySimulationCheckerConfigured() const noexcept {
  return !shouldRunChecker("construction") && shouldRunChecker("simulation") &&
         !shouldRunChecker("alternating") && !shouldRunChecker("zx") &&
         !shouldRunChecker("hsf");
}

nlohmann::basic_json<> Configuration::json() const {
  nlohmann::basic_json<> config{};
  auto& exe = config["execution"];
  exe["method"] = execution.method;
  exe["tolerance"] = execution.numericalTolerance;
  exe["parallel"] = execution.parallel;
  exe["nthreads"] = execution.nthreads;
  exe["run_construction_checker"] = execution.runConstructionChecker;
  exe["run_simulation_checker"] = execution.runSimulationChecker;
  exe["run_alternating_checker"] = execution.runAlternatingChecker;
  exe["run_zx_checker"] = execution.runZXChecker;
  exe["run_hsf_checker"] = execution.runHSFChecker;
  exe["timeout"] = execution.timeout;

  auto& opt = config["optimizations"];
  opt["fuse_consecutive_single_qubit_gates"] =
      optimizations.fuseSingleQubitGates;
  opt["reconstruct_swaps"] = optimizations.reconstructSWAPs;
  opt["remove_diagonal_gates_before_measure"] =
      optimizations.removeDiagonalGatesBeforeMeasure;
  opt["transform_dynamic_circuit"] = optimizations.transformDynamicCircuit;
  opt["reorder_operations"] = optimizations.reorderOperations;
  opt["backpropagate_output_permutation"] =
      optimizations.backpropagateOutputPermutation;
  opt["elide_permutations"] = optimizations.elidePermutations;

  auto& app = config["application"];
  app["construction"] = ec::toString(application.constructionScheme);
  app["simulation"] = ec::toString(application.simulationScheme);
  app["alternating"] = ec::toString(application.alternatingScheme);
  if (!application.profile.empty()) {
    app["profile"] = application.profile;
  } else {
    app["profile"] = "cost_function";
  }

  auto& par = config["parameterized"];
  par["tolerance"] = parameterized.parameterizedTol;
  par["additional_instantiations"] = parameterized.nAdditionalInstantiations;

  auto& fun = config["functionality"];
  fun["trace_threshold"] = functionality.traceThreshold;
  fun["approximate_checking_threshold"] =
      functionality.approximateCheckingThreshold;
  fun["check_partial_equivalence"] = functionality.checkPartialEquivalence;
  fun["check_approximate_equivalence"] =
      functionality.checkApproximateEquivalence;

  auto& sim = config["simulation"];
  sim["fidelity_threshold"] = simulation.fidelityThreshold;
  sim["max_sims"] = simulation.maxSims;
  sim["state_type"] = ec::toString(simulation.stateType);
  sim["seed"] = simulation.seed;

  return config;
}

std::string Configuration::toString() const {
  constexpr auto indent = 2;
  return json().dump(indent);
}

std::ostream& operator<<(std::ostream& os, const Configuration& config) {
  return os << config.toString();
}
} // namespace ec
