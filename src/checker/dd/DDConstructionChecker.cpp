/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "checker/dd/DDConstructionChecker.hpp"

#include "Configuration.hpp"
#include "checker/dd/DDEquivalenceChecker.hpp"
#include "checker/dd/DDPackageConfigs.hpp"
#include "checker/dd/TaskManager.hpp"
#include "checker/dd/applicationscheme/ApplicationScheme.hpp"
#include "dd/Node.hpp"
#include "dd/Package.hpp"
#include "ir/QuantumComputation.hpp"

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <utility>

ec::DDConstructionChecker::DDConstructionChecker(
    const qc::QuantumComputation& circ1, const qc::QuantumComputation& circ2,
    Configuration config)
    : DDEquivalenceChecker(circ1, circ2, std::move(config),
                           ConstructionDDPackageConfig{}) {
  if (configuration.application.constructionScheme ==
      ApplicationSchemeType::Lookahead) {
    throw std::invalid_argument("Lookahead application scheme must not be "
                                "used with DD construction checker.");
  }
  initializeApplicationScheme(configuration.application.constructionScheme);
}

// this function is mainly placed here in order to have an out-of-line
// virtual method definition which avoids emitting the classes' vtable in
// every translation unit.
void ec::DDConstructionChecker::json(nlohmann::basic_json<>& j) const noexcept {
  DDEquivalenceChecker::json(j);
  j["checker"] = "decision_diagram_construction";
}

void ec::DDConstructionChecker::initializeTask(
    TaskManager<dd::MatrixDD>& taskManager) {
  DDEquivalenceChecker::initializeTask(taskManager);
  const auto initial = dd::Package::makeIdent();
  taskManager.setInternalState(initial);
  taskManager.incRef();
  taskManager.reduceAncillae();
}
