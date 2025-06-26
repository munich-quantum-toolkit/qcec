# Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Module for generating compilation flow profiles for the equivalence checking process."""

from __future__ import annotations

import warnings
from enum import Enum, unique
from pathlib import Path
from typing import TYPE_CHECKING, Any

import numpy as np

from ._compat.optional import HAS_QISKIT

if TYPE_CHECKING:
    from numpy.typing import NDArray
    from qiskit.circuit import QuantumCircuit

__all__ = [
    "AncillaMode",
    "generate_profile",
    "generate_profile_name",
]


def __dir__() -> list[str]:
    return __all__


@unique
class AncillaMode(Enum):
    """Enum for the ancilla mode."""

    NO_ANCILLA = "noancilla"
    """No ancilla qubits are used."""
    RECURSION = "recursion"
    """A single ancilla is used in a recursive manner."""
    V_CHAIN = "v-chain"
    """A chain of ancilla qubits is used."""

    def __eq__(self, other: object) -> bool:
        """Check if two AncillaMode objects are equal. Supports string comparison."""
        if isinstance(other, str):
            return self.value == other
        if isinstance(other, self.__class__):
            return self.value == other.value
        return False

    def __hash__(self) -> int:
        """Return the hash of the AncillaMode."""
        return hash(self.value)

    def __str__(self) -> str:
        """Return the string representation of the AncillaMode."""
        return self.value


single_qubit_gates_no_params = {
    "qubits": 1,
    "params": 0,
    "controls": 0,
    "gates": ["x", "y", "z", "h", "s", "sdg", "t", "tdg", "sx", "sxdg"],
}
single_qubit_gates_one_param = {
    "qubits": 1,
    "params": 1,
    "controls": 0,
    "gates": ["p", "rx", "ry", "rz"],
}
single_qubit_gates_three_params = {
    "qubits": 1,
    "params": 3,
    "controls": 0,
    "gates": ["u"],
}

two_qubit_gates_no_params = {"qubits": 2, "params": 0, "controls": 0, "gates": ["swap", "iswap", "dcx", "ecr"]}

two_qubit_gates_one_param = {
    "qubits": 2,
    "params": 1,
    "controls": 0,
    "gates": ["rxx", "ryy", "rzz", "rzx"],
}

single_controlled_single_qubit_gates_no_params = {
    "qubits": 1,
    "params": 0,
    "controls": 1,
    "gates": ["x", "y", "z", "h", "sx"],
}
single_controlled_single_qubit_gates_one_param = {
    "qubits": 1,
    "params": 1,
    "controls": 1,
    "gates": ["p", "rx", "ry", "rz"],
}
single_controlled_single_qubit_gates_three_params = {
    "qubits": 1,
    "params": 4,
    "controls": 1,
    "gates": ["u"],
}

single_controlled_two_qubit_gates = {
    "qubits": 2,
    "params": 0,
    "controls": 1,
    "gates": ["swap"],
}

double_controlled_single_qubit_gates_no_params = {
    "qubits": 1,
    "params": 0,
    "controls": 2,
    "gates": ["x"],
}

max_controls = 11
control_range = range(2, max_controls + 1)
mcx_no_ancilla = {
    "qubits": 1,
    "params": 0,
    "controls": control_range,
    "mode": AncillaMode.NO_ANCILLA,
    "ancilla_qubits": 0,
    "gates": ["x"],
}
mcx_recursion = {
    "qubits": 1,
    "params": 0,
    "controls": control_range,
    "mode": AncillaMode.RECURSION,
    "ancilla_qubits": 1,
    "gates": ["x"],
}
mcx_v_chain = {
    "qubits": 1,
    "params": 0,
    "controls": control_range,
    "mode": AncillaMode.V_CHAIN,
    "ancilla_qubits": None,
    "gates": ["x"],
}
mcphase = {
    "qubits": 1,
    "params": 1,
    "controls": control_range,
    "mode": None,
    "ancilla_qubits": 0,
    "gates": ["p"],
}

general_gates = [
    single_qubit_gates_no_params,
    single_qubit_gates_one_param,
    single_qubit_gates_three_params,
    two_qubit_gates_no_params,
    two_qubit_gates_one_param,
    single_controlled_single_qubit_gates_no_params,
    single_controlled_single_qubit_gates_one_param,
    single_controlled_single_qubit_gates_three_params,
    single_controlled_two_qubit_gates,
    double_controlled_single_qubit_gates_no_params,
]

multi_controlled_gates = [mcphase]
multi_controlled_gates_no_ancilla = [mcx_no_ancilla]
multi_controlled_gates_recursion = [mcx_recursion]
multi_controlled_gates_v_chain = [mcx_v_chain]


def __create_general_gate(qubits: int, params: int, controls: int, identifier: str) -> QuantumCircuit:
    """Create a ``QuantumCircuit`` containing a single gate ``identifier`` with the given number of ``qubits``, ``params``, and ``controls``."""
    from qiskit.circuit import QuantumCircuit

    required_qubits = qubits + controls
    qc = QuantumCircuit(required_qubits)
    gate_identifier = "c" * controls + identifier

    rng = np.random.default_rng(seed=12345)
    parameter_list = [rng.uniform(-np.pi, np.pi) for _ in range(params)]

    getattr(qc, gate_identifier)(*parameter_list, *range(required_qubits))
    return qc


