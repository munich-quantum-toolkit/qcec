# Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Test the verification of two circuits."""

from __future__ import annotations

import pytest
from mqt.core.ir import QuantumComputation
from qiskit import qasm2, transpile
from qiskit.circuit import AncillaRegister, Parameter, QuantumCircuit

from mqt.qcec import verify
from mqt.qcec.pyqcec import ApplicationScheme, Configuration, EquivalenceCriterion


@pytest.fixture
def original_circuit() -> QuantumCircuit:
    """Fixture for a simple circuit."""
    qc = QuantumCircuit(3)
    qc.h(0)
    qc.cx(0, 1)
    qc.cx(0, 2)
    qc.measure_all()
    return qc


@pytest.fixture
def alternative_circuit() -> QuantumCircuit:
    """Fixture for an alternative version of the simple circuit."""
    qc = QuantumCircuit(3, 3)
    qc.h(0)
    qc.cx(0, 1)
    qc.swap(0, 1)
    qc.cx(1, 2)
    qc.measure(0, 1)
    qc.measure(1, 0)
    qc.measure(2, 2)
    return qc


def test_verify(original_circuit: QuantumCircuit, alternative_circuit: QuantumCircuit) -> None:
    """Test the verification of two equivalent circuits."""
    result = verify(original_circuit, alternative_circuit)
    assert result.equivalence == EquivalenceCriterion.equivalent


@pytest.mark.parametrize("checker", ["construction", "alternating", "simulation"])
@pytest.mark.parametrize("as_qasm", [False, True])
def test_offset_measurement_destinations(checker: str, as_qasm: bool) -> None:
    """Unused classical bits must not become logical output qubits."""
    circuit = QuantumCircuit(2, 2)
    circuit.h(0)
    circuit.cx(0, 1)
    circuit.measure_all()
    config = Configuration()
    config.execution.run_construction_checker = checker == "construction"
    config.execution.run_alternating_checker = checker == "alternating"
    config.execution.run_simulation_checker = checker == "simulation"
    config.execution.run_zx_checker = False
    config.simulation.seed = 42
    source = qasm2.dumps(circuit) if as_qasm else circuit
    result = verify(source, source, config)
    assert result.considered_equivalent()


def test_offset_measurements_after_compilation() -> None:
    """Normalize shared outputs across different physical circuit sizes."""
    circuit = QuantumCircuit(2, 4)
    circuit.x(0)
    circuit.cx(0, 1)
    circuit.measure([0, 1], [2, 3])
    compiled = transpile(
        circuit,
        coupling_map=[[0, 1], [1, 0], [1, 2], [2, 1]],
        initial_layout=[2, 0],
        basis_gates=["cx", "x", "h"],
        seed_transpiler=42,
    )
    result = verify(
        circuit,
        compiled,
        run_construction_checker=True,
        run_alternating_checker=False,
        run_simulation_checker=False,
        run_zx_checker=False,
    )
    assert result.considered_equivalent()


def test_offset_measurements_with_symbolic_gate() -> None:
    """Normalize outputs on the symbolic ZX path before parameter binding."""
    circuit = QuantumCircuit(2, 2)
    circuit.rx(Parameter("theta"), 0)
    circuit.cx(0, 1)
    circuit.measure_all()
    assert verify(circuit, circuit.copy()).considered_equivalent()


def test_offset_measurements_reject_different_labels() -> None:
    """Independent output renumbering must not erase classical label differences."""
    first = QuantumCircuit(2, 4)
    first.x(0)
    second = first.copy()
    first.measure([0, 1], [2, 3])
    second.measure([0, 1], [0, 1])
    with pytest.raises(ValueError, match="different classical measurement destinations"):
        verify(first, second)


def test_verify_kwargs(original_circuit: QuantumCircuit, alternative_circuit: QuantumCircuit) -> None:
    """Test the verification of two equivalent circuits with some keyword arguments (one of each category)."""
    result = verify(
        original_circuit,
        alternative_circuit,
        alternating_scheme=ApplicationScheme.one_to_one,
        timeout=3600,
        trace_threshold=1e-6,
        transform_dynamic_circuit=True,
        additional_instantiations=2,
        seed=42,
    )
    assert result.equivalence == EquivalenceCriterion.equivalent


def test_verify_config(original_circuit: QuantumCircuit, alternative_circuit: QuantumCircuit) -> None:
    """Test the verification of two equivalent circuits with a configuration object."""
    config = Configuration()
    config.execution.timeout = 3600
    result = verify(original_circuit, alternative_circuit, config)
    assert result.equivalence == EquivalenceCriterion.equivalent


