# Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

import enum
from typing import Any

import mqt.core.dd
import mqt.core.ir

class ApplicationScheme(enum.IntEnum):
    """Describes the order in which the individual operations of both circuits are applied during the equivalence check.

    In case of the alternating equivalence checker, this is the key component to allow the intermediate decision diagrams to remain close to the identity (as proposed in :cite:p:`burgholzer2021advanced`).
    See :doc:`/compilation_flow_verification` for more information on the dedicated application scheme for verifying compilation flow results (as proposed in :cite:p:`burgholzer2020verifyingResultsIBM`).

    In case of the other checkers, which consider both circuits individually, using a non-sequential application scheme can significantly boost the operation caching performance in the underlying decision diagram package.
    """

    sequential = 0
    """
    Applies all gates from the first circuit, before proceeding with the second circuit.

    Referred to as *"reference"* in :cite:p:`burgholzer2021advanced`.
    """

    reference = 0
    """Alias for :attr:`~ApplicationScheme.sequential`."""

    one_to_one = 1
    """
    Alternates between applications from the first and the second circuit.

    Referred to as *"naive"* in :cite:p:`burgholzer2021advanced`.
    """

    naive = 1
    """Alias for :attr:`~ApplicationScheme.one_to_one`."""

    lookahead = 2
    """
    Looks whether an application from the first circuit or the second circuit yields the smaller decision diagram.

    Only works for the alternating equivalence checker.
    """

    gate_cost = 3
    """
    Each gate of the first circuit is associated with a corresponding cost according to some cost function *f(...)*.
    Whenever a gate *g* from the first circuit is applied *f(g)* gates are applied from the second circuit.

    Referred to as *"compilation_flow"* in :cite:p:`burgholzer2020verifyingResultsIBM`.
    """

    compilation_flow = 3
    """Alias for :attr:`~ApplicationScheme.gate_cost`."""

    proportional = 4
    """
    Alternates between applications from the first and the second circuit, but applies the gates in proportion to the number of gates in each circuit.
    """

