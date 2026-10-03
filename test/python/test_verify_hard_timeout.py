# Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Tests for file-based checks with a hard deadline."""

from __future__ import annotations

import io
import json
import sys
from typing import TYPE_CHECKING, cast

import pytest
from mqt.core.ir import QuantumComputation

from mqt.qcec import verify_with_hard_timeout
from mqt.qcec.pyqcec import ApplicationScheme, StateType
from mqt.qcec.verify_hard_timeout import _run_worker

if TYPE_CHECKING:
    from pathlib import Path


def test_verify_with_hard_timeout(tmp_path: Path) -> None:
    """Return checker details from a completed worker."""
    circuit = QuantumComputation(1)
    circuit.x(0)
    path = tmp_path / "circuit.qasm"
    path.write_text(circuit.qasm3_str())

    result = verify_with_hard_timeout(
        path,
        path,
        deadline=5,
        parallel=False,
        run_simulation_checker=False,
        run_zx_checker=False,
        alternating_scheme=ApplicationScheme.one_to_one,
        state_type=StateType.computational_basis,
    )

    assert result["equivalence"] == "equivalent"
    assert result["checkers"][0]["checker"] == "decision_diagram_alternating"


def test_verify_with_hard_timeout_kills_worker(tmp_path: Path) -> None:
    """Kill a worker before it can finish starting."""
    circuit = QuantumComputation(1)
    path = tmp_path / "circuit.qasm"
    path.write_text(circuit.qasm3_str())

    with pytest.raises(TimeoutError, match="deadline"):
        verify_with_hard_timeout(path, path, deadline=0.001)


def test_verify_with_hard_timeout_errors(tmp_path: Path) -> None:
    """Reject invalid inputs and report worker failures."""
    path = tmp_path / "missing.qasm"
    with pytest.raises(ValueError, match="positive finite"):
        verify_with_hard_timeout(path, path, deadline=0)
    with pytest.raises(TypeError, match="paths must be strings"):
        verify_with_hard_timeout(cast("str", b"circuit.qasm"), path, deadline=1)
    with pytest.raises(RuntimeError, match="missing"):
        verify_with_hard_timeout(path, path, deadline=5)


def test_hard_timeout_worker_protocol(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    """Decode enum options and emit the verifier's JSON result."""
    circuit = QuantumComputation(1)
    circuit.x(0)
    path = tmp_path / "circuit.qasm"
    path.write_text(circuit.qasm3_str())
    request = [
        [str(path), str(path)],
        {
            "parallel": False,
            "run_simulation_checker": False,
            "run_zx_checker": False,
            "alternating_scheme": "one_to_one",
            "state_type": "computational_basis",
        },
    ]
    output = io.StringIO()
    monkeypatch.setattr(sys, "stdin", io.StringIO(json.dumps(request)))
    monkeypatch.setattr(sys, "stdout", output)

    _run_worker()

    assert json.loads(output.getvalue())["equivalence"] == "equivalent"
