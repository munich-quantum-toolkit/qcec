/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "checker/dd/DDSimulationChecker.hpp"

#include "Configuration.hpp"
#include "EquivalenceCriterion.hpp"
#include "checker/dd/DDEquivalenceChecker.hpp"
#include "checker/dd/DDPackageConfigs.hpp"
#include "checker/dd/TaskManager.hpp"
#include "checker/dd/simulation/StateGenerator.hpp"
#include "dd/StateGeneration.hpp"
#include "ir/QuantumComputation.hpp"

#include <nlohmann/json.hpp>
#include <utility>

namespace ec {
DDSimulationChecker::DDSimulationChecker(const qc::QuantumComputation& circ1,
                                         const qc::QuantumComputation& circ2,
                                         Configuration config)
    : DDEquivalenceChecker(circ1, circ2, std::move(config),
                           SimulationDDPackageConfig{}) {
  initialState = dd::makeZeroState(nqubits, *dd);
  initializeApplicationScheme(configuration.application.simulationScheme);
}

void DDSimulationChecker::initializeTask(
    TaskManager<dd::VectorDD>& taskManager) {
  DDEquivalenceChecker::initializeTask(taskManager);
  taskManager.setInternalState(initialState);
  taskManager.incRef();
}

EquivalenceCriterion DDSimulationChecker::checkEquivalence() {
  equivalence = DDEquivalenceChecker::checkEquivalence();

  // adjust reference counts to facilitate reuse of the simulation checker
  taskManager1.decRef();
  taskManager2.decRef();
  dd->decRef(initialState);

  return equivalence;
}

void DDSimulationChecker::setRandomInitialState(StateGenerator& generator) {
  const auto nancillary = nqubits - qc1->getNqubitsWithoutAncillae();
  const auto stateType = configuration.simulation.stateType;

  initialState =
      generator.generateRandomState(*dd, nqubits, nancillary, stateType);
}

void DDSimulationChecker::json(nlohmann::basic_json<>& j) const noexcept {
  DDEquivalenceChecker::json(j);
  j["checker"] = "decision_diagram_simulation";
}

} // namespace ec
