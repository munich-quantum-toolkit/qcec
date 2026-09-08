# Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Tests the hybrid Schrödinger-Feynman checker."""

from __future__ import annotations

from math import pi

from qiskit import QuantumCircuit

from mqt import qcec
from mqt.qcec.pyqcec import Configuration, EquivalenceCriterion


def hsf_configuration(threshold: float = 0.0) -> Configuration:
    """Create a configuration that runs only the HSF checker."""
    config = Configuration()
    config.execution.parallel = False
    config.execution.nthreads = 1
    config.execution.run_alternating_checker = False
    config.execution.run_construction_checker = False
    config.execution.run_simulation_checker = False
    config.execution.run_zx_checker = False
    config.execution.run_hsf_checker = True
    config.functionality.check_approximate_equivalence = True
    config.functionality.approximate_checking_threshold = threshold
    return config


def test_hsf_configuration() -> None:
    """Run the HSF checker through a Configuration object."""
    circuit = QuantumCircuit(2)
    circuit.h(0)
    circuit.cx(0, 1)
    config = hsf_configuration()

    result = qcec.verify(circuit, circuit, configuration=config)

    assert config.execution.run_hsf_checker
    assert config.execution.nthreads == 1
    assert result.equivalence == EquivalenceCriterion.equivalent


def test_hsf_preserves_circuit_global_phase() -> None:
    """Preserve an explicit circuit global phase in the trace."""
    original = QuantumCircuit(2)
    original.h(0)
    original.x(1)
    alternative = original.copy()
    alternative.global_phase = pi / 3.0

    result = qcec.verify(original, alternative, configuration=hsf_configuration())

    assert result.equivalence == EquivalenceCriterion.equivalent_up_to_global_phase


def test_hsf_keyword_arguments() -> None:
    """Configure the HSF checker through verify keyword arguments."""
    original = QuantumCircuit(2)
    alternative = QuantumCircuit(2)
    original.x(0)
    original.h(1)
    alternative.x(0)
    alternative.ry(pi / 3.0, 0)
    alternative.h(1)
    config = hsf_configuration()
    config.execution.run_hsf_checker = False
    config.functionality.check_approximate_equivalence = False

    result = qcec.verify(
        original,
        alternative,
        configuration=config,
        run_hsf_checker=True,
        check_approximate_equivalence=True,
        approximate_checking_threshold=0.51,
    )

    assert result.equivalence == EquivalenceCriterion.equivalent
