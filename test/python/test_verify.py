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
from qiskit import transpile
from qiskit.circuit import AncillaRegister, QuantumCircuit

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
    """Regression test for https://github.com/cda-tum/qcec/issues/236.

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
