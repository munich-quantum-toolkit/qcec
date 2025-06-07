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

#include <pybind11/pybind11.h>
#include <pybind11/stl.h> // NOLINT(misc-include-cleaner)

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(MQT_QCEC_MODULE_NAME, m, py::mod_gil_not_used()) {
  // Class definitions
  py::class_<ec::Configuration> configuration(m, "Configuration");
  py::class_<ec::Configuration::Execution> execution(configuration,
                                                     "Execution");
  py::class_<ec::Configuration::Optimizations> optimizations(configuration,
                                                             "Optimizations");
  py::class_<ec::Configuration::Application> application(configuration,
                                                         "Application");
  py::class_<ec::Configuration::Functionality> functionality(configuration,
                                                             "Functionality");
  py::class_<ec::Configuration::Simulation> simulation(configuration,
                                                       "Simulation");
  py::class_<ec::Configuration::Parameterized> parameterized(configuration,
                                                             "Parameterized");

  // Configuration
  configuration.def(py::init<>())
      .def_readwrite("execution", &ec::Configuration::execution)
      .def_readwrite("optimizations", &ec::Configuration::optimizations)
      .def_readwrite("application", &ec::Configuration::application)
      .def_readwrite("functionality", &ec::Configuration::functionality)
      .def_readwrite("simulation", &ec::Configuration::simulation)
      .def_readwrite("parameterized", &ec::Configuration::parameterized)
      .def("json", &ec::Configuration::json)
      .def("__repr__", &ec::Configuration::toString);

  // execution options
  execution.def(py::init<>())
      .def_readwrite("parallel", &ec::Configuration::Execution::parallel)
      .def_readwrite("nthreads", &ec::Configuration::Execution::nthreads)
      .def_readwrite("timeout", &ec::Configuration::Execution::timeout)
      .def_readwrite("run_construction_checker",
                     &ec::Configuration::Execution::runConstructionChecker)
      .def_readwrite("run_simulation_checker",
                     &ec::Configuration::Execution::runSimulationChecker)
      .def_readwrite("run_alternating_checker",
                     &ec::Configuration::Execution::runAlternatingChecker)
      .def_readwrite("run_zx_checker",
                     &ec::Configuration::Execution::runZXChecker)
      .def_readwrite("numerical_tolerance",
                     &ec::Configuration::Execution::numericalTolerance);

  // optimization options
  optimizations.def(py::init<>())
      .def_readwrite("fuse_single_qubit_gates",
                     &ec::Configuration::Optimizations::fuseSingleQubitGates)
      .def_readwrite("reconstruct_swaps",
                     &ec::Configuration::Optimizations::reconstructSWAPs)
      .def_readwrite(
          "remove_diagonal_gates_before_measure",
          &ec::Configuration::Optimizations::removeDiagonalGatesBeforeMeasure)
      .def_readwrite("transform_dynamic_circuit",
                     &ec::Configuration::Optimizations::transformDynamicCircuit)
      .def_readwrite("reorder_operations",
                     &ec::Configuration::Optimizations::reorderOperations)
      .def_readwrite(
          "backpropagate_output_permutation",
          &ec::Configuration::Optimizations::backpropagateOutputPermutation)
      .def_readwrite("elide_permutations",
                     &ec::Configuration::Optimizations::elidePermutations);

  // application options
  application.def(py::init<>())
      .def_readwrite("construction_scheme",
                     &ec::Configuration::Application::constructionScheme)
      .def_readwrite("simulation_scheme",
                     &ec::Configuration::Application::simulationScheme)
      .def_readwrite("alternating_scheme",
                     &ec::Configuration::Application::alternatingScheme)
      .def_readwrite("profile", &ec::Configuration::Application::profile);

  // functionality options
  functionality.def(py::init<>())
      .def_readwrite("trace_threshold",
                     &ec::Configuration::Functionality::traceThreshold)
      .def_readwrite(
          "check_partial_equivalence",
          &ec::Configuration::Functionality::checkPartialEquivalence);

  // simulation options
  simulation.def(py::init<>())
      .def_readwrite("fidelity_threshold",
                     &ec::Configuration::Simulation::fidelityThreshold)
      .def_readwrite("max_sims", &ec::Configuration::Simulation::maxSims)
      .def_readwrite("state_type", &ec::Configuration::Simulation::stateType)
      .def_readwrite("seed", &ec::Configuration::Simulation::seed);

  // parameterized options
  parameterized.def(py::init<>())
      .def_readwrite("parameterized_tolerance",
                     &ec::Configuration::Parameterized::parameterizedTol)
      .def_readwrite(
          "additional_instantiations",
          &ec::Configuration::Parameterized::nAdditionalInstantiations);
}