def __create_multi_controlled_gate(
    qubits: int,
    params: int,
    controls: int,
    mode: AncillaMode | None,
    ancilla_qubits: int | None,
    identifier: str,
) -> QuantumCircuit:
    """Create a ``QuantumCircuit`` containing a single multi-controlled gate ``identifier`` with the given number of ``qubits``, ``params``, and ``controls`` using ``ancilla_qubits`` ancilla qubits and the given ancilla ``mode``."""
    from qiskit.circuit import QuantumCircuit

    required_qubits = qubits + controls

    # special handling for v-chain mode which is indicated by the ancilla_qubits being None
    if ancilla_qubits is None:
        ancilla_qubits = max(0, controls - 2)

    # special handling for recursion mode with less than 5 controls,
    # which does not require ancilla qubits
    no_ancilla_threshold = 5
    if mode == "recursion" and controls < no_ancilla_threshold:
        ancilla_qubits = 0

    required_qubits += ancilla_qubits
    qc = QuantumCircuit(required_qubits)
    gate_identifier = "mc" + identifier

    rng = np.random.default_rng(seed=12345)
    parameter_list = [rng.uniform(-np.pi, np.pi) for _ in range(params)]

    if mode is not None:
        getattr(qc, gate_identifier)(
            *parameter_list,
            control_qubits=list(range(controls)),
            target_qubit=controls,
            ancilla_qubits=list(range(controls + 1, controls + 1 + ancilla_qubits)),
            mode=mode,
        )
    else:
        getattr(qc, gate_identifier)(*parameter_list, control_qubits=list(range(controls)), target_qubit=controls)
    return qc


def __compute_cost(
    qc: QuantumCircuit,
    basis_gates: list[str],
    optimization_level: int = 1,
) -> int:
    """Compute the cost of a circuit by transpiling the circuit to a given ``basis_gates`` gate set and a certain ``optimization_level``."""
    from qiskit import transpile

    transpiled_circuit = transpile(
        qc, basis_gates=basis_gates, optimization_level=optimization_level, seed_transpiler=12345
    )
    size: int = transpiled_circuit.size()
    return size


class GateType(Enum):
    """Enum for gate types."""

    GENERAL = 1
    MULTI_CONTROLLED = 2


def __create_gate_profile_data(
    gate_collection: list[dict[str, Any]],
    gate_type: GateType,
    basis_gates: list[str] | None = None,
    optimization_level: int = 1,
) -> dict[tuple[str, int], int]:
    """Create a dictionary of gate profile data."""
    if basis_gates is None:
        basis_gates = ["id", "rz", "sx", "x", "cx"]

    profile_data = {}
    for gate_set in gate_collection:
        gates = gate_set["gates"]
        qubits = gate_set["qubits"]
        params = gate_set["params"]
        controls = gate_set["controls"]

        # pack single control numbers into list
        if gate_type == GateType.GENERAL:
            controls = [controls]

        for gate in gates:
            for control in controls:
                qc = None
                # create the gate
                if gate_type == GateType.GENERAL:
                    qc = __create_general_gate(qubits, params, control, gate)
                elif gate_type == GateType.MULTI_CONTROLLED:
                    qc = __create_multi_controlled_gate(
                        qubits,
                        params,
                        control,
                        gate_set["mode"],
                        gate_set["ancilla_qubits"],
                        gate,
                    )
                # compute the cost
                cost = __compute_cost(qc, basis_gates, optimization_level)

                # add the cost to the profile data
                profile_data[gate, control] = cost
    return profile_data


def __add_special_case_data(
    profile_data: dict[tuple[str, int], int],
    special_cases: dict[str, Any] | None = None,
) -> None:
    """Add special case data to the profile data. This is used to extrapolate the cost of specific rotation gates (e.g., S, T, ...) from the cost of the generic phase gate."""
    if special_cases is None:
        special_cases = {
            "gates": ["z", "s", "sdg", "t", "tdg"],
            "underlying_gate": "p",
        }

    gates = special_cases["gates"]
    underlying_gate = special_cases["underlying_gate"]

    for (g, nc), cost in profile_data.copy().items():
        if g is underlying_gate:
            for gate in gates:
                profile_data.setdefault((gate, nc), cost)


def __write_profile_data_to_file(profile_data: dict[tuple[str, int], int], filename: Path) -> None:
    """Write the profile data to a file."""
    HAS_QISKIT.require_now("generate compilation flow profiles")
    from qiskit import __version__ as qiskit_version

    with Path(filename).open("w+", encoding="utf-8") as f:
        f.write(f"# {filename}, Qiskit version: {qiskit_version}\n")
        f.writelines(f"{gate} {controls} {cost}\n" for (gate, controls), cost in profile_data.items())


