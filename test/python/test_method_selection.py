# Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Test explicit checker selection and automatic fallback."""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest
from mqt.core import load
from mqt.core.ir import QuantumComputation
from qiskit.circuit import AncillaRegister, Parameter, QuantumCircuit

from mqt.qcec import verify, verify_compilation, verify_with_hard_timeout
from mqt.qcec.pyqcec import ApplicationScheme, Configuration, EquivalenceCheckingManager, EquivalenceCriterion

if TYPE_CHECKING:
    from mqt.qcec.configuration_options import ConfigurationOptions


@pytest.mark.parametrize("method", ["alternating", "construction", "simulation", "zx", "hsf"])
@pytest.mark.parametrize("parallel", [False, True])
def test_explicit_method(method: str, parallel: bool) -> None:
    """An explicit method overrides conflicting legacy checker flags."""
    circuit = QuantumCircuit(2)
    circuit.h(0)
    circuit.cx(0, 1)
    config = Configuration()
    config.execution.method = method
    result = verify(
        circuit,
        circuit,
        configuration=config,
        parallel=parallel,
        max_sims=2,
        run_construction_checker=True,
        run_simulation_checker=True,
        run_alternating_checker=True,
        run_zx_checker=True,
        run_hsf_checker=True,
        check_approximate_equivalence=method == "hsf",
    )
    assert result.considered_equivalent()
    checkers = result.json()["checkers"]
    assert checkers
    assert all(
        ("hybrid_schrodinger_feynman" if method == "hsf" else method) in checker["checker"].lower()
        for checker in checkers
    )


@pytest.mark.parametrize("method", ["alternating", "construction", "simulation", "hsf"])
def test_symbolic_dd_method_rejected(method: str) -> None:
    """Explicit DD methods do not instantiate parameters or invoke ZX."""
    circuit = QuantumCircuit(1)
    circuit.rx(Parameter("theta"), 0)
    config = Configuration()
    config.execution.method = method
    with pytest.raises(ValueError, match="symbolic"):
        verify(circuit, circuit, configuration=config, check_approximate_equivalence=method == "hsf")


def test_symbolic_zx_does_not_instantiate() -> None:
    """An inconclusive explicit symbolic ZX check stays inconclusive."""
    left = QuantumCircuit(1)
    left.rx(Parameter("theta"), 0)
    right = left.copy()
    right.h(0)
    result = verify(left, right, method="zx")
    assert result.performed_instantiations == 0
    assert all("zx" in checker["checker"].lower() for checker in result.json()["checkers"])


@pytest.mark.parametrize("method", ["zx", "simulation"])
def test_unsupported_approximate_method(method: str) -> None:
    """Explicit methods must implement the requested equivalence criterion."""
    circuit = QuantumCircuit(1)
    circuit.h(0)
    config = Configuration()
    config.execution.method = method
    with pytest.raises(ValueError, match=r"[Aa]pproximate"):
        verify(circuit, circuit, configuration=config, check_approximate_equivalence=True)


def test_unsupported_zx_gate() -> None:
    """Unsupported ZX input raises instead of silently skipping the checker."""
    circuit = QuantumComputation(4)
    circuit.mcy({0, 1, 2}, 3)
    with pytest.raises(ValueError, match="ZX"):
        verify(circuit, circuit, method="zx")


def test_explicit_alternating_ancilla_rejected() -> None:
    """Only auto may substitute construction for alternating."""
    circuit = QuantumCircuit(1)
    circuit.add_register(AncillaRegister(1))
    circuit.h(1)
    with pytest.raises(ValueError, match="alternating"):
        verify(circuit, circuit, method="alternating")
    assert verify(
        circuit, circuit, method="auto", run_simulation_checker=False, run_zx_checker=False
    ).considered_equivalent()


