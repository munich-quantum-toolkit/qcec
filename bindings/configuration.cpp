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

namespace ec {

// NOLINTNEXTLINE(misc-include-cleaner)
PYBIND11_MODULE(MQT_QCEC_MODULE_NAME, m, py::mod_gil_not_used()) {
  // Class definitions
  py::class_<Configuration> configuration(m, "Configuration");
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
