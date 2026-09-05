# Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Tests for approximate equivalence checking."""

from __future__ import annotations

import math

import pytest
from qiskit import AncillaRegister, QuantumCircuit
from qiskit.circuit import Parameter

from mqt import qcec
from mqt.qcec.pyqcec import Configuration, EquivalenceCriterion


@pytest.fixture
def phase_pair() -> tuple[QuantumCircuit, QuantumCircuit]:
    """Return circuits with projective Hilbert--Schmidt distance one half."""
    identity = QuantumCircuit(1)
    phase = QuantumCircuit(1)
    phase.p(math.pi / 3, 0)
    return identity, phase


def test_configuration_options_accept_approximate_pair(
    phase_pair: tuple[QuantumCircuit, QuantumCircuit],
) -> None:
    """Configure approximate equivalence through a Configuration object."""
    identity, phase = phase_pair
    config = Configuration()
    config.execution.parallel = False
    config.functionality.check_approximate_equivalence = True
    config.functionality.approximate_checking_threshold = 0.51

    result = qcec.verify(identity, phase, configuration=config)

    assert result.equivalence == EquivalenceCriterion.equivalent
    assert result.started_simulations == 0


def test_keyword_options_reject_pair_beyond_threshold(
    phase_pair: tuple[QuantumCircuit, QuantumCircuit],
) -> None:
    """Configure approximate equivalence through verify keyword arguments."""
    identity, phase = phase_pair

    result = qcec.verify(
        identity,
        phase,
        parallel=False,
        check_approximate_equivalence=True,
        approximate_checking_threshold=0.49,
    )

    assert result.equivalence == EquivalenceCriterion.not_equivalent
    assert result.started_simulations == 0


@pytest.mark.parametrize("parallel", [False, True])
def test_default_simulation_is_not_used_for_process_distance(parallel: bool) -> None:
    """Do not let state simulation overrule the configured process metric."""
    identity = QuantumCircuit(1)
    x_gate = QuantumCircuit(1)
    x_gate.x(0)

    result = qcec.verify(
        identity,
        x_gate,
        parallel=parallel,
        check_approximate_equivalence=True,
        approximate_checking_threshold=1.0,
    )

    assert result.equivalence == EquivalenceCriterion.equivalent
    assert result.started_simulations == 0


@pytest.mark.parametrize("threshold", [-0.1, 1.1, math.nan, math.inf, -math.inf])
def test_invalid_threshold_is_rejected(threshold: float) -> None:
    """Require a finite process-distance threshold in the unit interval."""
    circuit = QuantumCircuit(1)

    with pytest.raises(ValueError, match="threshold"):
        qcec.verify(
            circuit,
            circuit,
            check_approximate_equivalence=True,
            approximate_checking_threshold=threshold,
        )


def test_partial_approximate_equivalence_is_rejected() -> None:
    """Reject the undefined combination of partial and approximate checking."""
    circuit = QuantumCircuit(1)

    with pytest.raises(ValueError, match="partial"):
        qcec.verify(
            circuit,
            circuit,
            check_partial_equivalence=True,
            check_approximate_equivalence=True,
        )


def test_approximate_equivalence_with_ancilla_is_rejected() -> None:
    """Reject process-distance checking for non-unitary ancilla semantics."""
    ancilla = AncillaRegister(1)
    circuit = QuantumCircuit(1)
    circuit.add_register(ancilla)
    circuit.h(ancilla[0])

    with pytest.raises(ValueError, match="ancilla"):
        qcec.verify(circuit, circuit, check_approximate_equivalence=True)


def test_approximate_equivalence_for_symbolic_circuits_is_rejected() -> None:
    """Reject approximate checking without a universal symbolic definition."""
    theta = Parameter("theta")
    symbolic = QuantumCircuit(1)
    symbolic.rz(theta, 0)

    with pytest.raises(ValueError, match="symbolic"):
        qcec.verify(symbolic, symbolic, check_approximate_equivalence=True)


def test_approximate_equivalence_requires_a_supported_dd_checker() -> None:
    """Require a checker that implements the configured process metric."""
    circuit = QuantumCircuit(1)

    with pytest.raises(ValueError, match=r"alternating.*construction"):
        qcec.verify(
            circuit,
            circuit,
            check_approximate_equivalence=True,
            run_alternating_checker=False,
            run_construction_checker=False,
        )


def test_approximate_options_are_serialized() -> None:
    """Expose approximate-equivalence options in the configuration JSON."""
    config = Configuration()
    config.functionality.check_approximate_equivalence = True
    config.functionality.approximate_checking_threshold = 0.25

    functionality = config.json()["functionality"]

    assert functionality["check_approximate_equivalence"] is True
    assert functionality["approximate_checking_threshold"] == pytest.approx(0.25)
