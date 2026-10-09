# Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Test compilation flow profiles."""

from __future__ import annotations

import os
from importlib import resources
from typing import TYPE_CHECKING

import pytest
from qiskit import transpile
from qiskit.circuit import QuantumCircuit

from mqt.qcec import verify_compilation
from mqt.qcec.compilation_flow_profiles import generate_profile, generate_profile_name

if TYPE_CHECKING:
    from pathlib import Path


@pytest.mark.parametrize("optimization_level", [0, 1, 2, 3])
def test_profile_name(optimization_level: int) -> None:
    """Test profile names for each optimization level."""
    assert generate_profile_name(optimization_level) == f"qiskit_O{optimization_level}.profile"


def test_default_profile_name() -> None:
    """Test the profile name for the default optimization level."""
    assert generate_profile_name() == "qiskit_O1.profile"


def test_default_profile_generation(tmp_path: Path) -> None:
    """Test the default profile and its multi-controlled gate costs."""
    generate_profile(filepath=tmp_path)
    profile = tmp_path / "qiskit_O1.profile"
    costs = {
        (gate, int(controls)): int(cost)
        for gate, controls, cost in (line.split() for line in profile.read_text(encoding="utf-8").splitlines()[1:])
    }
    assert costs["x", 0] == 1
    controls = 5
    mcx = QuantumCircuit(controls + 1)
    mcx.mcx(list(range(controls)), controls)
    mcp = QuantumCircuit(controls + 1)
    mcp.mcp(1, list(range(controls)), controls)
    for gate, circuit in [("x", mcx), ("p", mcp)]:
        compiled = transpile(
            circuit,
            basis_gates=["id", "rz", "sx", "x", "cx"],
            optimization_level=1,
            seed_transpiler=12345,
        )
        assert costs[gate, controls] == compiled.size()


@pytest.mark.skipif(
    os.environ.get("CHECK_PROFILES") is None,
    reason="This test is only executed if the CHECK_PROFILES environment variable is set.",
)
@pytest.mark.parametrize("optimization_level", [0, 1, 2, 3])
def test_generated_profiles_are_still_valid(optimization_level: int, tmp_path: Path) -> None:
    """Test validity of generated profiles.

    The main intention of this check is to catch cases where an update in Qiskit changes the respective costs.
    """
    generate_profile(optimization_level, tmp_path)

    profile_name = generate_profile_name(optimization_level)
    ref = resources.files("mqt.qcec") / "profiles" / profile_name

    with resources.as_file(ref) as path:
        # The header contains the file path and Qiskit version, which can differ.
        ref_profile = path.read_text(encoding="utf-8").splitlines()[1:]
        gen_profile = (tmp_path / profile_name).read_text(encoding="utf-8").splitlines()[1:]
        assert gen_profile == ref_profile, (
            f"The generated profile {profile_name} differs from the reference profile {ref}. "
            f"This might be due to a change in Qiskit. If this is the case, the reference profile should be updated."
        )


@pytest.mark.parametrize("basis_gates", [["rx", "rz", "cz"], ["rz", "sx", "x", "ecr"]])
def test_custom_basis_profile(tmp_path: Path, basis_gates: list[str]) -> None:
    """Test coexistence, gate costs, and verification of custom basis profiles."""
    generate_profile(filepath=tmp_path)
    default_profile = tmp_path / generate_profile_name()
    default_data = default_profile.read_bytes()
    generate_profile(filepath=tmp_path, basis_gates=basis_gates)
    name = generate_profile_name(basis_gates=basis_gates)
    assert name != default_profile.name
    assert name == generate_profile_name(basis_gates=[*reversed(basis_gates), basis_gates[0]])
    assert default_profile.read_bytes() == default_data
    costs = {
        (gate, int(controls)): int(cost)
        for gate, controls, cost in (line.split() for line in (tmp_path / name).read_text().splitlines()[1:])
    }
    native_gate = ("z", 1) if "cz" in basis_gates else ("ecr", 0)
    assert costs[native_gate] == 1
    mcx = QuantumCircuit(6)
    mcx.mcx(list(range(5)), 5)
    compiled = transpile(mcx, basis_gates=basis_gates, optimization_level=1, seed_transpiler=12345)
    assert costs["x", 5] == compiled.size()
    mcx.measure_all()
    compiled.measure_all()
    result = verify_compilation(
        mcx, compiled, profile=str(tmp_path / name), run_simulation_checker=False, run_zx_checker=False
    )
    assert result.considered_equivalent()


def test_explicit_default_basis_profile_name() -> None:
    """Test that gate order does not affect the default profile name."""
    assert generate_profile_name(basis_gates=["cx", "sx", "rz", "x", "id"]) == generate_profile_name()


@pytest.mark.parametrize("basis_gates", [[], ["../cx"], [""]])
def test_invalid_basis_profile_name(basis_gates: list[str]) -> None:
    """Test rejection of empty bases and invalid gate identifiers."""
    with pytest.raises(ValueError, match="gate identifiers"):
        generate_profile_name(basis_gates=basis_gates)
