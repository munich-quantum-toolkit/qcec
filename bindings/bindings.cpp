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
#include "checker/dd/simulation/StateType.hpp"
#include "dd/RealNumber.hpp"
#include "ir/QuantumComputation.hpp"

#include <algorithm>
#include <exception>
#include <memory>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11_json/pybind11_json.hpp>
#include <stdexcept>
#include <string>
#include <thread>

namespace py = pybind11;
using namespace pybind11::literals;

namespace ec {

PYBIND11_MODULE(pyqcec, m, py::mod_gil_not_used()) {
  m.doc() = "Python interface for the MQT QCEC quantum circuit equivalence "
            "checking tool";

  // Application scheme enum
  py::enum_<ApplicationSchemeType>(m, "ApplicationScheme")
      .value("sequential", ApplicationSchemeType::Sequential)
      .value("one_to_one", ApplicationSchemeType::OneToOne)
      .value("proportional", ApplicationSchemeType::Proportional)
      .value("lookahead", ApplicationSchemeType::Lookahead)
      .value(
          "gate_cost", ApplicationSchemeType::GateCost,
          "Each gate of the first circuit is associated with a corresponding "
          "cost according to some cost function *f(...)*. Whenever a gate *g* "
          "from the first circuit is applied *f(g)* gates are applied from the "
          "second circuit. Referred to as *compilation_flow* in "
          ":cite:p:`burgholzer2020verifyingResultsIBM`.")
      // allow construction from a string
      .def(py::init([](const std::string& str) -> ApplicationSchemeType {
             return applicationSchemeFromString(str);
           }),
           "scheme"_a)
      // provide a string representation of the enum
      .def(
          "__str__",
          [](const ApplicationSchemeType scheme) { return toString(scheme); },
          py::prepend());
  // allow implicit conversion from string to ApplicationSchemeType
  py::implicitly_convertible<std::string, ApplicationSchemeType>();

  // State type enum
  py::enum_<StateType>(m, "StateType")
      .value("computational_basis", StateType::ComputationalBasis)
      .value("random_1Q_basis", StateType::Random1QBasis)
      .value("stabilizer", StateType::Stabilizer)
      // allow construction from a string
      .def(py::init([](const std::string& str) -> StateType {
             return stateTypeFromString(str);
           }),
           "state_type"_a)
      // provide a string representation of the enum
      .def(
          "__str__", [](const StateType type) { return toString(type); },
          py::prepend());
  // allow implicit conversion from string to StateType
  py::implicitly_convertible<std::string, StateType>();

  // Equivalence criterion enum
  py::enum_<EquivalenceCriterion>(m, "EquivalenceCriterion")
      .value("no_information", EquivalenceCriterion::NoInformation)
      .value("not_equivalent", EquivalenceCriterion::NotEquivalent)
      .value("equivalent", EquivalenceCriterion::Equivalent)
      .value("equivalent_up_to_phase",
             EquivalenceCriterion::EquivalentUpToPhase)
      .value("equivalent_up_to_global_phase",
             EquivalenceCriterion::EquivalentUpToGlobalPhase)
      .value("probably_equivalent", EquivalenceCriterion::ProbablyEquivalent)
      .value("probably_not_equivalent",
             EquivalenceCriterion::ProbablyNotEquivalent)
      // allow construction from a string
      .def(py::init([](const std::string& str) -> EquivalenceCriterion {
             return fromString(str);
           }),
           "criterion"_a)
      // provide a string representation of the enum
      .def(
          "__str__",
          [](const EquivalenceCriterion crit) { return toString(crit); },
          py::prepend());
  // allow implicit conversion from string to EquivalenceCriterion
  py::implicitly_convertible<std::string, EquivalenceCriterion>();

  // Class definitions
  py::class_<EquivalenceCheckingManager> ecm(m, "EquivalenceCheckingManager");
  py::class_<EquivalenceCheckingManager::Results> results(ecm, "Results");
  py::class_<Configuration> configuration(m, "Configuration");

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

  // Configuration sub-classes
  py::class_<Configuration::Execution> execution(configuration, "Execution");
  py::class_<Configuration::Optimizations> optimizations(configuration,
                                                         "Optimizations");
  py::class_<Configuration::Application> application(configuration,
                                                     "Application");
  py::class_<Configuration::Functionality> functionality(configuration,
                                                         "Functionality");
  py::class_<Configuration::Simulation> simulation(configuration, "Simulation");
  py::class_<Configuration::Parameterized> parameterized(configuration,
                                                         "Parameterized");

  // Configuration
  configuration.def(py::init<>())
      .def_readwrite("execution", &Configuration::execution)
      .def_readwrite("optimizations", &Configuration::optimizations)
      .def_readwrite("application", &Configuration::application)
      .def_readwrite("functionality", &Configuration::functionality)
      .def_readwrite("simulation", &Configuration::simulation)
      .def_readwrite("parameterized", &Configuration::parameterized)
      .def("json", &Configuration::json)
      .def("__repr__", &Configuration::toString);

  // execution options
  execution.def(py::init<>())
      .def_readwrite("parallel", &Configuration::Execution::parallel)
      .def_readwrite("nthreads", &Configuration::Execution::nthreads)
      .def_readwrite("timeout", &Configuration::Execution::timeout)
      .def_readwrite("run_construction_checker",
                     &Configuration::Execution::runConstructionChecker)
      .def_readwrite("run_simulation_checker",
                     &Configuration::Execution::runSimulationChecker)
      .def_readwrite("run_alternating_checker",
                     &Configuration::Execution::runAlternatingChecker)
      .def_readwrite("run_zx_checker", &Configuration::Execution::runZXChecker)
      .def_readwrite("numerical_tolerance",
                     &Configuration::Execution::numericalTolerance)
      .def_readwrite("set_all_ancillae_garbage",
                     &Configuration::Execution::setAllAncillaeGarbage);

  // optimization options
  optimizations.def(py::init<>())
      .def_readwrite("fuse_single_qubit_gates",
                     &Configuration::Optimizations::fuseSingleQubitGates)
      .def_readwrite("reconstruct_swaps",
                     &Configuration::Optimizations::reconstructSWAPs)
      .def_readwrite(
          "remove_diagonal_gates_before_measure",
          &Configuration::Optimizations::removeDiagonalGatesBeforeMeasure)
      .def_readwrite("transform_dynamic_circuit",
                     &Configuration::Optimizations::transformDynamicCircuit)
      .def_readwrite("reorder_operations",
                     &Configuration::Optimizations::reorderOperations)
      .def_readwrite(
          "backpropagate_output_permutation",
          &Configuration::Optimizations::backpropagateOutputPermutation)
      .def_readwrite("elide_permutations",
                     &Configuration::Optimizations::elidePermutations);

  // application options
  application.def(py::init<>())
      .def_readwrite("construction_scheme",
                     &Configuration::Application::constructionScheme)
      .def_readwrite("simulation_scheme",
                     &Configuration::Application::simulationScheme)
      .def_readwrite("alternating_scheme",
                     &Configuration::Application::alternatingScheme)
      .def_readwrite("profile", &Configuration::Application::profile);

  // functionality options
  functionality.def(py::init<>())
      .def_readwrite("trace_threshold",
                     &Configuration::Functionality::traceThreshold)
      .def_readwrite("check_partial_equivalence",
                     &Configuration::Functionality::checkPartialEquivalence);

  // simulation options
  simulation.def(py::init<>())
      .def_readwrite("fidelity_threshold",
                     &Configuration::Simulation::fidelityThreshold)
      .def_readwrite("max_sims", &Configuration::Simulation::maxSims)
      .def_readwrite("state_type", &Configuration::Simulation::stateType)
      .def_readwrite("seed", &Configuration::Simulation::seed);

  // parameterized options
  parameterized.def(py::init<>())
      .def_readwrite("parameterized_tolerance",
                     &Configuration::Parameterized::parameterizedTol)
      .def_readwrite("additional_instantiations",
                     &Configuration::Parameterized::nAdditionalInstantiations);
}
} // namespace ec
