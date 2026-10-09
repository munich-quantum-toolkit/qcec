# Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Test the compilation flow verification."""

from __future__ import annotations

from contextlib import contextmanager
from importlib import resources
from typing import TYPE_CHECKING

import pytest
from qiskit import transpile
from qiskit.circuit import QuantumCircuit

from mqt.qcec import verify_compilation
from mqt.qcec.pyqcec import Configuration, EquivalenceCriterion

if TYPE_CHECKING:
    from collections.abc import Generator
    from importlib.resources.abc import Traversable
    from pathlib import Path


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


@pytest.mark.parametrize("optimization_level", [0, 1, 2, 3])
def test_verify_compilation_with_multi_controlled_gates(optimization_level: int) -> None:
    """Test compilation verification with Qiskit's default multi-controlled gate synthesis."""
    original_circuit = QuantumCircuit(6)
    original_circuit.h(range(5))
    original_circuit.mcx(list(range(5)), 5)
    original_circuit.mcp(1, list(range(5)), 5)
    original_circuit.measure_all()
    compiled_circuit = transpile(
        original_circuit,
        basis_gates=["id", "rz", "sx", "x", "cx"],
        optimization_level=optimization_level,
        seed_transpiler=12345,
    )
    result = verify_compilation(
        original_circuit,
        compiled_circuit,
        optimization_level,
        run_simulation_checker=False,
        run_zx_checker=False,
    )
    assert result.equivalence in {
        EquivalenceCriterion.equivalent,
        EquivalenceCriterion.equivalent_up_to_global_phase,
    }


@pytest.mark.parametrize("as_keyword", [False, True])
def test_custom_compilation_profile(original_circuit: QuantumCircuit, tmp_path: Path, as_keyword: bool) -> None:
    """Test that explicit profiles remain selected and are read by the checker."""
    config = Configuration()
    config.execution.run_construction_checker = True
    config.execution.run_alternating_checker = False
    config.execution.run_simulation_checker = False
    config.execution.run_zx_checker = False
    profile = tmp_path / "custom.profile"
    kwargs = {"profile": str(profile)} if as_keyword else {}
    if not as_keyword:
        config.application.profile = str(profile)
    with pytest.raises(ValueError, match="Error opening LUT file"):
        verify_compilation(original_circuit, original_circuit, configuration=config, **kwargs)
    profile.write_text("x 1 1\n")
    result = verify_compilation(original_circuit, original_circuit, configuration=config, **kwargs)
    assert result.considered_equivalent()
    assert config.application.profile == str(profile)


def test_default_profile_does_not_persist(original_circuit: QuantumCircuit) -> None:
    """Test that bundled profiles do not persist when reusing a configuration."""
    config = Configuration()
    for level in (1, 2):
        result = verify_compilation(original_circuit, original_circuit, optimization_level=level, configuration=config)
        assert result.considered_equivalent()
        assert not config.application.profile


@pytest.mark.parametrize("missing", [False, True])
def test_extracted_profile_lifetime(
    original_circuit: QuantumCircuit, tmp_path: Path, monkeypatch: pytest.MonkeyPatch, missing: bool
) -> None:
    """Test temporary profile lifetime and configuration restoration on failure."""
    extracted = tmp_path / "extracted.profile"

    @contextmanager
    def extract_profile(ref: Traversable) -> Generator[Path, None, None]:
        if not missing:
            extracted.write_bytes(ref.read_bytes())
        try:
            yield extracted
        finally:
            extracted.unlink(missing_ok=True)

    monkeypatch.setattr(resources, "as_file", extract_profile)
    config = Configuration()
    config.execution.run_construction_checker = True
    config.execution.run_alternating_checker = False
    config.execution.run_simulation_checker = False
    config.execution.run_zx_checker = False
    if missing:
        with pytest.raises(ValueError, match="Error opening LUT file"):
            verify_compilation(original_circuit, original_circuit, configuration=config)
    else:
        result = verify_compilation(original_circuit, original_circuit, configuration=config)
        assert result.considered_equivalent()
    assert not extracted.exists()
    assert not config.application.profile