class Configuration:
    """Provides all the means to configure QCEC.

    All options are split into the following categories:

    - :class:`.Execution`
    - :class:`.Optimizations`
    - :class:`.Application`
    - :class:`.Functionality`
    - :class:`.Simulation`
    - :class:`.Parameterized`

    All options can be passed to the :func:`.verify` and :func:`.verify_compilation` functions as keyword arguments.
    There, they are incorporated into the :class:`.Configuration` using the :func:`~.configuration.augment_config_from_kwargs` function.
    """

    def __init__(self) -> None:
        """Initializes the configuration."""

    class Execution:
        """Options that orchestrate the :meth:`~.EquivalenceCheckingManager.run` method."""

        def __init__(self) -> None: ...
        @property
        def parallel(self) -> bool:
            """Set whether execution should happen in parallel. Defaults to :code:`True`."""

        @parallel.setter
        def parallel(self, arg: bool, /) -> None: ...
        @property
        def nthreads(self) -> int:
            """Set the maximum number of threads to use. Defaults to the maximum number of available threads reported by the OS."""

        @nthreads.setter
        def nthreads(self, arg: int, /) -> None: ...
        @property
        def timeout(self) -> float:
            """Set a timeout for :meth:`~.EquivalenceCheckingManager.run` (in seconds).

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
                From experience, they tend to work reliably well for the ZX-based checkers, but they are less reliable for the DD-based checkers.
            """

        @timeout.setter
        def timeout(self, arg: float, /) -> None: ...
        @property
        def run_construction_checker(self) -> bool:
            """Set whether the construction checker should be executed.

            Defaults to :code:`False` since the alternating checker is to be preferred in most cases.
            """

        @run_construction_checker.setter
        def run_construction_checker(self, arg: bool, /) -> None: ...
        @property
        def run_simulation_checker(self) -> bool:
            """Set whether the simulation checker should be executed.

            Defaults to :code:`True` since simulations can quickly show the non-equivalence of circuits in many cases.
            """

        @run_simulation_checker.setter
        def run_simulation_checker(self, arg: bool, /) -> None: ...
        @property
        def run_alternating_checker(self) -> bool:
            """Set whether the alternating checker should be executed.

            Defaults to :code:`True` since staying close to the identity can quickly show the equivalence of circuits in many cases.
            """

        @run_alternating_checker.setter
        def run_alternating_checker(self, arg: bool, /) -> None: ...
        @property
        def run_zx_checker(self) -> bool:
            """Set whether the ZX-calculus checker should be executed.

            Defaults to :code:`True` but arbitrary multi-controlled operations are only partially supported.
            """

        @run_zx_checker.setter
        def run_zx_checker(self, arg: bool, /) -> None: ...
        @property
        def numerical_tolerance(self) -> float:
            """Set the numerical tolerance of the underlying decision diagram package.

            Defaults to :code:`2e-13` and should only be changed by users who know what they are doing.
            """

        @numerical_tolerance.setter
        def numerical_tolerance(self, arg: float, /) -> None: ...
        @property
        def set_all_ancillae_garbage(self) -> bool:
            """Set whether all ancillae should be treated as garbage qubits.

            Defaults to :code:`False` but the ZX-calculus checker will not be able to handle circuits with non-garbage ancillae.
            """

        @set_all_ancillae_garbage.setter
        def set_all_ancillae_garbage(self, arg: bool, /) -> None: ...

    class Optimizations:
        """Options that influence which circuit optimizations are applied during pre-processing."""

        def __init__(self) -> None: ...
        @property
        def fuse_single_qubit_gates(self) -> bool:
            """Fuse consecutive single-qubit gates by grouping them together.

            Defaults to :code:`True` as this typically increases the performance of the subsequent equivalence check.
            """

        @fuse_single_qubit_gates.setter
        def fuse_single_qubit_gates(self, arg: bool, /) -> None: ...
        @property
        def reconstruct_swaps(self) -> bool:
            """Try to reconstruct SWAP gates that have been decomposed (into a sequence of 3 CNOT gates) or optimized away (as a consequence of a SWAP preceded or followed by a CNOT on the same qubits).

            Defaults to :code:`True` since this reconstruction enables the efficient tracking of logical to physical qubit permutations throughout circuits that have been mapped to a target architecture.
            """

        @reconstruct_swaps.setter
        def reconstruct_swaps(self, arg: bool, /) -> None: ...
        @property
        def remove_diagonal_gates_before_measure(self) -> bool:
            """Remove any diagonal gates at the end of the circuit.

            This might be desirable since any diagonal gate in front of a measurement does not influence the probabilities of the respective states.

            Defaults to :code:`False` since, in general, circuits differing by diagonal gates at the end should still be considered non-equivalent.
            """

        @remove_diagonal_gates_before_measure.setter
        def remove_diagonal_gates_before_measure(self, arg: bool, /) -> None: ...
        @property
        def transform_dynamic_circuit(self) -> bool:
            """Circuits containing dynamic circuit primitives such as mid-circuit measurements, resets, or classically-controlled operations cannot be verified in a straight-forward fashion due to the non-unitary nature of these primitives, which is why this setting defaults to :code:`False`.

            By enabling this optimization, any dynamic circuit is first transformed to a circuit without non-unitary primitives by, first, substituting qubit resets with new qubits and, then, applying the deferred measurement principle to defer measurements to the end.
            """

        @transform_dynamic_circuit.setter
        def transform_dynamic_circuit(self, arg: bool, /) -> None: ...
        @property
        def reorder_operations(self) -> bool:
            """The operations of a circuit are stored in a sequential container.

            This introduces some dependencies in the order of operations that are not naturally present in the quantum circuit.
            As a consequence, two quantum circuits that contain exactly the same operations, list their operations in different ways, also apply there operations in a different order.
            This optimization pass established a canonical ordering of operations by, first, constructing a directed, acyclic graph for the operations and, then, traversing it in a breadth-first fashion.

            Defaults to :code:`True`.
            """

        @reorder_operations.setter
        def reorder_operations(self, arg: bool, /) -> None: ...
        @property
        def backpropagate_output_permutation(self) -> bool:
            """Backpropagate the output permutation to the input permutation.

            Defaults to :code:`False` since this might mess up the initially given input permutation.
            Can be helpful for dynamic quantum circuits that have been transformed to a static circuit by enabling the :attr:`~.Configuration.Optimizations.transform_dynamic_circuit` optimization.
            """

        @backpropagate_output_permutation.setter
        def backpropagate_output_permutation(self, arg: bool, /) -> None: ...
        @property
        def elide_permutations(self) -> bool:
            """Elide permutations from the circuit by permuting the qubits in the circuit and eliminating SWAP gates from the circuits.

            Defaults to :code:`True` as this typically boosts performance.
            """

        @elide_permutations.setter
        def elide_permutations(self, arg: bool, /) -> None: ...

    class Application:
        """Options describing the :class:`.ApplicationScheme` used for the individual equivalence checkers."""

        def __init__(self) -> None: ...
        @property
        def construction_scheme(self) -> ApplicationScheme:
            """The :class:`.ApplicationScheme` used for the construction checker."""

        @construction_scheme.setter
        def construction_scheme(self, arg: ApplicationScheme, /) -> None: ...
        @property
        def simulation_scheme(self) -> ApplicationScheme:
            """The :class:`.ApplicationScheme` used for the simulation checker."""

        @simulation_scheme.setter
        def simulation_scheme(self, arg: ApplicationScheme, /) -> None: ...
        @property
        def alternating_scheme(self) -> ApplicationScheme:
            """The :class:`.ApplicationScheme` used for the alternating checker."""

        @alternating_scheme.setter
        def alternating_scheme(self, arg: ApplicationScheme, /) -> None: ...
        @property
        def profile(self) -> str:
            """The :attr:`Gate Cost <.ApplicationScheme.gate_cost>` application scheme can be configured with a profile that specifies the cost of gates.

            This profile can be set via a file constructed like a lookup table.
            Every line :code:`<GATE_ID> <N_CONTROLS> <COST>` specifies the cost for a given gate type and with a certain number of controls, e.g., :code:`X 0 1` denotes that a single-qubit X gate has a cost of :code:`1`, while :code:`X 2 15` denotes that a Toffoli gate has a cost of :code:`15`.
            """

        @profile.setter
        def profile(self, arg: str, /) -> None: ...

    class Functionality:
        """Options for all checkers that consider the whole functionality of a circuit."""

        def __init__(self) -> None: ...
        @property
        def trace_threshold(self) -> float:
            r"""While decision diagrams are canonical in theory, i.e., equivalent circuits produce equivalent decision diagrams, numerical inaccuracies and approximations can harm this property.

            This can result in a scenario where two decision diagrams are really close to one another, but cannot be identified as such by standard methods (i.e., comparing their root pointers).
            Instead, for two decision diagrams :code:`U` and :code:`U'` representing the functionalities of two circuits :code:`G` and :code:`G'`, the trace of the product of one decision diagram with the inverse of the other can be computed and compared to the trace of the identity.

            Alternatively, it can be checked, whether :code:`U*U`^-1` is \"close enough\" to the identity by recursively checking that each decision diagram node is close enough to the identity structure (i.e., the first and last successor have weights close to one, while the second and third successor have weights close to zero).
            Whenever any decision diagram node differs from this structure by more than the configured threshold, the circuits are concluded to be non-equivalent.

            Defaults to :code:`1e-8`.
            """

        @trace_threshold.setter
        def trace_threshold(self, arg: float, /) -> None: ...
        @property
        def check_partial_equivalence(self) -> bool:
            """Two circuits are partially equivalent if, for each possible initial input state, they have the same probability for each measurement outcome.

            If set to :code:`True`, a check for partial equivalence will be performed and the contributions of garbage qubits to the circuit are ignored.
            If set to :code:`False`, the checker will output 'not equivalent' for circuits that are partially equivalent but not totally equivalent.
            In particular, garbage qubits will be treated as if they were measured qubits.

            Defaults to :code:`False`.
            """

        @check_partial_equivalence.setter
        def check_partial_equivalence(self, arg: bool, /) -> None: ...

    class Simulation:
        """Options that influence the simulation checker."""

        def __init__(self) -> None: ...
        @property
        def fidelity_threshold(self) -> float:
            """Similar to :attr:`~.Configuration.Functionality.trace_threshold`, this setting is here to tackle numerical inaccuracies in the simulation checker.

            Instead of computing a trace, the fidelity between the states resulting from the simulation is computed.
            Whenever the fidelity differs from :code:`1.` by more than the configured threshold, the circuits are concluded to be non-equivalent.

            Defaults to :code:`1e-8`.
            """

        @fidelity_threshold.setter
        def fidelity_threshold(self, arg: float, /) -> None: ...
        @property
        def max_sims(self) -> int:
            """The maximum number of simulations to be started for the simulation checker.

            In practice, just a couple of simulations suffice in most cases to detect a potential non-equivalence.
            Either defaults to :code:`16` or the maximum number of available threads minus 2, whichever is more.
            """

        @max_sims.setter
        def max_sims(self, arg: int, /) -> None: ...
        @property
        def state_type(self) -> StateType:
            """The :class:`type of states <.StateType>` used for the simulations in the simulation checker.

            Defaults to :attr:`.StateType.computational_basis`.
            """

        @state_type.setter
        def state_type(self, arg: StateType, /) -> None: ...
        @property
        def seed(self) -> int:
            """The seed used in the quantum state generator.

            Defaults to :code:`0`, which means that the seed is chosen non-deterministically for each program run.
            """

        @seed.setter
        def seed(self, arg: int, /) -> None: ...

    class Parameterized:
        """Options that influence the equivalence checking scheme for parameterized circuits."""

        def __init__(self) -> None: ...
        @property
        def parameterized_tolerance(self) -> float:
            """Set threshold below which instantiated parameters shall be considered zero.

            Defaults to :code:`1e-12`.
            """

        @parameterized_tolerance.setter
        def parameterized_tolerance(self, arg: float, /) -> None: ...
        @property
        def additional_instantiations(self) -> int:
            """Number of instantiations shall be performed in addition to the default ones.

            For parameterized circuits that cannot be shown to be equivalent by the ZX checker the circuits are instantiated with concrete values for parameters and subsequently checked with QCEC's default schemes.
            The first instantiation tries to set as many gate parameters to 0.
            The last instantiations initializes the parameters with random values to guarantee completeness of the equivalence check.
            Because random instantiation is costly, additional instantiations can be performed that lead to simpler equivalence checking instances as the random instantiation.
            This option changes how many of those additional checks are performed.
            """

        @additional_instantiations.setter
        def additional_instantiations(self, arg: int, /) -> None: ...

    @property
    def execution(self) -> Configuration.Execution: ...
    @execution.setter
    def execution(self, arg: Configuration.Execution, /) -> None: ...
    @property
    def optimizations(self) -> Configuration.Optimizations: ...
    @optimizations.setter
    def optimizations(self, arg: Configuration.Optimizations, /) -> None: ...
    @property
    def application(self) -> Configuration.Application: ...
    @application.setter
    def application(self, arg: Configuration.Application, /) -> None: ...
    @property
    def functionality(self) -> Configuration.Functionality: ...
    @functionality.setter
    def functionality(self, arg: Configuration.Functionality, /) -> None: ...
    @property
    def simulation(self) -> Configuration.Simulation: ...
    @simulation.setter
    def simulation(self, arg: Configuration.Simulation, /) -> None: ...
    @property
    def parameterized(self) -> Configuration.Parameterized: ...
    @parameterized.setter
    def parameterized(self, arg: Configuration.Parameterized, /) -> None: ...
    def json(self) -> dict[str, Any]:
        """Returns a JSON-style dictionary of the configuration."""

