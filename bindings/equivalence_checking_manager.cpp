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
#include "ir/QuantumComputation.hpp"

#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h> // NOLINT(misc-include-cleaner)

namespace ec {

namespace py = pybind11;
using namespace pybind11::literals;

// NOLINTNEXTLINE(misc-use-internal-linkage)
void registerEquivalenceCheckingManager(const py::module& mod) {
  // Class definitions
  auto ecm =
      py::class_<EquivalenceCheckingManager>(mod, "EquivalenceCheckingManager");
  auto results =
      py::class_<EquivalenceCheckingManager::Results>(ecm, "Results");

  // Constructors
  ecm.def(py::init<const qc::QuantumComputation&, const qc::QuantumComputation&,
                   Configuration>(),
          "circ1"_a, "circ2"_a, "config"_a = Configuration());

  // Access to circuits
  ecm.def_property_readonly("qc1",
                            &EquivalenceCheckingManager::getFirstCircuit);
  ecm.def_property_readonly("qc2",
                            &EquivalenceCheckingManager::getSecondCircuit);

  // Access to configuration
  ecm.def_property(
      "configuration", &EquivalenceCheckingManager::getConfiguration,
      [](EquivalenceCheckingManager& manager, const Configuration& config) {
        manager.getConfiguration() = config;
      });

  // Run
  ecm.def("run", &EquivalenceCheckingManager::run);

  // Results
  ecm.def_property_readonly("results", &EquivalenceCheckingManager::getResults);
  ecm.def_property_readonly("equivalence",
                            &EquivalenceCheckingManager::equivalence);

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
  results.def(py::init<>())
      .def_readwrite("preprocessing_time",
                     &EquivalenceCheckingManager::Results::preprocessingTime)
      .def_readwrite("check_time",
                     &EquivalenceCheckingManager::Results::checkTime)
      .def_readwrite("equivalence",
                     &EquivalenceCheckingManager::Results::equivalence)
      .def_readwrite("started_simulations",
                     &EquivalenceCheckingManager::Results::startedSimulations)
      .def_readwrite("performed_simulations",
                     &EquivalenceCheckingManager::Results::performedSimulations)
      .def_readwrite("cex_input",
                     &EquivalenceCheckingManager::Results::cexInput)
      .def_readwrite("cex_output1",
                     &EquivalenceCheckingManager::Results::cexOutput1)
      .def_readwrite("cex_output2",
                     &EquivalenceCheckingManager::Results::cexOutput2)
      .def_readwrite(
          "performed_instantiations",
          &EquivalenceCheckingManager::Results::performedInstantiations)
      .def_readwrite("checker_results",
                     &EquivalenceCheckingManager::Results::checkerResults)
      .def("considered_equivalent",
           &EquivalenceCheckingManager::Results::consideredEquivalent)
      .def("json", &EquivalenceCheckingManager::Results::json)
      .def("__str__", &EquivalenceCheckingManager::Results::toString)
      .def("__repr__", [](const EquivalenceCheckingManager::Results& res) {
        return "<EquivalenceCheckingManager.Results: " +
               toString(res.equivalence) + ">";
      });
}

} // namespace ec
