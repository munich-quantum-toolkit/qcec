# Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Tests the dynamic quantum circuit support of QCEC."""

from __future__ import annotations

from mqt.qcec import verify
from mqt.qcec.equivalence_criterion import EquivalenceCriterion
from qiskit import QuantumCircuit


def test_regression1() -> None:
    """Test a regression from https://github.com/munich-quantum-toolkit/qcec/issues/343."""
    qc = QuantumCircuit(5)
    qc.cx(1, 2)
    qc.cx(0, 3)
    qc.cx(1, 4)
    qc.cx(2, 4)
    qc.cx(3, 4)
    qc.measure_all()

    qc_dyn = QuantumCircuit(3, 5)
    qc_dyn.cx(0, 1)
    qc_dyn.cx(0, 2)
    qc_dyn.measure(0, 1)
    qc_dyn.reset(0)

    qc_dyn.cx(1, 2)
    qc_dyn.measure(1, 2)
    qc_dyn.reset(1)

    qc_dyn.cx(0, 1)
    qc_dyn.measure(0, 0)

    qc_dyn.cx(1, 2)
    qc_dyn.measure(1, 3)
    qc_dyn.measure(2, 4)

    result = verify(qc, qc_dyn, transform_dynamic_circuit=True, backpropagate_output_permutation=True)
    assert result.equivalence == EquivalenceCriterion.equivalent


def test_regression2() -> None:
    """Test a regression from https://github.com/munich-quantum-toolkit/qcec/issues/343."""
    qc = QuantumCircuit(9)
    qc.h(range(9))
    qc.z(8)
    qc.cx(0, 8)
    qc.cx(1, 8)
    qc.cx(3, 8)
    qc.cx(5, 8)
    qc.cx(6, 8)
    qc.cx(7, 8)
    qc.h(range(8))
    qc.measure_all()

    qc_dyn = QuantumCircuit(2, 9)
    qc_dyn.h(0)
    qc_dyn.h(0)
    qc_dyn.measure(0, 2)
    qc_dyn.reset(0)
    qc_dyn.h(0)
    qc_dyn.h(0)
    qc_dyn.measure(0, 4)
    qc_dyn.reset(0)
    qc_dyn.h(0)
    qc_dyn.h(1)
    qc_dyn.z(1)
    qc_dyn.cx(0, 1)
    qc_dyn.h(0)
    qc_dyn.measure(0, 0)
    qc_dyn.reset(0)
    qc_dyn.h(0)
    qc_dyn.cx(0, 1)
    qc_dyn.h(0)
    qc_dyn.measure(0, 1)
    qc_dyn.reset(0)
    qc_dyn.h(0)
    qc_dyn.cx(0, 1)
    qc_dyn.h(0)
    qc_dyn.measure(0, 3)
    qc_dyn.reset(0)
    qc_dyn.h(0)
    qc_dyn.cx(0, 1)
    qc_dyn.h(0)
    qc_dyn.measure(0, 5)
    qc_dyn.reset(0)
    qc_dyn.h(0)
    qc_dyn.cx(0, 1)
    qc_dyn.h(0)
    qc_dyn.measure(0, 6)
    qc_dyn.reset(0)
    qc_dyn.h(0)
    qc_dyn.cx(0, 1)
    qc_dyn.measure(1, 8)
    qc_dyn.h(0)
    qc_dyn.measure(0, 7)

    result = verify(qc, qc_dyn, transform_dynamic_circuit=True, backpropagate_output_permutation=True)
    assert result.equivalence == EquivalenceCriterion.equivalent
