# Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Python bindings module for :class:`~.StateType`."""

from typing import ClassVar, Literal, overload

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
