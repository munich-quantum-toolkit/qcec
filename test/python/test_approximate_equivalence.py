# Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Tests the approximate equivalence checking support of QCEC."""

from __future__ import annotations

import pytest
from qiskit import QuantumCircuit

from mqt import qcec
from mqt.qcec.pyqcec import Configuration, EquivalenceCriterion


@pytest.fixture
def original_circuit() -> QuantumCircuit:
    """Fixture for a simple circuit."""
    qc = QuantumCircuit(3)
    qc.mcx([0, 1], 2)
    return qc


@pytest.fixture
def alternative_circuit() -> QuantumCircuit:
    """Fixture for an approximately equivalent version of the simple circuit."""
    qc = QuantumCircuit(3)
    qc.id(0)
    qc.id(1)
    qc.id(2)
    return qc


def test_configuration_pec(original_circuit: QuantumCircuit, alternative_circuit: QuantumCircuit) -> None:
    """Test if the flag for approximate equivalence checking works."""
    config = Configuration()
    config.execution.run_alternating_checker = True
    config.execution.run_construction_checker = False
    config.execution.run_simulation_checker = False
    config.execution.run_zx_checker = False
    config.functionality.check_approximate_equivalence = True
    config.functionality.approximate_checking_threshold = 0.3
    result = qcec.verify(original_circuit, alternative_circuit, configuration=config)
    assert result.equivalence == EquivalenceCriterion.equivalent


def test_argument_pec(original_circuit: QuantumCircuit, alternative_circuit: QuantumCircuit) -> None:
    """Test if the flag for approximate equivalence checking works."""
    config = Configuration()
    config.execution.run_alternating_checker = True
    config.execution.run_construction_checker = False
    config.execution.run_simulation_checker = False
    config.execution.run_zx_checker = False
    result = qcec.verify(
        original_circuit,
        alternative_circuit,
        configuration=config,
        check_approximate_equivalence=True,
        approximate_checking_threshold=0.3,
    )
    assert result.equivalence == EquivalenceCriterion.equivalent
