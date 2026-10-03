# Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Tests for checks with a hard deadline."""

from __future__ import annotations

import io
import json
import os
import sys
from typing import TYPE_CHECKING, cast

import pytest
from mqt.core.ir import QuantumComputation
from qiskit.circuit import Parameter, QuantumCircuit

from mqt.qcec import verify_with_hard_timeout
from mqt.qcec.pyqcec import ApplicationScheme, StateType
from mqt.qcec.verify_hard_timeout import _decode_circuit, _encode_circuit, _run_worker

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


@pytest.mark.parametrize(
    "qasm",
    [
        'OPENQASM 2.0; include "qelib1.inc"; qreg q[1]; x q[0];',
        "OPENQASM 3.0; qubit[1] q; x q[0];",
    ],
)
def test_verify_with_hard_timeout_qasm_text(qasm: str) -> None:
    """Accept OpenQASM text in either supported version."""
    result = verify_with_hard_timeout(qasm, qasm, deadline=5)
    assert result["equivalence"] == "equivalent"


def test_verify_with_hard_timeout_qiskit() -> None:
    """Accept Qiskit circuits alone or mixed with OpenQASM text."""
    circuit = QuantumCircuit(1)
    circuit.x(0)
    qasm = "OPENQASM 3.0; qubit[1] q; x q[0];"

    for first, second in ((circuit, circuit), (qasm, circuit)):
        result = verify_with_hard_timeout(first, second, deadline=5)
        assert result["equivalence"] == "equivalent"

    symbolic = QuantumCircuit(1)
    symbolic.rx(Parameter("theta"), 0)
    assert verify_with_hard_timeout(symbolic, symbolic, deadline=5)["equivalence"] == "equivalent"


def test_verify_with_hard_timeout_kills_worker(tmp_path: Path) -> None:
    """Kill a worker before it can finish starting."""
    circuit = QuantumComputation(1)
    path = tmp_path / "circuit.qasm"
    path.write_text(circuit.qasm3_str())

    with pytest.raises(TimeoutError, match="deadline"):
        verify_with_hard_timeout(path, path, deadline=0.001)


def test_verify_with_hard_timeout_errors(tmp_path: Path) -> None:
    """Reject invalid inputs and report worker failures."""

    class BytesPath(os.PathLike[bytes]):
        def __fspath__(self) -> bytes:
            return b"circuit.qasm"

    path = tmp_path / "missing.qasm"
    with pytest.raises(ValueError, match="positive finite"):
        verify_with_hard_timeout(path, path, deadline=0)
    with pytest.raises(TypeError, match="string paths"):
        verify_with_hard_timeout(cast("str", BytesPath()), path, deadline=1)
    with pytest.raises(TypeError, match="Qiskit QuantumCircuit"):
        verify_with_hard_timeout(cast("str", QuantumComputation(1)), path, deadline=1)
    with pytest.raises(RuntimeError, match="missing"):
        verify_with_hard_timeout(path, path, deadline=5)


def test_hard_timeout_worker_decoding(monkeypatch: pytest.MonkeyPatch) -> None:
    """Decode mixed circuit inputs and checker options in the worker."""
    circuit = QuantumCircuit(1)
    circuit.x(0)
    qasm = "OPENQASM 3.0; qubit[1] q; x q[0];"
    request = [
        [_encode_circuit(qasm), _encode_circuit(circuit)],
        {"alternating_scheme": "one_to_one", "state_type": "computational_basis"},
    ]
    output = io.StringIO()
    monkeypatch.setattr(sys, "stdin", io.StringIO(json.dumps(request)))
    monkeypatch.setattr(sys, "stdout", output)

    _run_worker()

    assert json.loads(output.getvalue())["equivalence"] == "equivalent"
    with pytest.raises(ValueError, match="unknown circuit format"):
        _decode_circuit(["unknown", ""])
