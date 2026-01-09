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

namespace ec {
bool Configuration::anythingToExecute() const noexcept {
  return (execution.runSimulationChecker && simulation.maxSims > 0U) ||
         execution.runAlternatingChecker || execution.runConstructionChecker ||
         execution.runZXChecker;
}

bool Configuration::onlySingleTask() const noexcept {
  // only a single simulation shall be performed
  if (execution.runSimulationChecker && (simulation.maxSims == 1U) &&
      !execution.runAlternatingChecker && !execution.runConstructionChecker &&
      !execution.runZXChecker) {
    return true;
  }

  // no simulations and only one of the other checks shall be performed
  if (!execution.runSimulationChecker &&
      ((execution.runAlternatingChecker && !execution.runConstructionChecker &&
        !execution.runZXChecker) ||
       (!execution.runAlternatingChecker && execution.runConstructionChecker &&
        !execution.runZXChecker) ||
       (!execution.runAlternatingChecker && !execution.runConstructionChecker &&
        execution.runZXChecker))) {
    return true;
  }

  return false;
}

bool Configuration::onlyZXCheckerConfigured() const noexcept {
  return !execution.runConstructionChecker && !execution.runSimulationChecker &&
         !execution.runAlternatingChecker && execution.runZXChecker;
}

bool Configuration::onlySimulationCheckerConfigured() const noexcept {
  return !execution.runConstructionChecker && execution.runSimulationChecker &&
         !execution.runAlternatingChecker && !execution.runZXChecker;
}

nlohmann::basic_json<> Configuration::json() const {
  nlohmann::basic_json<> config{};
  auto& exe = config["execution"];
  exe["tolerance"] = execution.numericalTolerance;
  exe["parallel"] = execution.parallel;
  exe["nthreads"] = execution.nthreads;
  exe["run_construction_checker"] = execution.runConstructionChecker;
  exe["run_simulation_checker"] = execution.runSimulationChecker;
  exe["run_alternating_checker"] = execution.runAlternatingChecker;
  exe["run_zx_checker"] = execution.runZXChecker;
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
  fun["check_partial_equivalence"] = functionality.checkPartialEquivalence;

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
