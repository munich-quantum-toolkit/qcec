# Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Python bindings module for :class:`~.EquivalenceCriterion`."""

from typing import ClassVar, Literal, overload

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
