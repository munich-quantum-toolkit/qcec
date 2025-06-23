# Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""The main entry point for the QCEC package."""

from __future__ import annotations

from typing import TYPE_CHECKING

from mqt.core import load

from .configuration_options import augment_config_from_kwargs
from .parameterized import check_parameterized
from .pyqcec import Configuration, EquivalenceCheckingManager

if TYPE_CHECKING:
    import os

    from mqt.core.ir import QuantumComputation
    from qiskit.circuit import QuantumCircuit

    from ._compat.typing import Unpack
    from .configuration_options import ConfigurationOptions

__all__ = ["verify"]


def __dir__() -> list[str]:
    return __all__


def verify(
    circ1: QuantumComputation | str | os.PathLike[str] | QuantumCircuit,
    circ2: QuantumComputation | str | os.PathLike[str] | QuantumCircuit,
    configuration: Configuration | None = None,
    **kwargs: Unpack[ConfigurationOptions],
) -> EquivalenceCheckingManager.Results:
    """Verify that ``circ1`` and ``circ2`` are equivalent.

    Wraps creating an instance of :class:`EquivalenceCheckingManager <.EquivalenceCheckingManager>`,
    calling :meth:`.EquivalenceCheckingManager.run`,
    and returning :attr:`.EquivalenceCheckingManager.results`.

    There are two (non-exclusive) ways of configuring the equivalence checking process:

    1. Pass a :class:`.Configuration` instance as the ``configuration`` argument.

    2. Pass keyword arguments to this function. These are directly incorporated into the :class:`.Configuration`.
    Any existing configuration is overridden by keyword arguments.

    Args:
        circ1: The first circuit.
        circ2: The second circuit.
        configuration: The configuration to use for the equivalence checking process.
        **kwargs: Keyword arguments to configure the equivalence checking process.

    Returns:
        The results of the equivalence checking process.
    """
    if configuration is None:
        configuration = Configuration()

    # prepare the configuration
    augment_config_from_kwargs(configuration, kwargs)

    # load the circuits
    qc1 = load(circ1)
    qc2 = load(circ2)

    if not qc1.is_variable_free() or not qc2.is_variable_free():
        return check_parameterized(qc1, qc2, configuration)

    # create the equivalence checker from configuration
    ecm = EquivalenceCheckingManager(qc1, qc2, configuration)

    # execute the check
    ecm.run()

    # obtain the result
    return ecm.results
