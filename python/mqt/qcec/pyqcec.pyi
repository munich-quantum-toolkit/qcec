# Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Python bindings module for QCEC."""

from typing import Any, ClassVar, Literal, overload

from mqt.core.dd import VectorDD
from mqt.core.ir import QuantumComputation

class ApplicationScheme:
    """Describes the order in which the individual operations of both circuits are applied during the equivalence check.

    In case of the alternating equivalence checker, this is the key component to allow the intermediate decision diagrams to remain close to the identity (as proposed in :cite:p:`burgholzer2021advanced`).
    See :doc:`/compilation_flow_verification` for more information on the dedicated application scheme for verifying compilation flow results (as proposed in :cite:p:`burgholzer2020verifyingResultsIBM`).

    In case of the other checkers, which consider both circuits individually, using a non-sequential application scheme can significantly boost the operation caching performance in the underlying decision diagram package.
    """

    sequential: ClassVar[ApplicationScheme] = ...
    """Applies all gates from the first circuit, before proceeding with the second circuit.

     Referred to as *"reference"* in :cite:p:`burgholzer2021advanced`.
     """

    one_to_one: ClassVar[ApplicationScheme] = ...
    """Alternates between applications from the first and the second circuit.

    Referred to as *"naive"* in :cite:p:`burgholzer2021advanced`.
    """

    proportional: ClassVar[ApplicationScheme] = ...
    """Alternates between applications from the first and the second circuit, but applies the gates in proportion to the number of gates in each circuit.
    """

    lookahead: ClassVar[ApplicationScheme] = ...
    """Looks whether an application from the first circuit or the second circuit yields the smaller decision diagram.

    Only works for the alternating equivalence checker.
    """

    gate_cost: ClassVar[ApplicationScheme] = ...
    """
    Each gate of the first circuit is associated with a corresponding cost according to some cost function *f(...)*.
    Whenever a gate *g* from the first circuit is applied *f(g)* gates are applied from the second circuit.

    Referred to as *"compilation_flow"* in :cite:p:`burgholzer2020verifyingResultsIBM`.
    """

    @overload
    def __init__(self, value: int) -> None: ...
    @overload
    def __init__(
        self, scheme: Literal["sequential", "one_to_one", "proportional", "lookahead", "gate_cost"]
    ) -> None: ...
    @overload
    def __init__(self, scheme: str) -> None: ...

    __members__: ClassVar[dict[ApplicationScheme, int]] = ...  # read-only
    def name(self) -> str: ...
    def __eq__(self, other: object) -> bool: ...
    def __getstate__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __index__(self) -> int: ...
    def __int__(self) -> int: ...
    def __ne__(self, other: object) -> bool: ...
    def __setstate__(self, state: int) -> None: ...
    @property
    def value(self) -> int: ...

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

    class Execution:
        """Options that orchestrate the :meth:`~.EquivalenceCheckingManager.run` method."""

        parallel: bool = True
        """Set whether execution should happen in parallel. Defaults to :code:`True`."""

        nthreads: int
        """Set the maximum number of threads to use. Defaults to the maximum number of available threads reported by the OS."""

        timeout: float = 0.0
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

        run_construction_checker: bool = False
        """Set whether the construction checker should be executed.

        Defaults to :code:`False` since the alternating checker is to be preferred in most cases.
        """

        run_simulation_checker: bool = True
        """Set whether the simulation checker should be executed.

        Defaults to :code:`True` since simulations can quickly show the non-equivalence of circuits in many cases.
        """

        run_alternating_checker: bool = True
        """
        Set whether the alternating checker should be executed.

        Defaults to :code:`True` since staying close to the identity can quickly show the equivalence of circuits in many cases.
        """

        run_zx_checker: bool = True
        """Set whether the ZX-calculus checker should be executed.

        Defaults to :code:`True` but arbitrary multi-controlled operations are only partially supported.
        """

        numerical_tolerance: float = 2e-13
        """
        Set the numerical tolerance of the underlying decision diagram package.

        Defaults to :code:`2e-13` and should only be changed by users who know what they are doing.
        """

        set_all_ancillae_garbage: bool = False
        """
        Set whether all ancillae should be treated as garbage qubits.

        Defaults to :code:`False` but the ZX-calculus checker will not be able to handle circuits with non-garbage ancillae.
        """

        def __init__(self) -> None: ...

    class Optimizations:
        """Options that influence which circuit optimizations are applied during pre-processing."""

        fuse_single_qubit_gates: bool = True
        """Fuse consecutive single-qubit gates by grouping them together.

        Defaults to :code:`True` as this typically increases the performance of the subsequent equivalence check.
        """

        reconstruct_swaps: bool = True
        """Try to reconstruct SWAP gates that have been decomposed (into a sequence of 3 CNOT gates) or optimized away (as a consequence of a SWAP preceded or followed by a CNOT on the same qubits).

        Defaults to :code:`True` since this reconstruction enables the efficient tracking of logical to physical qubit permutations throughout circuits that have been mapped to a target architecture.
        """

        remove_diagonal_gates_before_measure: bool = False
        """Remove any diagonal gates at the end of the circuit.

        This might be desirable since any diagonal gate in front of a measurement does not influence the probabilities of the respective states.

        Defaults to :code:`False` since, in general, circuits differing by diagonal gates at the end should still be considered non-equivalent.
        """

        transform_dynamic_circuit: bool = False
        """Circuits containing dynamic circuit primitives such as mid-circuit measurements, resets, or classically-controlled operations cannot be verified in a straight-forward fashion due to the non-unitary nature of these primitives, which is why this setting defaults to :code:`False`.

        By enabling this optimization, any dynamic circuit is first transformed to a circuit without non-unitary primitives by, first, substituting qubit resets with new qubits and, then, applying the deferred measurement principle to defer measurements to the end.
        """

        reorder_operations: bool = True
        """The operations of a circuit are stored in a sequential container.
        This introduces some dependencies in the order of operations that are not naturally present in the quantum circuit.
        As a consequence, two quantum circuits that contain exactly the same operations, list their operations in different ways, also apply there operations in a different order.
        This optimization pass established a canonical ordering of operations by, first, constructing a directed, acyclic graph for the operations and, then, traversing it in a breadth-first fashion.

        Defaults to :code:`True`.
        """

        backpropagate_output_permutation: bool = False
        """Backpropagate the output permutation to the input permutation.

        Defaults to :code:`False` since this might mess up the initially given input permutation.
        Can be helpful for dynamic quantum circuits that have been transformed to a static circuit by enabling the :attr:`~.Configuration.Optimizations.transform_dynamic_circuit` optimization.
        """

        elide_permutations: bool = True
        """Elide permutations from the circuit by permuting the qubits in the circuit and eliminating SWAP gates from the circuits.

        Defaults to :code:`True` as this typically boosts performance.
        """

        def __init__(self) -> None: ...

    class Application:
        """Options describing the :class:`.ApplicationScheme` used for the individual equivalence checkers."""

        construction_scheme: ApplicationScheme
        """The :class:`.ApplicationScheme` used for the construction checker."""

        simulation_scheme: ApplicationScheme
        """The :class:`.ApplicationScheme` used for the simulation checker."""

        alternating_scheme: ApplicationScheme
        """The :class:`.ApplicationScheme` used for the alternating checker."""

        profile: str
        """The :attr:`Gate Cost <.ApplicationScheme.gate_cost>` application scheme can be configured with a profile that specifies the cost of gates.
        This profile can be set via a file constructed like a lookup table.
        Every line :code:`<GATE_ID> <N_CONTROLS> <COST>` specifies the cost for a given gate type and with a certain number of controls, e.g., :code:`X 0 1` denotes that a single-qubit X gate has a cost of :code:`1`, while :code:`X 2 15` denotes that a Toffoli gate has a cost of :code:`15`.
        """

        def __init__(self) -> None: ...

    class Functionality:
        """Options for all checkers that consider the whole functionality of a circuit."""

        trace_threshold: float = 1e-8
        """While decision diagrams are canonical in theory, i.e., equivalent circuits produce equivalent decision diagrams, numerical inaccuracies and approximations can harm this property.

        This can result in a scenario where two decision diagrams are really close to one another, but cannot be identified as such by standard methods (i.e., comparing their root pointers).
        Instead, for two decision diagrams :code:`U` and :code:`U'` representing the functionalities of two circuits :code:`G` and :code:`G'`, the trace of the product of one decision diagram with the inverse of the other can be computed and compared to the trace of the identity.

        Alternatively, it can be checked, whether :code:`U*U`^-1` is \"close enough\" to the identity by recursively checking that each decision diagram node is close enough to the identity structure (i.e., the first and last successor have weights close to one, while the second and third successor have weights close to zero).
        Whenever any decision diagram node differs from this structure by more than the configured threshold, the circuits are concluded to be non-equivalent.

        Defaults to :code:`1e-8`.
        """

        check_partial_equivalence: bool = False
        """Two circuits are partially equivalent if, for each possible initial input state, they have the same probability for each measurement outcome.
        If set to :code:`True`, a check for partial equivalence will be performed and the contributions of garbage qubits to the circuit are ignored.
        If set to :code:`False`, the checker will output 'not equivalent' for circuits that are partially equivalent but not totally equivalent.
        In particular, garbage qubits will be treated as if they were measured qubits.

        Defaults to :code:`False`.
        """

        def __init__(self) -> None: ...

    class Simulation:
        """Options that influence the simulation checker."""

        fidelity_threshold: float
        """Similar to :attr:`~.Configuration.Functionality.trace_threshold`, this setting is here to tackle numerical inaccuracies in the simulation checker.
        Instead of computing a trace, the fidelity between the states resulting from the simulation is computed.
        Whenever the fidelity differs from :code:`1.` by more than the configured threshold, the circuits are concluded to be non-equivalent.

        Defaults to :code:`1e-8`.
        """

        max_sims: int
        """The maximum number of simulations to be started for the simulation checker.

        In practice, just a couple of simulations suffice in most cases to detect a potential non-equivalence.
        Either defaults to :code:`16` or the maximum number of available threads minus 2, whichever is more.
        """

        state_type: StateType = ...
        """The :class:`type of states <.StateType>` used for the simulations in the simulation checker.

        Defaults to :attr:`.StateType.computational_basis`.
        """

        seed: int = 0
        """The seed used in the quantum state generator.

        Defaults to :code:`0`, which means that the seed is chosen non-deterministically for each program run.
        """

        def __init__(self) -> None: ...

    class Parameterized:
        """Options that influence the equivalence checking scheme for parameterized circuits."""

        parameterized_tolerance: float = 1e-12
        """Set threshold below which instantiated parameters shall be considered zero.

        Defaults to :code:`1e-12`.
        """

        additional_instantiations: int = 0
        """Number of instantiations shall be performed in addition to the default ones.

        For parameterized circuits that cannot be shown to be equivalent by the ZX checker the circuits are instantiated with concrete values for parameters and subsequently checked with QCEC's default schemes.
        The first instantiation tries to set as many gate parameters to 0.
        The last instantiations initializes the parameters with random values to guarantee completeness of the equivalence check.
        Because random instantiation is costly, additional instantiations can be performed that lead to simpler equivalence checking instances as the random instantiation.
        This option changes how many of those additional checks are performed.
        """

        def __init__(self) -> None: ...

    application: Configuration.Application
    execution: Configuration.Execution
    functionality: Configuration.Functionality
    optimizations: Configuration.Optimizations
    parameterized: Configuration.Parameterized
    simulation: Configuration.Simulation
    def __init__(self) -> None:
        """Initializes the configuration."""

    def json(self) -> dict[str, Any]:
        """Returns a JSON-style dictionary of the configuration."""