@pytest.mark.parametrize(
    ("method", "options", "message"),
    [
        ("unknown", {}, "method"),
        ("simulation", {"max_sims": 0}, "max_sims"),
        ("hsf", {}, "approximate"),
        ("zx", {"check_partial_equivalence": True}, "partial"),
    ],
)
def test_unsupported_configuration(method: str, options: ConfigurationOptions, message: str) -> None:
    """Validate explicit selections even for empty circuits."""
    circuit = QuantumCircuit(1)
    config = Configuration()
    config.execution.method = method
    with pytest.raises(ValueError, match=message):
        verify(circuit, circuit, configuration=config, **options)


def test_mutable_method() -> None:
    """Changing method before run applies selection and simulation limits."""
    circuit = QuantumCircuit(1)
    circuit.h(0)
    config = Configuration()
    assert config.execution.method == "auto"
    config.execution.method = "construction"
    config.simulation.max_sims = 100
    manager = EquivalenceCheckingManager(load(circuit), load(circuit), config)
    manager.configuration.execution.method = "simulation"
    manager.run()
    assert manager.results.considered_equivalent()
    assert manager.results.performed_simulations == 2
    assert manager.configuration.json()["execution"]["method"] == "simulation"

    manager = EquivalenceCheckingManager(load(circuit), load(circuit), config)
    manager.disable_all_checkers()
    manager.run()
    assert manager.results.equivalence == EquivalenceCriterion.no_information


def test_hard_timeout_method() -> None:
    """The child process honors explicit method selection."""
    circuit = QuantumCircuit(1)
    circuit.h(0)
    result = verify_with_hard_timeout(circuit, circuit, deadline=30, method="construction")
    assert result["equivalence"] == "equivalent"
    assert len(result["checkers"]) == 1
    assert "construction" in result["checkers"][0]["checker"].lower()


def test_unsupported_zx_ancilla() -> None:
    """Explicit ZX rejects non-garbage ancillary outputs."""
    circuit = QuantumCircuit(1)
    circuit.add_register(AncillaRegister(1))
    circuit.h(1)
    with pytest.raises(ValueError, match="ZX"):
        verify(circuit, circuit, method="zx")


@pytest.mark.parametrize("method", ["alternating", "construction", "simulation", "zx"])
def test_empty_circuit_uses_explicit_checker(method: str) -> None:
    """Even an empty circuit uses the selected checker and its validation."""
    circuit = QuantumCircuit(1)
    config = Configuration()
    config.execution.method = method
    result = verify(circuit, circuit, configuration=config)
    assert result.considered_equivalent()
    assert len(result.json()["checkers"]) == 1


@pytest.mark.parametrize("method", ["construction", "simulation"])
def test_empty_circuit_rejects_unsupported_scheme(method: str) -> None:
    """Preprocessing must not bypass validation of the selected checker."""
    circuit = QuantumCircuit(1)
    config = Configuration()
    config.execution.method = method
    config.application.construction_scheme = ApplicationScheme.lookahead
    config.application.simulation_scheme = ApplicationScheme.lookahead
    with pytest.raises(ValueError, match="Lookahead"):
        verify(circuit, circuit, configuration=config)


def test_compilation_method() -> None:
    """Compilation verification forwards the checker selection."""
    circuit = QuantumCircuit(1)
    circuit.h(0)
    circuit.measure_all()
    result = verify_compilation(circuit, circuit, method="construction")
    assert result.considered_equivalent()
    assert result.json()["checkers"][0]["checker"] == "decision_diagram_construction"


@pytest.mark.parametrize("method", ["alternating", "construction", "zx"])
def test_empty_global_phase(method: str) -> None:
    """Explicit checks preserve the phase-only circuit result."""
    circuit = QuantumCircuit(1)
    shifted = circuit.copy()
    shifted.global_phase = 0.5
    config = Configuration()
    config.execution.method = method
    result = verify(circuit, shifted, configuration=config)
    assert result.equivalence == EquivalenceCriterion.equivalent_up_to_global_phase


def test_empty_hsf_rejected() -> None:
    """HSF still requires two qubits after preprocessing."""
    circuit = QuantumCircuit(2)
    with pytest.raises(ValueError, match="at least two qubits"):
        verify(circuit, circuit, method="hsf", check_approximate_equivalence=True)