def test_compiled_circuit_without_measurements() -> None:
    """Regression test for https://github.com/munich-quantum-toolkit/qcec/issues/236.

    It makes sure that circuits compiled without measurements are handled correctly.
    """
    qc = QuantumCircuit(1)
    qc.x(0)
    qc_compiled = transpile(
        qc,
        coupling_map=[[0, 1], [1, 0], [1, 2], [2, 1], [2, 3], [3, 2], [3, 4], [4, 3]],
        basis_gates=["cx", "x", "id", "u3", "measure", "u2", "rz", "u1", "reset", "sx"],
    )

    result = verify(qc, qc_compiled)
    assert result.equivalence == EquivalenceCriterion.equivalent


def test_cpp_exception_propagation_internal() -> None:
    """Test that C++ exceptions caused by code within QCEC are propagated correctly."""
    qc = QuantumCircuit(1)
    qc.x(0)

    config = Configuration()
    config.execution.run_alternating_checker = False
    config.execution.run_simulation_checker = True
    config.execution.run_construction_checker = False
    config.execution.run_zx_checker = False
    config.application.simulation_scheme = ApplicationScheme.lookahead

    with pytest.raises(ValueError, match=r"Lookahead application scheme can only be used for matrices."):
        verify(qc, qc, configuration=config)


def test_zx_ancilla_support() -> None:
    """This is a regression test for the handling of ancilla registers in the ZX checker."""
    anc = AncillaRegister(1)

    qc1 = QuantumCircuit(1, 0)

    qc1.add_register(anc)
    qc1.h(anc[0])

    qc2 = QuantumCircuit(1, 0)
    qc2.add_register(anc)

    result = verify(
        qc1,
        qc2,
        check_partial_equivalence=True,
        parallel=False,
        run_alternating_checker=False,
        run_simulation_checker=False,
        run_zx_checker=True,
        run_construction_checker=False,
    )
    assert result.equivalence == EquivalenceCriterion.no_information


def test_issue_928() -> None:
    """This is a regression test for the issue described in https://github.com/munich-quantum-toolkit/qcec/issues/928."""
    qc1 = QuantumComputation.from_qasm_str("""
OPENQASM 2.0;
include "qelib1.inc";

qreg q[8];
creg m_c_7[1];

id q[7];
id q[0];
id q[1];
id q[2];
id q[3];
id q[4];
id q[5];
id q[6];
measure q[7] -> m_c_7[0];
id q[0];
id q[1];
id q[2];
id q[3];
id q[4];
id q[5];
id q[6];
id q[7];
""")

    qc2 = QuantumComputation.from_qasm_str("""
OPENQASM 2.0;
include "qelib1.inc";

qreg q[9];
creg c[9];

id q[0];
id q[1];
id q[2];
id q[3];
id q[4];
id q[5];
id q[6];
id q[7];
id q[8];
cx q[7],q[8];
""")

    result = verify(qc1, qc2, transform_dynamic_circuit=True)
    assert result.equivalence == EquivalenceCriterion.not_equivalent


def _swapped_controlled_rz_pair(gate: str) -> tuple[QuantumComputation, QuantumComputation]:
    """Build a controlled RZ-type gate and the same gate with a control and a target qubit swapped."""
    if gate == "crz":
        qc1, qc2 = QuantumComputation(2), QuantumComputation(2)
        qc1.crz(1.0, 0, 1)
        qc2.crz(1.0, 1, 0)
    elif gate == "mcrz":
        qc1, qc2 = QuantumComputation(3), QuantumComputation(3)
        qc1.mcrz(1.0, {0, 1}, 2)
        qc2.mcrz(1.0, {0, 2}, 1)
    else:
        qc1, qc2 = QuantumComputation(3), QuantumComputation(3)
        qc1.crzz(1.0, 0, 1, 2)
        qc2.crzz(1.0, 1, 0, 2)
    return qc1, qc2


@pytest.mark.parametrize("gate", ["crz", "mcrz", "crzz"])
def test_issue_1084(gate: str) -> None:
    """This is a regression test for the issue described in https://github.com/munich-quantum-toolkit/qcec/issues/1084.

    Controlled RZ gates are not symmetric in their control and target qubits.
    """
    qc1, qc2 = _swapped_controlled_rz_pair(gate)
    result = verify(
        qc1,
        qc2,
        run_alternating_checker=True,
        run_construction_checker=False,
        run_simulation_checker=False,
        run_zx_checker=False,
    )
    assert result.equivalence == EquivalenceCriterion.not_equivalent


def test_controlled_phase_control_target_swap() -> None:
    """Test that a controlled phase gate is symmetric in its control and target qubits."""
    qc1, qc2 = QuantumComputation(2), QuantumComputation(2)
    qc1.cp(1.0, 0, 1)
    qc2.cp(1.0, 1, 0)
    result = verify(
        qc1,
        qc2,
        run_alternating_checker=True,
        run_construction_checker=False,
        run_simulation_checker=False,
        run_zx_checker=False,
    )
    assert result.equivalence == EquivalenceCriterion.equivalent
