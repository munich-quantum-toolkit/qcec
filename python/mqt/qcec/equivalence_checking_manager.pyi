# Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Python bindings module for :class:`~.EquivalenceCheckingManager`."""

from typing import Any

from mqt.core.dd import VectorDD
from mqt.core.ir import QuantumComputation
from mqt.qcec.application_scheme import ApplicationScheme
from mqt.qcec.configuration import Configuration
from mqt.qcec.equivalence_criterion import EquivalenceCriterion

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