class EquivalenceCheckingManager:
    """The main class of QCEC.

    Allows checking the equivalence of quantum circuits based on the methods proposed in :cite:p:`burgholzer2021advanced`.
    It features many configuration options that orchestrate the procedure.
    """
    def __init__(self, circ1: QuantumComputation, circ2: QuantumComputation, config: Configuration = ...) -> None:
        """Create an equivalence checking manager for two circuits and configure it with a :class:`.Configuration` object."""

    @property
    def qc1(self) -> QuantumComputation:
        """The first circuit to be checked."""

    @property
    def qc2(self) -> QuantumComputation:
        """The second circuit to be checked."""

    @property
    def configuration(self) -> Configuration: ...
    @configuration.setter
    def configuration(self, config: Configuration) -> None:
        """The configuration of the equivalence checking manager."""

    def run(self) -> None:
        """Execute the equivalence check as configured."""

    @property
    def results(self) -> Results:
        """The results of the equivalence check."""

    @property
    def equivalence(self) -> EquivalenceCriterion:
        """The :class:`.EquivalenceCriterion` determined as the result of the equivalence check."""

    def disable_all_checkers(self) -> None:
        """Disable all equivalence checkers."""

    def set_application_scheme(self, scheme: ApplicationScheme = ...) -> None:
        """Set the :class:`.ApplicationScheme` used for all checkers (based on decision diagrams).

        Arguments:
            scheme: The application scheme. Defaults to :attr:`.ApplicationScheme.proportional`.
        """

    def set_gate_cost_profile(self, profile: str = "") -> None:
        """Set the :attr:`profile <.Configuration.Application.profile>` used in the :attr:`Gate Cost <.ApplicationScheme.gate_cost>` application scheme for all checkers (based on decision diagrams).

        Arguments:
            profile: The path to the profile file.
        """

    class Results:
        """Captures the main results and statistics from :meth:`~.EquivalenceCheckingManager.run`."""

        preprocessing_time: float
        """Time spent during preprocessing (in seconds)."""

        check_time: float
        """Time spent during equivalence check (in seconds)."""

        equivalence: EquivalenceCriterion
        """Final result of the equivalence check."""

        started_simulations: int
        """Number of simulations that have been started."""

        performed_simulations: int
        """Number of simulations that have been finished."""

        cex_input: VectorDD
        """DD representation of the initial state that produced a counterexample."""

        cex_output1: VectorDD
        """DD representation of the first circuit's counterexample output state."""

        cex_output2: VectorDD
        """DD representation of the second circuit's counterexample output state."""

        performed_instantiations: int
        """Number of circuit instantiations performed during equivalence checking of parameterized quantum circuits."""

        checker_results: dict[str, Any]
        """Dictionary of the results of the individual checkers."""

        def __init__(self) -> None:
            """Initializes the results."""

        def considered_equivalent(self) -> bool:
            """Convenience function to check whether the result is considered equivalent."""

        def json(self) -> dict[str, Any]:
            """Returns a JSON-style dictionary of the results."""

