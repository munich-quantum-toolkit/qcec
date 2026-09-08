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

from mqt.qcec._compat.optional import HAS_QISKIT
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


def test_compilation_flow_profile_generation_fails_without_qiskit() -> None:
    """Test that profile generation fails if Qiskit is not available."""
    with HAS_QISKIT.disable_locally(), pytest.raises(ImportError, match=r"The 'qiskit' library is required to .*"):
        generate_profile()
