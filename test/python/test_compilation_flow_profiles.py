# Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Test compilation flow profiles."""

from __future__ import annotations

import difflib
import filecmp
import locale
import os
import sys
from pathlib import Path
from typing import Any, cast

import pytest

from mqt.qcec._compat.importlib import resources
from mqt.qcec._compat.optional import HAS_QISKIT
from mqt.qcec.compilation_flow_profiles import AncillaMode, generate_profile, generate_profile_name


@pytest.fixture(params=[0, 1, 2, 3])
def optimization_level(request: Any) -> int:  # noqa: ANN401
    """Fixture for optimization levels."""
    return cast("int", request.param)


@pytest.fixture(params=[AncillaMode.NO_ANCILLA, AncillaMode.RECURSION, AncillaMode.V_CHAIN])
def ancilla_mode(request: Any) -> AncillaMode:  # noqa: ANN401
    """Fixture for ancilla modes."""
    return cast("AncillaMode", request.param)


def test_ancilla_mode_conversion(ancilla_mode: AncillaMode) -> None:
    """Test conversion and equality of ancilla modes."""
    ancilla_str = str(ancilla_mode)
    assert AncillaMode(ancilla_str) == ancilla_mode
    assert ancilla_str == ancilla_mode
    assert ancilla_mode == ancilla_str


@pytest.mark.skipif(
    os.environ.get("CHECK_PROFILES") is None,
    reason="This test is only executed if the CHECK_PROFILES environment variable is set.",
)
def test_generated_profiles_are_still_valid(optimization_level: int, ancilla_mode: AncillaMode) -> None:
    """Test validity of generated profiles.

    The main intention of this check is to catch cases where an update in Qiskit changes the respective costs.
    """
    # generate the profile
    with pytest.warns(
        expected_warning=DeprecationWarning,
        match=r"(.*?) is deprecated as of Qiskit 2.1",
    ):
        generate_profile(optimization_level=optimization_level, mode=ancilla_mode, filepath=Path())

    # get path to the profile from the package resources
    profile_name = generate_profile_name(optimization_level=optimization_level, mode=ancilla_mode)
    ref = resources.files("mqt.qcec") / "profiles" / profile_name

    # compare the generated profile with the reference profile
    with resources.as_file(ref) as path:
        equal = filecmp.cmp(path, profile_name, shallow=False)

        if equal:
            return

        ref_profile = path.read_text().splitlines(keepends=True)
        gen_profile = (
            Path(profile_name).read_text(encoding=locale.getpreferredencoding(False)).splitlines(keepends=True)
        )
        diff = difflib.unified_diff(ref_profile, gen_profile, fromfile="reference", tofile="generated", n=0)
        num_diffs = sum(
            1
            for line in diff
            if (
                line.find("Qiskit version") == -1
                and (
                    (line.startswith("-") and not line.startswith("---"))
                    or (line.startswith("+") and not line.startswith("+++"))
                )
            )
        )
        sys.stdout.writelines(diff)

        assert num_diffs <= 1, (
            f"The generated profile {profile_name} differs from the reference profile {ref} by {num_diffs} lines. "
            f"This might be due to a change in Qiskit. If this is the case, the reference profile should be updated."
        )


@pytest.mark.filterwarnings("ignore:.*is deprecated as of Qiskit 2.1.* ")
@pytest.mark.filterwarnings("ignore:.*Implicit conversion to integers .* is deprecated.*")
def test_deprecation_warning() -> None:
    """Tests that a deprecation warning is raised when the ``mode`` argument is passed."""
    with pytest.warns(
        expected_warning=DeprecationWarning,
        match=r"``mqt.qcec`` has deprecated the ``mode`` argument",
    ):
        generate_profile(0, mode=AncillaMode.V_CHAIN, filepath=Path())


def test_compilation_flow_profile_generation_fails_without_qiskit() -> None:
    """Test that profile generation fails if Qiskit is not available."""
    with HAS_QISKIT.disable_locally(), pytest.raises(ImportError, match=r"The 'qiskit' library is required to .*"):
        generate_profile()
