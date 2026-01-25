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
#include "checker/dd/applicationscheme/ApplicationScheme.hpp"
#include "ir/QuantumComputation.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>      // NOLINT(misc-include-cleaner)
#include <nanobind/stl/string_view.h> // NOLINT(misc-include-cleaner)
#include <string>

namespace ec {

namespace nb = nanobind;
using namespace nb::literals;

// NOLINTNEXTLINE(misc-use-internal-linkage)
void registerEquivalenceCheckingManager(const nb::module_& m) {
  nb::module_::import_("mqt.core.dd");

  // Class definitions
  auto ecm = nb::class_<EquivalenceCheckingManager>(
      m, "EquivalenceCheckingManager", R"pb(The main class of QCEC.

Allows checking the equivalence of quantum circuits based on the methods proposed in :cite:p:`burgholzer2021advanced`.
It features many configuration options that orchestrate the procedure.)pb");

  auto results = nb::class_<EquivalenceCheckingManager::Results>(
      ecm, "Results",
      R"pb(Captures the main results and statistics from :meth:`~.EquivalenceCheckingManager.run`.)pb");

  // Constructors
  ecm.def(
      nb::init<const qc::QuantumComputation&, const qc::QuantumComputation&,
               Configuration>(),
      "circ1"_a, "circ2"_a, "config"_a = Configuration(),
      R"pb(Create an equivalence checking manager for two circuits and configure it with a :class:`.Configuration` object.)pb");

  // Access to circuits
  ecm.def_prop_ro("qc1", &EquivalenceCheckingManager::getFirstCircuit,
                  nb::rv_policy::reference_internal,
                  R"pb(The first circuit to be checked.)pb");
  ecm.def_prop_ro("qc2", &EquivalenceCheckingManager::getSecondCircuit,
                  nb::rv_policy::reference_internal,
                  R"pb(The second circuit to be checked.)pb");

  // Access to configuration
  ecm.def_prop_rw(
      "configuration", &EquivalenceCheckingManager::getConfiguration,
      [](EquivalenceCheckingManager& manager, const Configuration& config) {
        manager.getConfiguration() = config;
      },
      nb::rv_policy::reference_internal,
      R"pb(The configuration of the equivalence checking manager.)pb");

  // Run
  ecm.def("run", &EquivalenceCheckingManager::run,
          R"pb(Execute the equivalence check as configured.)pb");

  // Results
  ecm.def_prop_ro("results", &EquivalenceCheckingManager::getResults,
                  nb::rv_policy::reference_internal,
                  R"pb(The results of the equivalence check.)pb");

  ecm.def_prop_ro(
      "equivalence", &EquivalenceCheckingManager::equivalence,
      R"pb(The :class:`.EquivalenceCriterion` determined as the result of the equivalence check.)pb");

  // Convenience functions
  // Execution
  ecm.def("disable_all_checkers",
          &EquivalenceCheckingManager::disableAllCheckers,
          R"pb(Disable all equivalence checkers.)pb")

      // Application
      .def(
          "set_application_scheme",
          &EquivalenceCheckingManager::setApplicationScheme,
          "scheme"_a = ApplicationSchemeType::Proportional,
          R"pb(Set the :class:`.ApplicationScheme` used for all checkers (based on decision diagrams).

Args:
    scheme: The application scheme. Defaults to :attr:`.ApplicationScheme.proportional`.)pb")

      .def(
          "set_gate_cost_profile",
          &EquivalenceCheckingManager::setGateCostProfile, "profile"_a = "",
          R"pb(Set the :attr:`profile <.Configuration.Application.profile>` used in the :attr:`Gate Cost <.ApplicationScheme.gate_cost>` application scheme for all checkers (based on decision diagrams).

Args:
    profile: The path to the profile file.)pb")

      .def("__repr__", [](const EquivalenceCheckingManager& manager) {
        return "<EquivalenceCheckingManager: " +
               toString(manager.equivalence()) + ">";
      });

  // EquivalenceCheckingManager::Results bindings
  results
      .def(nb::init<>(), "Initializes the results.")

      .def_rw("preprocessing_time",
              &EquivalenceCheckingManager::Results::preprocessingTime,
              R"pb(Time spent during preprocessing (in seconds).)pb")

      .def_rw("check_time", &EquivalenceCheckingManager::Results::checkTime,
              R"pb(Time spent during equivalence check (in seconds).)pb")

      .def_rw("equivalence", &EquivalenceCheckingManager::Results::equivalence,
              R"pb(Final result of the equivalence check.)pb")

      .def_rw("started_simulations",
              &EquivalenceCheckingManager::Results::startedSimulations,
              R"pb(Number of simulations that have been started.)pb")

      .def_rw("performed_simulations",
              &EquivalenceCheckingManager::Results::performedSimulations,
              R"pb(Number of simulations that have been finished.)pb")

      .def_rw(
          "cex_input", &EquivalenceCheckingManager::Results::cexInput,
          R"pb(DD representation of the initial state that produced a counterexample.)pb")

      .def_rw(
          "cex_output1", &EquivalenceCheckingManager::Results::cexOutput1,
          R"pb(DD representation of the first circuit's counterexample output state.)pb")

      .def_rw(
          "cex_output2", &EquivalenceCheckingManager::Results::cexOutput2,
          R"pb(DD representation of the second circuit's counterexample output state.)pb")

      .def_rw(
          "performed_instantiations",
          &EquivalenceCheckingManager::Results::performedInstantiations,
          R"pb(Number of circuit instantiations performed during equivalence checking of parameterized quantum circuits.)pb")

      .def_prop_rw(
          "checker_results",
          [](const EquivalenceCheckingManager::Results& results) {
            const auto json = nb::module_::import_("json");
            const auto loads = json.attr("loads");
            const auto dict = loads(results.checkerResults.dump());
            return nb::cast<nb::typed<nb::dict, nb::str, nb::any>>(dict);
          },
          [](EquivalenceCheckingManager::Results& results,
             const nb::dict& value) {
            const auto json = nb::module_::import_("json");
            const auto dumps = json.attr("dumps");
            const auto jsonString = nb::cast<std::string>(dumps(value));
            results.checkerResults = nlohmann::json::parse(jsonString);
          },
          R"pb(Dictionary of the results of the individual checkers.)pb")

      .def(
          "considered_equivalent",
          &EquivalenceCheckingManager::Results::consideredEquivalent,
          R"pb(Convenience function to check whether the result is considered equivalent.)pb")

      .def(
          "json",
          [](const EquivalenceCheckingManager::Results& results) {
            const auto json = nb::module_::import_("json");
            const auto loads = json.attr("loads");
            const auto dict = loads(results.json().dump());
            return nb::cast<nb::typed<nb::dict, nb::str, nb::any>>(dict);
          },
          R"pb(Returns a JSON-style dictionary of the results.)pb")

      .def("__str__", &EquivalenceCheckingManager::Results::toString)
      .def("__repr__", [](const EquivalenceCheckingManager::Results& res) {
        return "<EquivalenceCheckingManager.Results: " +
               toString(res.equivalence) + ">";
      });
}

} // namespace ec
