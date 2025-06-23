# Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Test symbolic equivalence checking."""

from __future__ import annotations

import pytest
from qiskit import QuantumCircuit, transpile
from qiskit.circuit import Parameter

from mqt.qcec import verify, verify_compilation
from mqt.qcec.compilation_flow_profiles import AncillaMode
from mqt.qcec.pyqcec import Configuration, EquivalenceCriterion

alpha = Parameter("alpha")
beta = Parameter("beta")


@pytest.fixture
def rz_commute_lhs() -> QuantumCircuit:
    """Fixture for a circuit."""
    qc = QuantumCircuit(2)
    qc.rz(alpha, 0)
    qc.cx(1, 0)
    qc.rz(beta, 0)
    qc.cx(1, 0)
    return qc


@pytest.fixture
def rz_commute_rhs_correct() -> QuantumCircuit:
    """Fixture for an equivalent circuit."""
    qc = QuantumCircuit(2)
    qc.cx(1, 0)
    qc.rz(beta, 0)
    qc.cx(1, 0)
    qc.rz(alpha, 0)
    return qc


@pytest.fixture
def rz_commute_rhs_incorrect() -> QuantumCircuit:
    """Fixture for a non-equivalent circuit."""
    qc = QuantumCircuit(2)
    qc.cx(1, 0)
    qc.rz(alpha, 0)
    qc.cx(1, 0)
    qc.rz(beta, 0)
    return qc


@pytest.fixture
def rotation_gate_fuse_lhs() -> QuantumCircuit:
    """Fixture for a symbolic circuit with simplification opportunities."""
    qc = QuantumCircuit(1)
    qc.rz(alpha, 0)
    qc.rz(beta, 0)
    return qc


@pytest.fixture
def rotation_gate_fuse_rhs_correct() -> QuantumCircuit:
    """Fixture for an equivalent, optimized circuit."""
    qc = QuantumCircuit(1)
    qc.rz(alpha + beta, 0)
    return qc


@pytest.fixture
def rotation_gate_fuse_rhs_incorrect() -> QuantumCircuit:
    """Fixture for a non-equivalent, optimized circuit."""
    qc = QuantumCircuit(1)
    qc.rz(alpha - beta, 0)
    return qc


@pytest.fixture
def rotation_gate_fuse_rhs_approximate() -> QuantumCircuit:
    """Fixture for an approximately-equivalent, optimized circuit."""
    qc = QuantumCircuit(1)
    qc.rz(alpha + beta + 1e-7, 0)
    return qc


@pytest.fixture
def cnot_rx() -> QuantumCircuit:
    """Fixture for a circuit with a CNOT and a rotation gate."""
    qc = QuantumCircuit(2)
    qc.cx(0, 1)
    qc.rx(alpha, 0)
    return qc


@pytest.fixture
def cnot_rx_flipped() -> QuantumCircuit:
    """Fixture for a circuit with a CNOT and a rotation gate, but commuted."""
    qc = QuantumCircuit(2)
    qc.cx(1, 0)
    qc.rx(alpha, 0)
    return qc


@pytest.fixture
def cnot_rx_flipped_approx() -> QuantumCircuit:
    """Fixture for an approximately-equivalent circuit with a CNOT and a rotation gate."""
    qc = QuantumCircuit(2)
    qc.cx(1, 0)
    qc.rx(alpha + 1e-12, 0)
    return qc


@pytest.fixture
def original_circuit() -> QuantumCircuit:
    """Fixture for a circuit with a couple of gates."""
    qc = QuantumCircuit(3)
    qc.h(0)
    qc.cx(0, 1)
    qc.cx(0, 2)
    qc.rx(alpha, 0)
    qc.measure_all()
    return qc


def test_equivalent_rz_commute(rz_commute_lhs: QuantumCircuit, rz_commute_rhs_correct: QuantumCircuit) -> None:
    """Test an RZ commutation rule."""
    result = verify(rz_commute_lhs, rz_commute_rhs_correct)
    assert result.equivalence == EquivalenceCriterion.equivalent


def test_non_equivalent_rz_commute(rz_commute_lhs: QuantumCircuit, rz_commute_rhs_incorrect: QuantumCircuit) -> None:
    """Test an invalid RZ commutation rule."""
    result = verify(rz_commute_lhs, rz_commute_rhs_incorrect, timeout=3600)
    assert result.equivalence == EquivalenceCriterion.not_equivalent


def test_non_equivalent_phase_rz_commute(
    rz_commute_lhs: QuantumCircuit, rz_commute_rhs_incorrect: QuantumCircuit
) -> None:
    """Test an invalid RZ commutation rule with additional instantiations.."""
    result = verify(rz_commute_lhs, rz_commute_rhs_incorrect, additional_instantiations=2, timeout=3600)
    assert result.equivalence == EquivalenceCriterion.not_equivalent


def test_equivalent_rotation_gate_fuse(
    rotation_gate_fuse_lhs: QuantumCircuit, rotation_gate_fuse_rhs_correct: QuantumCircuit
) -> None:
    """Test a rotation gate fusion rule."""
    result = verify(rotation_gate_fuse_lhs, rotation_gate_fuse_rhs_correct)
    assert result.equivalence == EquivalenceCriterion.equivalent


