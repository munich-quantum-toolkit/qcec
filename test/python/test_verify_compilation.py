# Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Test the compilation flow verification."""

from __future__ import annotations

import pytest
from qiskit import QuantumCircuit, transpile

from mqt.qcec import verify_compilation
from mqt.qcec.pyqcec import EquivalenceCriterion
from mqt.qcec.compilation_flow_profiles import AncillaMode


@pytest.fixture
def original_circuit() -> QuantumCircuit:
    """Fixture for a simple circuit."""
    qc = QuantumCircuit(3)
    qc.h(0)
    qc.cx(0, 1)
    qc.cx(0, 2)
    qc.measure_all()
    return qc


@pytest.mark.parametrize("optimization_level", [0, 1, 2, 3])
def test_verify_compilation_on_optimization_levels(original_circuit: QuantumCircuit, optimization_level: int) -> None:
    """Test the verification of the compilation of a circuit to the 5-qubit IBMQ Athens architecture with various optimization levels."""
    compiled_circuit = transpile(
        original_circuit,
        coupling_map=[[0, 1], [1, 0], [1, 2], [2, 1], [2, 3], [3, 2], [3, 4], [4, 3]],
        basis_gates=["cx", "x", "id", "u3", "measure", "u2", "rz", "u1", "reset", "sx"],
        optimization_level=optimization_level,
    )
    result = verify_compilation(original_circuit, compiled_circuit, optimization_level=optimization_level)
    assert result.equivalence in {
        EquivalenceCriterion.equivalent,
        EquivalenceCriterion.equivalent_up_to_global_phase,
    }


def test_warning_on_missing_measurements() -> None:
    """Tests that a warning is raised when either one of the circuits does not contain measurements."""
    qc = QuantumCircuit(2)
    qc.h(0)
    qc.cx(0, 1)

    with pytest.warns(UserWarning, match=r"One of the circuits does not contain any measurements."):
        result = verify_compilation(qc, qc)
    assert result.equivalence == EquivalenceCriterion.equivalent


def test_deprecation_warning(original_circuit: QuantumCircuit) -> None:
    """Tests that a deprecation warning is raised when the ``ancilla_mode`` argument is passed."""
    with pytest.warns(DeprecationWarning, match=r"``mqt.qcec`` has deprecated the ``ancilla_mode`` argument"):
        verify_compilation(original_circuit, original_circuit, ancilla_mode=AncillaMode.V_CHAIN)
