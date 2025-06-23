# Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Verify compilation flow results."""

from __future__ import annotations

import warnings
from typing import TYPE_CHECKING

from mqt.core import load

from ._compat.importlib import resources
from .compilation_flow_profiles import AncillaMode, generate_profile_name
from .configuration_options import augment_config_from_kwargs
from .pyqcec import ApplicationScheme, Configuration
from .verify import verify

if TYPE_CHECKING:
    import os

    from mqt.core.ir import QuantumComputation
    from qiskit.circuit import QuantumCircuit

    from ._compat.typing import Unpack
    from .configuration_options import ConfigurationOptions
    from .pyqcec import EquivalenceCheckingManager

__all__ = ["verify_compilation"]


def __dir__() -> list[str]:
    return __all__


def __check_if_circuit_contains_measurements(circuit: QuantumComputation) -> None:
    """Check if the circuit contains measurements and emits a warning if it does not.

    Args:
        circuit: The circuit to check.
    """
    for op in circuit:
        if op.name == "measure":
            return

    warnings.warn(
        UserWarning(
            "One of the circuits does not contain any measurements. "
            "This may lead to unexpected results since the measurements are used "
            "to infer the output qubit permutation at the end of the circuit. "
            "Please consider adding measurements to the circuit _before_ compilation."
        ),
        stacklevel=2,
    )


def verify_compilation(
    original_circuit: QuantumComputation | str | os.PathLike[str] | QuantumCircuit,
    compiled_circuit: QuantumComputation | str | os.PathLike[str] | QuantumCircuit,
    optimization_level: int = 1,
    ancilla_mode: AncillaMode = AncillaMode.NO_ANCILLA,
    configuration: Configuration | None = None,
    **kwargs: Unpack[ConfigurationOptions],
) -> EquivalenceCheckingManager.Results:
    """Verify compilation flow results.

    Similar to :func:`verify <.verify>`, but uses a dedicated compilation flow profile to guide the equivalence checking process.
    The compilation flow profile is determined by the ``optimization_level`` and ``ancilla_mode`` arguments.

    There are two (non-exclusive) ways of configuring the equivalence checking process:

    1. Pass a :class:`Configuration <.Configuration>` instance as the ``configuration`` argument.

    2. Pass keyword arguments to this function. These are directly incorporated into the :class:`Configuration <.Configuration>`.
    Any existing configuration is overridden by keyword arguments.

    Args:
        original_circuit: The original circuit.
        compiled_circuit: The compiled circuit.
        optimization_level: The optimization level used for compiling the circuit (0, 1, 2, or 3). Defaults to 1.
        ancilla_mode:
            The :class:`ancilla mode <.AncillaMode>` used for realizing multi-controlled Toffoli gates, as available in Qiskit.
            Defaults to :attr:`.AncillaMode.NO_ANCILLA`.
        configuration: The configuration to use for the equivalence checking process.
        **kwargs: Keyword arguments to configure the equivalence checking process.

    Returns:
        The results of the equivalence checking process.

    .. warning::
        Qiskit has deprecated the ``mode`` argument of ``QuantumCircuit.mcx()`` with version 2.1.
        In accordance with this, ``mqt.qcec`` has deprecated the ``ancilla_mode`` argument as well.
        The argument will be removed in a future release.
    """
    if ancilla_mode != AncillaMode.NO_ANCILLA:
        warnings.warn(
            "Qiskit has deprecated the ``mode`` argument of ``QuantumCircuit.mcx()`` method with version 2.1. "
            "In accordance with this, ``mqt.qcec`` has deprecated the ``ancilla_mode`` argument of ``verify_compilation()`` as well. "
            "The argument will be removed in a future release.",
            DeprecationWarning,
            stacklevel=2,
        )

    if configuration is None:
        configuration = Configuration()

    # prepare the configuration
    augment_config_from_kwargs(configuration, kwargs)

    # load the circuits
    qc1 = load(original_circuit)
    __check_if_circuit_contains_measurements(qc1)
    qc2 = load(compiled_circuit)
    __check_if_circuit_contains_measurements(qc2)

    # use the gate_cost scheme for the verification
    configuration.application.construction_scheme = ApplicationScheme.gate_cost
    configuration.application.simulation_scheme = ApplicationScheme.gate_cost
    configuration.application.alternating_scheme = ApplicationScheme.gate_cost

    # get the pre-defined profile for the gate_cost scheme
    profile_name = generate_profile_name(optimization_level=optimization_level, mode=ancilla_mode)
    ref = resources.files("mqt.qcec") / "profiles" / profile_name
    with resources.as_file(ref) as path:
        configuration.application.profile = str(path)

    return verify(qc1, qc2, configuration=configuration)