def __check_recurrence(seq: list[int], order: int = 2) -> list[int] | None:
    """Determine a recurrence relation with a given ``order`` in ``sequence`` and return the corresponding coefficients or ``None`` if no relation was determined."""
    if len(seq) < (2 * order + 1):
        return None

    mat = np.array([seq[i : i + order] for i in range(order)], dtype=int)
    f = np.array([seq[i + order] for i in range(order)], dtype=int)

    if np.linalg.det(mat) == 0:
        return None

    coefficients: NDArray[np.float64] = np.linalg.inv(mat).dot(f)

    for i in range(2 * order, len(seq)):
        predict: float = np.sum(coefficients * np.array(seq[i - order : i], dtype=float))
        if abs(predict - seq[i]) > 10 ** (-2):
            return None

    return list(coefficients)


def __find_continuation(
    profile_data: dict[tuple[str, int], int],
    gate: str,
    cutoff: int = 5,
    max_order: int = 3,
    max_control: int = 11,
    max_qubits: int = 128,
    prediction_cutoff: float = 1e6,
) -> None:
    """Extrapolate from the given profile data by finding recurrence relations."""
    sequence = [cost for (g, _), cost in profile_data.items() if g == gate]

    # sort the sequence
    sequence = sorted(sequence)

    # cut off the sequence at the cutoff point
    sequence = sequence[cutoff:]

    coeffs = None
    for order in range(1, max_order + 1):
        coeffs = __check_recurrence(sequence, order)
        if coeffs is not None:
            break

    # if no recurrence sequence was found, assume the sequence is linear
    if coeffs is None:
        coeffs = [-1, 2]

    entries_to_compute = max_qubits - max_control - 1
    order = len(coeffs)
    for i in range(entries_to_compute):
        next_term = int(np.round(np.sum(coeffs * np.array(sequence[-order:]))))
        if next_term >= prediction_cutoff:
            break
        sequence.append(next_term)
        profile_data[gate, max_control + i + 1] = next_term


gate_collection_for_mode = {
    AncillaMode.NO_ANCILLA: multi_controlled_gates_no_ancilla,
    AncillaMode.RECURSION: multi_controlled_gates_recursion,
    AncillaMode.V_CHAIN: multi_controlled_gates_v_chain,
}
default_profile_path = Path(__file__).resolve().parent.joinpath("profiles")


def generate_profile_name(optimization_level: int = 1, mode: AncillaMode = AncillaMode.NO_ANCILLA) -> str:
    """Generate a profile name based on the given optimization level and ancilla mode."""
    return "qiskit_O" + str(optimization_level) + "_" + str(mode) + ".profile"


def generate_profile(
    optimization_level: int = 1,
    mode: AncillaMode = AncillaMode.NO_ANCILLA,
    filepath: Path | None = None,
) -> None:
    """Generate a compilation flow profile for the given optimization level and ancilla mode.

    Args:
        optimization_level:
            The IBM Qiskit optimization level to use for the profile (0, 1, 2, or 3). Defaults to 1.
        mode:
            The :class:`ancilla mode <.AncillaMode>` used for realizing multi-controlled Toffoli gates, as available in Qiskit.
            Defaults to :attr:`.AncillaMode.NO_ANCILLA`.
        filepath:
            The path to the directory where the profile should be stored.
            Defaults to the ``profiles`` directory in the ``mqt.qcec`` package.

    .. warning::
        Qiskit has deprecated the ``mode`` argument of ``QuantumCircuit.mcx()`` with version 2.1.
        In accordance with this, ``mqt.qcec`` has deprecated the ``mode`` argument as well.
        The argument will be removed in a future release.
    """
    if mode != AncillaMode.NO_ANCILLA:
        warnings.warn(
            "Qiskit has deprecated the ``mode`` argument of ``QuantumCircuit.mcx()`` with version 2.1. "
            "In accordance with this, ``mqt.qcec`` has deprecated the ``mode`` argument of ``generate_profile`` as well. "
            "The argument will be removed in a future release.",
            DeprecationWarning,
            stacklevel=2,
        )

    if filepath is None:
        filepath = default_profile_path

    # generate general profile data
    profile = __create_gate_profile_data(general_gates, GateType.GENERAL, optimization_level=optimization_level)

    # add multi-controlled gates
    profile.update(
        __create_gate_profile_data(
            multi_controlled_gates,
            GateType.MULTI_CONTROLLED,
            optimization_level=optimization_level,
        )
    )
    __find_continuation(profile, gate="p", max_control=max_controls)

    gate_collection = gate_collection_for_mode[mode]

    # add multi-controlled gates with specific mode
    profile.update(
        __create_gate_profile_data(
            gate_collection,
            GateType.MULTI_CONTROLLED,
            optimization_level=optimization_level,
        )
    )
    __find_continuation(profile, gate="x", max_control=max_controls)

    # add special case data
    __add_special_case_data(profile)

    # write profile data to file
    filename = generate_profile_name(optimization_level=optimization_level, mode=mode)
    filepath = filepath.joinpath(filename)
    __write_profile_data_to_file(profile, filepath)
