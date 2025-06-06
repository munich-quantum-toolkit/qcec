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

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(MQT_QCEC_MODULE_NAME, m, py::mod_gil_not_used()) {
  // Class definitions
  py::class_<ec::EquivalenceCheckingManager> ecm(m,
                                                 "EquivalenceCheckingManager");
  py::class_<ec::EquivalenceCheckingManager::Results> results(ecm, "Results");

  // Constructors
  ecm.def(py::init<const qc::QuantumComputation&, const qc::QuantumComputation&,
                   ec::Configuration>(),
          "circ1"_a, "circ2"_a, "config"_a = ec::Configuration());

  // Access to circuits
  ecm.def_property_readonly("qc1",
                            &ec::EquivalenceCheckingManager::getFirstCircuit);
  ecm.def_property_readonly("qc2",
                            &ec::EquivalenceCheckingManager::getSecondCircuit);

  // Access to configuration
  ecm.def_property("configuration",
                   &ec::EquivalenceCheckingManager::getConfiguration,
                   [](ec::EquivalenceCheckingManager& manager,
                      const ec::Configuration& config) {
                     manager.getConfiguration() = config;
                   });

  // Run
  ecm.def("run", &ec::EquivalenceCheckingManager::run);

  // Results
  ecm.def_property_readonly("results",
                            &ec::EquivalenceCheckingManager::getResults);
  ecm.def_property_readonly("equivalence",
                            &ec::EquivalenceCheckingManager::equivalence);

  // Convenience functions
  // Execution
  ecm.def("disable_all_checkers",
          &ec::EquivalenceCheckingManager::disableAllCheckers)
      // Application
      .def("set_application_scheme",
           &ec::EquivalenceCheckingManager::setApplicationScheme,
           "scheme"_a = "proportional")
      .def("set_gate_cost_profile",
           &ec::EquivalenceCheckingManager::setGateCostProfile,
           "profile"_a = "")
      .def("__repr__", [](const ec::EquivalenceCheckingManager& manager) {
        return "<EquivalenceCheckingManager: " +
               toString(manager.equivalence()) + ">";
      });

  // ec::EquivalenceCheckingManager::Results bindings
  results.def(py::init<>())
      .def_readwrite(
          "preprocessing_time",
          &ec::EquivalenceCheckingManager::Results::preprocessingTime)
      .def_readwrite("check_time",
                     &ec::EquivalenceCheckingManager::Results::checkTime)
      .def_readwrite("equivalence",
                     &ec::EquivalenceCheckingManager::Results::equivalence)
      .def_readwrite(
          "started_simulations",
          &ec::EquivalenceCheckingManager::Results::startedSimulations)
      .def_readwrite(
          "performed_simulations",
          &ec::EquivalenceCheckingManager::Results::performedSimulations)
      .def_readwrite("cex_input",
                     &ec::EquivalenceCheckingManager::Results::cexInput)
      .def_readwrite("cex_output1",
                     &ec::EquivalenceCheckingManager::Results::cexOutput1)
      .def_readwrite("cex_output2",
                     &ec::EquivalenceCheckingManager::Results::cexOutput2)
      .def_readwrite(
          "performed_instantiations",
          &ec::EquivalenceCheckingManager::Results::performedInstantiations)
      .def_readwrite("checker_results",
                     &ec::EquivalenceCheckingManager::Results::checkerResults)
      .def("considered_equivalent",
           &ec::EquivalenceCheckingManager::Results::consideredEquivalent)
      .def("json", &ec::EquivalenceCheckingManager::Results::json)
      .def("__str__", &ec::EquivalenceCheckingManager::Results::toString)
      .def("__repr__", [](const ec::EquivalenceCheckingManager::Results& res) {
        return "<EquivalenceCheckingManager.Results: " +
               toString(res.equivalence) + ">";
      });
}