class EquivalenceCheckingManager:
    """The main class of QCEC.

    Allows checking the equivalence of quantum circuits based on the methods proposed in :cite:p:`burgholzer2021advanced`.
    It features many configuration options that orchestrate the procedure.
    """

    def __init__(
        self, circ1: mqt.core.ir.QuantumComputation, circ2: mqt.core.ir.QuantumComputation, config: Configuration = ...
    ) -> None:
        """Create an equivalence checking manager for two circuits and configure it with a :class:`.Configuration` object."""

    class Results:
        """Captures the main results and statistics from :meth:`~.EquivalenceCheckingManager.run`."""

        def __init__(self) -> None:
            """Initializes the results."""

        @property
        def preprocessing_time(self) -> float:
            """Time spent during preprocessing (in seconds)."""

        @preprocessing_time.setter
        def preprocessing_time(self, arg: float, /) -> None: ...
        @property
        def check_time(self) -> float:
            """Time spent during equivalence check (in seconds)."""

        @check_time.setter
        def check_time(self, arg: float, /) -> None: ...
        @property
        def equivalence(self) -> EquivalenceCriterion:
            """Final result of the equivalence check."""

        @equivalence.setter
        def equivalence(self, arg: EquivalenceCriterion, /) -> None: ...
        @property
        def started_simulations(self) -> int:
            """Number of simulations that have been started."""

        @started_simulations.setter
        def started_simulations(self, arg: int, /) -> None: ...
        @property
        def performed_simulations(self) -> int:
            """Number of simulations that have been finished."""

        @performed_simulations.setter
        def performed_simulations(self, arg: int, /) -> None: ...
        @property
        def cex_input(self) -> mqt.core.dd.VectorDD:
            """DD representation of the initial state that produced a counterexample."""

        @cex_input.setter
        def cex_input(self, arg: mqt.core.dd.VectorDD, /) -> None: ...
        @property
        def cex_output1(self) -> mqt.core.dd.VectorDD:
            """DD representation of the first circuit's counterexample output state."""

        @cex_output1.setter
        def cex_output1(self, arg: mqt.core.dd.VectorDD, /) -> None: ...
        @property
        def cex_output2(self) -> mqt.core.dd.VectorDD:
            """DD representation of the second circuit's counterexample output state."""

        @cex_output2.setter
        def cex_output2(self, arg: mqt.core.dd.VectorDD, /) -> None: ...
        @property
        def performed_instantiations(self) -> int:
            """Number of circuit instantiations performed during equivalence checking of parameterized quantum circuits."""

        @performed_instantiations.setter
        def performed_instantiations(self, arg: int, /) -> None: ...
        @property
        def checker_results(self) -> dict[str, Any]:
            """Dictionary of the results of the individual checkers."""

        @checker_results.setter
        def checker_results(self, arg: dict, /) -> None: ...
        def considered_equivalent(self) -> bool:
            """Convenience function to check whether the result is considered equivalent."""

        def json(self) -> dict[str, Any]:
            """Returns a JSON-style dictionary of the results."""

    @property
    def qc1(self) -> mqt.core.ir.QuantumComputation:
        """The first circuit to be checked."""

    @property
    def qc2(self) -> mqt.core.ir.QuantumComputation:
        """The second circuit to be checked."""

    @property
    def configuration(self) -> Configuration:
        """The configuration of the equivalence checking manager."""

    @configuration.setter
    def configuration(self, arg: Configuration, /) -> None: ...
    def run(self) -> None:
        """Execute the equivalence check as configured."""

    @property
    def results(self) -> EquivalenceCheckingManager.Results:
        """The results of the equivalence check."""

    @property
    def equivalence(self) -> EquivalenceCriterion:
        """The :class:`.EquivalenceCriterion` determined as the result of the equivalence check."""

    def disable_all_checkers(self) -> None:
        """Disable all equivalence checkers."""

    def set_application_scheme(self, scheme: ApplicationScheme = ...) -> None:
        """Set the :class:`.ApplicationScheme` used for all checkers (based on decision diagrams).

        Args:
            scheme: The application scheme. Defaults to :attr:`.ApplicationScheme.proportional`.
        """

    def set_gate_cost_profile(self, profile: str = "") -> None:
        """Set the :attr:`profile <.Configuration.Application.profile>` used in the :attr:`Gate Cost <.ApplicationScheme.gate_cost>` application scheme for all checkers (based on decision diagrams).

        Args:
            profile: The path to the profile file.
        """