def test_non_equivalent_rotation_gate_fuse(
    rotation_gate_fuse_lhs: QuantumCircuit, rotation_gate_fuse_rhs_incorrect: QuantumCircuit
) -> None:
    """Test an invalid rotation gate fusion rule."""
    result = verify(rotation_gate_fuse_lhs, rotation_gate_fuse_rhs_incorrect)
    assert result.equivalence == EquivalenceCriterion.not_equivalent


def test_almost_zero_non_equ(
    rotation_gate_fuse_lhs: QuantumCircuit, rotation_gate_fuse_rhs_approximate: QuantumCircuit
) -> None:
    """Test an invalid rotation gate fusion rule with some small deviation."""
    result = verify(rotation_gate_fuse_lhs, rotation_gate_fuse_rhs_approximate)
    assert result.equivalence == EquivalenceCriterion.not_equivalent


def test_almost_zero_non_equ_random(
    rotation_gate_fuse_lhs: QuantumCircuit, rotation_gate_fuse_rhs_approximate: QuantumCircuit
) -> None:
    """Test an invalid rotation gate fusion rule with some small deviation."""
    result = verify(rotation_gate_fuse_lhs, rotation_gate_fuse_rhs_approximate, parameterized_tolerance=1e-9)
    assert result.equivalence == EquivalenceCriterion.not_equivalent


def test_cnot_rx_non_equ(cnot_rx: QuantumCircuit, cnot_rx_flipped: QuantumCircuit) -> None:
    """Test an invalid CNOT-RX rule."""
    result = verify(cnot_rx, cnot_rx_flipped)
    assert result.equivalence == EquivalenceCriterion.not_equivalent


def test_cnot_rx_non_equ_approx(cnot_rx: QuantumCircuit, cnot_rx_flipped_approx: QuantumCircuit) -> None:
    """Test an invalid CNOT-RX rule with some small deviation."""
    result = verify(cnot_rx, cnot_rx_flipped_approx)
    assert result.equivalence == EquivalenceCriterion.not_equivalent


@pytest.mark.parametrize("optimization_level", [0, 1, 2, 3])
def test_verify_compilation_on_optimization_levels(original_circuit: QuantumCircuit, optimization_level: int) -> None:
    """Test the verification of the compilation of a circuit to the 5-qubit IBMQ Athens architecture with various optimization levels."""
    compiled_circuit = transpile(
        original_circuit,
        coupling_map=[[0, 1], [1, 0], [1, 2], [2, 1], [2, 3], [3, 2], [3, 4], [4, 3]],
        basis_gates=["cx", "x", "id", "u3", "measure", "u2", "rz", "u1", "reset", "sx"],
        optimization_level=optimization_level,
    )
    result = verify_compilation(original_circuit, compiled_circuit, optimization_level, timeout=3600)
    assert result.equivalence in {
        EquivalenceCriterion.equivalent,
        EquivalenceCriterion.equivalent_up_to_global_phase,
    }


@pytest.mark.parametrize("optimization_level", [0, 1, 2, 3])
def test_verify_compilation_on_optimization_levels_config(
    original_circuit: QuantumCircuit, optimization_level: int
) -> None:
    """Test the verification of the compilation of a circuit to the 5-qubit IBMQ Athens architecture with various optimization levels."""
    config = Configuration()
    config.execution.run_zx_checker = False
    compiled_circuit = transpile(
        original_circuit,
        coupling_map=[[0, 1], [1, 0], [1, 2], [2, 1], [2, 3], [3, 2], [3, 4], [4, 3]],
        basis_gates=["cx", "x", "id", "u3", "measure", "u2", "rz", "u1", "reset", "sx"],
        optimization_level=optimization_level,
    )
    result = verify_compilation(original_circuit, compiled_circuit, optimization_level, AncillaMode.NO_ANCILLA, config)
    assert result.equivalence in {
        EquivalenceCriterion.equivalent,
        EquivalenceCriterion.equivalent_up_to_global_phase,
    }


def test_performed_instantiations(rz_commute_lhs: QuantumCircuit, rz_commute_rhs_incorrect: QuantumCircuit) -> None:
    """Test the number of performed instantiations."""
    result = verify(rz_commute_lhs, rz_commute_rhs_incorrect, additional_instantiations=10)
    assert result.equivalence == EquivalenceCriterion.not_equivalent
    min_instantiations = 1
    max_instantiations = 10
    assert min_instantiations < result.performed_instantiations < max_instantiations


def test_with_config(rz_commute_lhs: QuantumCircuit, rz_commute_rhs_incorrect: QuantumCircuit) -> None:
    """Test the number of performed instantiations via a configuration object."""
    config = Configuration()
    config.parameterized.additional_instantiations = 10
    result = verify(rz_commute_lhs, rz_commute_rhs_incorrect, config)
    assert result.equivalence == EquivalenceCriterion.not_equivalent
    min_instantiations = 1
    max_instantiations = 10
    assert min_instantiations < result.performed_instantiations < max_instantiations
