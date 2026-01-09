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

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h> // NOLINT(misc-include-cleaner)

namespace ec {

namespace nb = nanobind;
using namespace nb::literals;

// NOLINTNEXTLINE(misc-use-internal-linkage)
void registerConfiguration(const nb::module_& m) {
  // Class definitions
  auto configuration = nb::class_<Configuration>(m, "Configuration");
  auto execution =
      nb::class_<Configuration::Execution>(configuration, "Execution");
  auto optimizations =
      nb::class_<Configuration::Optimizations>(configuration, "Optimizations");
  auto application =
      nb::class_<Configuration::Application>(configuration, "Application");
  auto functionality =
      nb::class_<Configuration::Functionality>(configuration, "Functionality");
  auto simulation =
      nb::class_<Configuration::Simulation>(configuration, "Simulation");
  auto parameterized =
      nb::class_<Configuration::Parameterized>(configuration, "Parameterized");

  // Configuration
  configuration.def(nb::init<>())
      .def_rw("execution", &Configuration::execution)
      .def_rw("optimizations", &Configuration::optimizations)
      .def_rw("application", &Configuration::application)
      .def_rw("functionality", &Configuration::functionality)
      .def_rw("simulation", &Configuration::simulation)
      .def_rw("parameterized", &Configuration::parameterized)
      .def("json",
           [](const Configuration& config) { return nb::cast(config.json()); })
      .def("__repr__", &Configuration::toString);

  // execution options
  execution.def(nb::init<>())
      .def_rw("parallel", &Configuration::Execution::parallel)
      .def_rw("nthreads", &Configuration::Execution::nthreads)
      .def_rw("timeout", &Configuration::Execution::timeout)
      .def_rw("run_construction_checker",
              &Configuration::Execution::runConstructionChecker)
      .def_rw("run_simulation_checker",
              &Configuration::Execution::runSimulationChecker)
      .def_rw("run_alternating_checker",
              &Configuration::Execution::runAlternatingChecker)
      .def_rw("run_zx_checker", &Configuration::Execution::runZXChecker)
      .def_rw("numerical_tolerance",
              &Configuration::Execution::numericalTolerance)
      .def_rw("set_all_ancillae_garbage",
              &Configuration::Execution::setAllAncillaeGarbage);

  // optimization options
  optimizations.def(nb::init<>())
      .def_rw("fuse_single_qubit_gates",
              &Configuration::Optimizations::fuseSingleQubitGates)
      .def_rw("reconstruct_swaps",
              &Configuration::Optimizations::reconstructSWAPs)
      .def_rw("remove_diagonal_gates_before_measure",
              &Configuration::Optimizations::removeDiagonalGatesBeforeMeasure)
      .def_rw("transform_dynamic_circuit",
              &Configuration::Optimizations::transformDynamicCircuit)
      .def_rw("reorder_operations",
              &Configuration::Optimizations::reorderOperations)
      .def_rw("backpropagate_output_permutation",
              &Configuration::Optimizations::backpropagateOutputPermutation)
      .def_rw("elide_permutations",
              &Configuration::Optimizations::elidePermutations);

  // application options
  application.def(nb::init<>())
      .def_rw("construction_scheme",
              &Configuration::Application::constructionScheme)
      .def_rw("simulation_scheme",
              &Configuration::Application::simulationScheme)
      .def_rw("alternating_scheme",
              &Configuration::Application::alternatingScheme)
      .def_rw("profile", &Configuration::Application::profile);

  // functionality options
  functionality.def(nb::init<>())
      .def_rw("trace_threshold", &Configuration::Functionality::traceThreshold)
      .def_rw("check_partial_equivalence",
              &Configuration::Functionality::checkPartialEquivalence);

  // simulation options
  simulation.def(nb::init<>())
      .def_rw("fidelity_threshold",
              &Configuration::Simulation::fidelityThreshold)
      .def_rw("max_sims", &Configuration::Simulation::maxSims)
      .def_rw("state_type", &Configuration::Simulation::stateType)
      .def_rw("seed", &Configuration::Simulation::seed);

  // parameterized options
  parameterized.def(nb::init<>())
      .def_rw("parameterized_tolerance",
              &Configuration::Parameterized::parameterizedTol)
      .def_rw("additional_instantiations",
              &Configuration::Parameterized::nAdditionalInstantiations);
}

} // namespace ec