class EquivalenceCriterion(enum.IntEnum):
    """Captures all the different notions of equivalence that can be the result of a :meth:`~.EquivalenceCheckingManager.run`."""

    no_information = 2
    """
    No information on the equivalence is available.

    This can be because the check has not been run or that a timeout happened.
    """

    not_equivalent = 0
    """Circuits are shown to be non-equivalent."""

    equivalent = 1
    """Circuits are shown to be equivalent."""

    equivalent_up_to_phase = 5
    """Circuits are equivalent up to a global phase factor."""

    equivalent_up_to_global_phase = 4
    """Circuits are equivalent up to a certain (global or relative) phase."""

    probably_equivalent = 3
    """
    Circuits are probably equivalent.

    A result obtained whenever a couple of simulations did not show the non-equivalence in the simulation checker.
    """

    probably_not_equivalent = 6
    """
    Circuits are probably not equivalent.

    A result obtained whenever the ZX-calculus checker could not reduce the combined circuit to the identity.
    """

class StateType(enum.IntEnum):
    """The type of states used in the simulation checker allows trading off efficiency versus performance.

    - Classical stimuli (i.e., random *computational basis states*) already offer extremely high error detection rates in general and are comparatively fast to simulate, which makes them the default.
    - Local quantum stimuli (i.e., random *single-qubit basis states*) are a little bit more computationally intensive, but provide even better error detection rates.
    - Global quantum stimuli (i.e., random  *stabilizer states*) offer the highest available error detection rate, while at the same time incurring the highest computational effort.

    For details, see :cite:p:`burgholzer2021randomStimuliGenerationQuantum`.
    """

    computational_basis = 0
    """
    Randomly choose computational basis states. Also referred to as "*classical*".
    """

    classical = 0
    """Alias for :attr:`~StateType.computational_basis`."""

    random_1q_basis = 1
    """
    Randomly choose a single-qubit basis state for each qubit from the six-tuple *(|0>, |1>, |+>, |->, |L>, |R>)*. Also referred to as *"local_random"*.
    """

    local_quantum = 1
    """Alias for :attr:`~StateType.random_1q_basis`."""

    stabilizer = 2
    """
    Randomly choose a stabilizer state by creating a random Clifford circuit. Also referred to as *"global_random"*.
    """

    global_quantum = 2
    """Alias for :attr:`~StateType.stabilizer`."""
