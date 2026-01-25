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
  auto configuration = nb::class_<Configuration>(
      m, "Configuration", R"pb(Provides all the means to configure QCEC.

All options are split into the following categories:

- :class:`.Execution`
- :class:`.Optimizations`
- :class:`.Application`
- :class:`.Functionality`
- :class:`.Simulation`
- :class:`.Parameterized`

All options can be passed to the :func:`.verify` and :func:`.verify_compilation` functions as keyword arguments.
There, they are incorporated into the :class:`.Configuration` using the :func:`~.configuration.augment_config_from_kwargs` function.)pb");

  auto execution = nb::class_<Configuration::Execution>(
      configuration, "Execution",
      R"pb(Options that orchestrate the :meth:`~.EquivalenceCheckingManager.run` method.)pb");

  auto optimizations = nb::class_<Configuration::Optimizations>(
      configuration, "Optimizations",
      R"pb(Options that influence which circuit optimizations are applied during pre-processing.)pb");

  auto application = nb::class_<Configuration::Application>(
      configuration, "Application",
      R"pb(Options describing the :class:`.ApplicationScheme` used for the individual equivalence checkers.)pb");

  auto functionality = nb::class_<Configuration::Functionality>(
      configuration, "Functionality",
      R"pb(Options for all checkers that consider the whole functionality of a circuit.)pb");

  auto simulation = nb::class_<Configuration::Simulation>(
      configuration, "Simulation",
      R"pb(Options that influence the simulation checker.)pb");

  auto parameterized = nb::class_<Configuration::Parameterized>(
      configuration, "Parameterized",
      R"pb(Options that influence the equivalence checking scheme for parameterized circuits.)pb");

  // Configuration
  configuration.def(nb::init<>(), "Initializes the configuration.")
      .def_rw("execution", &Configuration::execution)
      .def_rw("optimizations", &Configuration::optimizations)
      .def_rw("application", &Configuration::application)
      .def_rw("functionality", &Configuration::functionality)
      .def_rw("simulation", &Configuration::simulation)
      .def_rw("parameterized", &Configuration::parameterized)
      .def(
          "json",
          [](const Configuration& config) {
            const auto json = nb::module_::import_("json");
            const auto loads = json.attr("loads");
            const auto dict = loads(config.json().dump());
            return nb::cast<nb::typed<nb::dict, nb::str, nb::any>>(dict);
          },
          R"pb(Returns a JSON-style dictionary of the configuration.)pb")
      .def("__repr__", &Configuration::toString);

  // execution options
  execution.def(nb::init<>())
      .def_rw(
          "parallel", &Configuration::Execution::parallel,
          R"pb(Set whether execution should happen in parallel. Defaults to :code:`True`.)pb")

      .def_rw(
          "nthreads", &Configuration::Execution::nthreads,
          R"pb(Set the maximum number of threads to use. Defaults to the maximum number of available threads reported by the OS.)pb")

      .def_rw(
          "timeout", &Configuration::Execution::timeout,
          R"pb(Set a timeout for :meth:`~.EquivalenceCheckingManager.run` (in seconds).

Defaults to :code:`0.`, which means no timeout.

.. note::

    Timeouts in QCEC work by checking an atomic flag in between the application of gates (for DD-based checkers) or rewrite rules (for the ZX-based checkers).
    Unfortunately, this means that an operation needs to be fully applied before a timeout can set in.
    If a certain operation during the equivalence check takes a very long time (e.g., because the DD is becoming very large), the timeout will not be triggered until that operation is finished.
    Thus, it is possible that the timeout is not triggered at the expected time, and it might seem like the timeout is being ignored.

    Unfortunately, there is no clean way to kill a thread without letting it finish its computation.
    That's something that could be made possible by switching from multi-threading to multi-processing, but the overhead of processes versus threads is huge on certain platforms and that would not be a good trade-off.
    In addition, more fine-grained abortion checks would significantly decrease the overall performance due to all the branching that would be necessary.

    Consequently, timeouts in QCEC are a best-effort feature, and they should not be relied upon to always work as expected.
    From experience, they tend to work reliably well for the ZX-based checkers, but they are less reliable for the DD-based checkers.)pb")
      .def_rw("run_construction_checker",
              &Configuration::Execution::runConstructionChecker,
              R"pb(Set whether the construction checker should be executed.

Defaults to :code:`False` since the alternating checker is to be preferred in most cases.)pb")

      .def_rw("run_simulation_checker",
              &Configuration::Execution::runSimulationChecker,
              R"pb(Set whether the simulation checker should be executed.

Defaults to :code:`True` since simulations can quickly show the non-equivalence of circuits in many cases.)pb")

      .def_rw("run_alternating_checker",
              &Configuration::Execution::runAlternatingChecker,
              R"pb(Set whether the alternating checker should be executed.

Defaults to :code:`True` since staying close to the identity can quickly show the equivalence of circuits in many cases.)pb")

      .def_rw("run_zx_checker", &Configuration::Execution::runZXChecker,
              R"pb(Set whether the ZX-calculus checker should be executed.

Defaults to :code:`True` but arbitrary multi-controlled operations are only partially supported.)pb")

      .def_rw(
          "numerical_tolerance", &Configuration::Execution::numericalTolerance,
          R"pb(Set the numerical tolerance of the underlying decision diagram package.

Defaults to :code:`2e-13` and should only be changed by users who know what they are doing.)pb")

      .def_rw("set_all_ancillae_garbage",
              &Configuration::Execution::setAllAncillaeGarbage,
              R"pb(Set whether all ancillae should be treated as garbage qubits.

Defaults to :code:`False` but the ZX-calculus checker will not be able to handle circuits with non-garbage ancillae.)pb");

  // optimization options
  optimizations.def(nb::init<>())
      .def_rw(
          "fuse_single_qubit_gates",
          &Configuration::Optimizations::fuseSingleQubitGates,
          R"pb(Fuse consecutive single-qubit gates by grouping them together.

Defaults to :code:`True` as this typically increases the performance of the subsequent equivalence check.)pb")

      .def_rw(
          "reconstruct_swaps", &Configuration::Optimizations::reconstructSWAPs,
          R"pb(Try to reconstruct SWAP gates that have been decomposed (into a sequence of 3 CNOT gates) or optimized away (as a consequence of a SWAP preceded or followed by a CNOT on the same qubits).

        Defaults to :code:`True` since this reconstruction enables the efficient tracking of logical to physical qubit permutations throughout circuits that have been mapped to a target architecture.)pb")

      .def_rw("remove_diagonal_gates_before_measure",
              &Configuration::Optimizations::removeDiagonalGatesBeforeMeasure,
              R"pb(Remove any diagonal gates at the end of the circuit.

This might be desirable since any diagonal gate in front of a measurement does not influence the probabilities of the respective states.

Defaults to :code:`False` since, in general, circuits differing by diagonal gates at the end should still be considered non-equivalent.)pb")

      .def_rw(
          "transform_dynamic_circuit",
          &Configuration::Optimizations::transformDynamicCircuit,
          R"pb(Circuits containing dynamic circuit primitives such as mid-circuit measurements, resets, or classically-controlled operations cannot be verified in a straight-forward fashion due to the non-unitary nature of these primitives, which is why this setting defaults to :code:`False`.

By enabling this optimization, any dynamic circuit is first transformed to a circuit without non-unitary primitives by, first, substituting qubit resets with new qubits and, then, applying the deferred measurement principle to defer measurements to the end.)pb")

      .def_rw(
          "reorder_operations",
          &Configuration::Optimizations::reorderOperations,
          R"pb(The operations of a circuit are stored in a sequential container.

This introduces some dependencies in the order of operations that are not naturally present in the quantum circuit.
As a consequence, two quantum circuits that contain exactly the same operations, list their operations in different ways, also apply there operations in a different order.
This optimization pass established a canonical ordering of operations by, first, constructing a directed, acyclic graph for the operations and, then, traversing it in a breadth-first fashion.

Defaults to :code:`True`.)pb")

      .def_rw(
          "backpropagate_output_permutation",
          &Configuration::Optimizations::backpropagateOutputPermutation,
          R"pb(Backpropagate the output permutation to the input permutation.

Defaults to :code:`False` since this might mess up the initially given input permutation.
Can be helpful for dynamic quantum circuits that have been transformed to a static circuit by enabling the :attr:`~.Configuration.Optimizations.transform_dynamic_circuit` optimization.)pb")

      .def_rw(
          "elide_permutations",
          &Configuration::Optimizations::elidePermutations,
          R"pb(Elide permutations from the circuit by permuting the qubits in the circuit and eliminating SWAP gates from the circuits.

Defaults to :code:`True` as this typically boosts performance.)pb");

  // application options
  application.def(nb::init<>())
      .def_rw(
          "construction_scheme",
          &Configuration::Application::constructionScheme,
          R"pb(The :class:`.ApplicationScheme` used for the construction checker.)pb")

      .def_rw(
          "simulation_scheme", &Configuration::Application::simulationScheme,
          R"pb(The :class:`.ApplicationScheme` used for the simulation checker.)pb")

      .def_rw(
          "alternating_scheme", &Configuration::Application::alternatingScheme,
          R"pb(The :class:`.ApplicationScheme` used for the alternating checker.)pb")

      .def_rw(
          "profile", &Configuration::Application::profile,
          R"pb(The :attr:`Gate Cost <.ApplicationScheme.gate_cost>` application scheme can be configured with a profile that specifies the cost of gates.

This profile can be set via a file constructed like a lookup table.
Every line :code:`<GATE_ID> <N_CONTROLS> <COST>` specifies the cost for a given gate type and with a certain number of controls, e.g., :code:`X 0 1` denotes that a single-qubit X gate has a cost of :code:`1`, while :code:`X 2 15` denotes that a Toffoli gate has a cost of :code:`15`.)pb");

  // functionality options
  functionality.def(nb::init<>())
      .def_rw(
          "trace_threshold", &Configuration::Functionality::traceThreshold,
          R"pb(While decision diagrams are canonical in theory, i.e., equivalent circuits produce equivalent decision diagrams, numerical inaccuracies and approximations can harm this property.

This can result in a scenario where two decision diagrams are really close to one another, but cannot be identified as such by standard methods (i.e., comparing their root pointers).
Instead, for two decision diagrams :code:`U` and :code:`U'` representing the functionalities of two circuits :code:`G` and :code:`G'`, the trace of the product of one decision diagram with the inverse of the other can be computed and compared to the trace of the identity.

Alternatively, it can be checked, whether :code:`U*U`^-1` is \"close enough\" to the identity by recursively checking that each decision diagram node is close enough to the identity structure (i.e., the first and last successor have weights close to one, while the second and third successor have weights close to zero).
Whenever any decision diagram node differs from this structure by more than the configured threshold, the circuits are concluded to be non-equivalent.

Defaults to :code:`1e-8`.)pb")

      .def_rw(
          "check_partial_equivalence",
          &Configuration::Functionality::checkPartialEquivalence,
          R"pb(Two circuits are partially equivalent if, for each possible initial input state, they have the same probability for each measurement outcome.

If set to :code:`True`, a check for partial equivalence will be performed and the contributions of garbage qubits to the circuit are ignored.
If set to :code:`False`, the checker will output 'not equivalent' for circuits that are partially equivalent but not totally equivalent.
In particular, garbage qubits will be treated as if they were measured qubits.

Defaults to :code:`False`.)pb");

  // simulation options
  simulation.def(nb::init<>())
      .def_rw(
          "fidelity_threshold", &Configuration::Simulation::fidelityThreshold,
          R"pb(Similar to :attr:`~.Configuration.Functionality.trace_threshold`, this setting is here to tackle numerical inaccuracies in the simulation checker.

Instead of computing a trace, the fidelity between the states resulting from the simulation is computed.
Whenever the fidelity differs from :code:`1.` by more than the configured threshold, the circuits are concluded to be non-equivalent.

Defaults to :code:`1e-8`.)pb")

      .def_rw(
          "max_sims", &Configuration::Simulation::maxSims,
          R"pb(The maximum number of simulations to be started for the simulation checker.

In practice, just a couple of simulations suffice in most cases to detect a potential non-equivalence.
Either defaults to :code:`16` or the maximum number of available threads minus 2, whichever is more.)pb")

      .def_rw(
          "state_type", &Configuration::Simulation::stateType,
          R"pb(The :class:`type of states <.StateType>` used for the simulations in the simulation checker.

Defaults to :attr:`.StateType.computational_basis`.)pb")

      .def_rw("seed", &Configuration::Simulation::seed,
              R"pb(The seed used in the quantum state generator.

Defaults to :code:`0`, which means that the seed is chosen non-deterministically for each program run.)pb");

  // parameterized options
  parameterized.def(nb::init<>())
      .def_rw(
          "parameterized_tolerance",
          &Configuration::Parameterized::parameterizedTol,
          R"pb(Set threshold below which instantiated parameters shall be considered zero.

Defaults to :code:`1e-12`.)pb")

      .def_rw(
          "additional_instantiations",
          &Configuration::Parameterized::nAdditionalInstantiations,
          R"pb(Number of instantiations shall be performed in addition to the default ones.

For parameterized circuits that cannot be shown to be equivalent by the ZX checker the circuits are instantiated with concrete values for parameters and subsequently checked with QCEC's default schemes.
The first instantiation tries to set as many gate parameters to 0.
The last instantiations initializes the parameters with random values to guarantee completeness of the equivalence check.
Because random instantiation is costly, additional instantiations can be performed that lead to simpler equivalence checking instances as the random instantiation.
This option changes how many of those additional checks are performed.)pb");
}

} // namespace ec
