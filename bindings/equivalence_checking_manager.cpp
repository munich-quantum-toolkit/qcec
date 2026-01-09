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

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>      // NOLINT(misc-include-cleaner)
#include <nanobind/stl/string_view.h> // NOLINT(misc-include-cleaner)
#include <nlohmann/json.hpp>

namespace ec {

namespace nb = nanobind;
using namespace nb::literals;

// NOLINTNEXTLINE(misc-use-internal-linkage)
void registerEquivalenceCheckingManager(const nb::module_& m) {
  // Class definitions
  auto ecm =
      nb::class_<EquivalenceCheckingManager>(m, "EquivalenceCheckingManager");
  auto results =
      nb::class_<EquivalenceCheckingManager::Results>(ecm, "Results");

  // Constructors
  ecm.def(nb::init<const qc::QuantumComputation&, const qc::QuantumComputation&,
                   Configuration>(),
          "circ1"_a, "circ2"_a, "config"_a = Configuration());

  // Access to circuits
  ecm.def_prop_ro("qc1", &EquivalenceCheckingManager::getFirstCircuit);
  ecm.def_prop_ro("qc2", &EquivalenceCheckingManager::getSecondCircuit);

  // Access to configuration
  ecm.def_prop_rw(
      "configuration", &EquivalenceCheckingManager::getConfiguration,
      [](EquivalenceCheckingManager& manager, const Configuration& config) {
        manager.getConfiguration() = config;
      });

  // Run
  ecm.def("run", &EquivalenceCheckingManager::run);

  // Results
  ecm.def_prop_ro("results", &EquivalenceCheckingManager::getResults);
  ecm.def_prop_ro("equivalence", &EquivalenceCheckingManager::equivalence);

  // Convenience functions
  // Execution
  ecm.def("disable_all_checkers",
          &EquivalenceCheckingManager::disableAllCheckers)
      // Application
      .def("set_application_scheme",
           &EquivalenceCheckingManager::setApplicationScheme,
           "scheme"_a = "proportional")
      .def("set_gate_cost_profile",
           &EquivalenceCheckingManager::setGateCostProfile, "profile"_a = "")
      .def("__repr__", [](const EquivalenceCheckingManager& manager) {
        return "<EquivalenceCheckingManager: " +
               toString(manager.equivalence()) + ">";
      });

  // EquivalenceCheckingManager::Results bindings
  results.def(nb::init<>())
      .def_rw("preprocessing_time",
              &EquivalenceCheckingManager::Results::preprocessingTime)
      .def_rw("check_time", &EquivalenceCheckingManager::Results::checkTime)
      .def_rw("equivalence", &EquivalenceCheckingManager::Results::equivalence)
      .def_rw("started_simulations",
              &EquivalenceCheckingManager::Results::startedSimulations)
      .def_rw("performed_simulations",
              &EquivalenceCheckingManager::Results::performedSimulations)
      .def_rw("cex_input", &EquivalenceCheckingManager::Results::cexInput)
      .def_rw("cex_output1", &EquivalenceCheckingManager::Results::cexOutput1)
      .def_rw("cex_output2", &EquivalenceCheckingManager::Results::cexOutput2)
      .def_rw("performed_instantiations",
              &EquivalenceCheckingManager::Results::performedInstantiations)
      .def_prop_rw(
          "checker_results",
          [](const EquivalenceCheckingManager::Results& results) {
            return nb::cast(results.checkerResults);
          },
          [](EquivalenceCheckingManager::Results& results,
             const nb::dict& value) {
            nb::module_ json = nb::module_::import_("json");
            nb::object dumps = json.attr("dumps");
            const auto jsonString = nb::cast<std::string>(dumps(value));
            results.checkerResults = nlohmann::json::parse(jsonString);
          })
      .def("considered_equivalent",
           &EquivalenceCheckingManager::Results::consideredEquivalent)
      .def("json",
           [](const EquivalenceCheckingManager::Results& results) {
             return nb::cast(results.json());
           })
      .def("__str__", &EquivalenceCheckingManager::Results::toString)
      .def("__repr__", [](const EquivalenceCheckingManager::Results& res) {
        return "<EquivalenceCheckingManager.Results: " +
               toString(res.equivalence) + ">";
      });
}

} // namespace ec
