# Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Tests the partial equivalence checking support of QCEC."""

from __future__ import annotations

import pytest
from qiskit import transpile
from qiskit.circuit import QuantumCircuit
from qiskit.providers.fake_provider import GenericBackendV2

from mqt.qcec import verify
from mqt.qcec.pyqcec import Configuration, EquivalenceCriterion


@pytest.fixture
def original_circuit() -> QuantumCircuit:
    """Fixture for a simple circuit."""
    qc = QuantumCircuit(3, 1)
    qc.cswap(1, 0, 2)
    qc.h(0)
    qc.z(2)
    qc.cswap(1, 0, 2)
    qc.measure(0, 0)
    return qc


@pytest.fixture
def alternative_circuit() -> QuantumCircuit:
    """Fixture for a partially equivalent version of the simple circuit."""
    qc = QuantumCircuit(3, 1)
    qc.x(1)
    qc.ch(1, 0)
    qc.measure(0, 0)
    return qc


def test_configuration_pec(original_circuit: QuantumCircuit, alternative_circuit: QuantumCircuit) -> None:
    """Test if the flag for partial equivalence checking works."""
    config = Configuration()
    config.functionality.check_partial_equivalence = True
    result = verify(original_circuit, alternative_circuit, configuration=config)
    assert result.equivalence == EquivalenceCriterion.equivalent


def test_argument_pec(original_circuit: QuantumCircuit, alternative_circuit: QuantumCircuit) -> None:
    """Test if the flag for partial equivalence checking works."""
    result = verify(original_circuit, alternative_circuit, check_partial_equivalence=True)
    assert result.equivalence == EquivalenceCriterion.equivalent


@pytest.mark.parametrize("measure_all", [False, True])
def test_compiled_steane_syndrome(measure_all: bool) -> None:
    """Check the measured outputs of the circuit from issue #440."""
    circuit = QuantumCircuit(10, 10 if measure_all else 3)
    circuit.x(5)
    for target, controls in [(7, [0, 2, 4, 6]), (8, [1, 2, 5, 6]), (9, [3, 4, 5, 6])]:
        circuit.cx(controls, target)
    circuit.measure(range(10) if measure_all else [7, 8, 9], range(10) if measure_all else range(3))
    backend = GenericBackendV2(
        num_qubits=13,
        coupling_map=[
            [0, 1],
            [1, 2],
            [2, 3],
            [2, 5],
            [4, 5],
            [5, 6],
            [5, 8],
            [7, 8],
            [8, 9],
            [9, 10],
            [3, 11],
            [7, 12],
        ],
        seed=1,
    )
    compiled = transpile(circuit, backend, seed_transpiler=0)
    result = verify(
        circuit, compiled, check_partial_equivalence=not measure_all, run_simulation_checker=False, run_zx_checker=False
    )
    assert result.considered_equivalent()