class EquivalenceCriterion:
    """Captures all the different notions of equivalence that can be the result of a :meth:`~.EquivalenceCheckingManager.run`."""

    no_information: ClassVar[EquivalenceCriterion] = ...
    """No information on the equivalence is available.
    This can be because the check has not been run or that a timeout happened."""

    not_equivalent: ClassVar[EquivalenceCriterion] = ...
    """Circuits are shown to be non-equivalent."""

    equivalent: ClassVar[EquivalenceCriterion] = ...
    """Circuits are shown to be equivalent."""

    equivalent_up_to_global_phase: ClassVar[EquivalenceCriterion] = ...
    """Circuits are equivalent up to a global phase factor."""

    equivalent_up_to_phase: ClassVar[EquivalenceCriterion] = ...
    """Circuits are equivalent up to a certain (global or relative) phase."""

    probably_equivalent: ClassVar[EquivalenceCriterion] = ...
    """Circuits are probably equivalent.
    A result obtained whenever a couple of simulations did not show the non-equivalence in the simulation checker."""

    probably_not_equivalent: ClassVar[EquivalenceCriterion] = ...
    """Circuits are probably not equivalent.
    A result obtained whenever the ZX-calculus checker could not reduce the combined circuit to the identity."""

    __members__: ClassVar[dict[EquivalenceCriterion, int]] = ...  # read-only

    @overload
    def __init__(self, value: int) -> None: ...
    @overload
    def __init__(
        self,
        criterion: Literal[
            "no_information",
            "not_equivalent",
            "equivalent",
            "equivalent_up_to_phase",
            "equivalent_up_to_global_phase",
            "probably_equivalent",
            "probably_not_equivalent",
        ],
    ) -> None: ...
    @overload
    def __init__(self, criterion: str) -> None: ...
    def name(self) -> str: ...
    def __eq__(self, other: object) -> bool: ...
    def __getstate__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __index__(self) -> int: ...
    def __int__(self) -> int: ...
    def __ne__(self, other: object) -> bool: ...
    def __setstate__(self, state: int) -> None: ...
    @property
    def value(self) -> int: ...

