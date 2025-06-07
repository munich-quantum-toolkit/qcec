# Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Python bindings module for :class:`~.ApplicationScheme`."""

from typing import ClassVar, Literal, overload

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