class StateType:
    """The type of states used in the simulation checker allows trading off efficiency versus performance.

    * Classical stimuli (i.e., random *computational basis states*) already offer extremely high error detection rates in general and are comparatively fast to simulate, which makes them the default.

    * Local quantum stimuli (i.e., random *single-qubit basis states*) are a little bit more computationally intensive, but provide even better error detection rates.

    * Global quantum stimuli (i.e., random  *stabilizer states*) offer the highest available error detection rate, while at the same time incurring the highest computational effort.

    For details, see :cite:p:`burgholzer2021randomStimuliGenerationQuantum`.
    """

    computational_basis: ClassVar[StateType] = ...
    """Randomly choose computational basis states. Also referred to as "*classical*."""

    random_1Q_basis: ClassVar[StateType] = ...  # noqa: N815
    """Randomly choose a single-qubit basis state for each qubit from the six-tuple *(|0>, |1>, |+>, |->, |L>, |R>)*. Also referred to as *local_random*."""

    stabilizer: ClassVar[StateType] = ...
    """Randomly choose a stabilizer state by creating a random Clifford circuit. Also referred to as *global_random*."""

    __members__: ClassVar[dict[StateType, int]] = ...  # read-only

    @overload
    def __init__(self, value: int) -> None: ...
    @overload
    def __init__(self, state_type: Literal["computational_basis", "random_1Q_basis", "stabilizer"]) -> None: ...
    @overload
    def __init__(self, state_type: str) -> None: ...
    def name(self) -> str: ...
    def __eq__(self, other: object) -> bool: ...
    def __getstate__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __index__(self) -> int: ...
    def __int__(self) -> int: ...
    def __ne__(self, other: object) -> bool: ...
    def __setstate__(self, state: int) -> None: ...
    @property
    def value(self) -> int: ...
