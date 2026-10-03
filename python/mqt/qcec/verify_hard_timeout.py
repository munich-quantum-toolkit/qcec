# Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Run equivalence checks in a terminable process."""

from __future__ import annotations

import base64
import io
import json
import math
import os
import subprocess  # ruff: ignore[suspicious-subprocess-import]  # A child process is required for forced termination.
import sys
from typing import TYPE_CHECKING, Any, Unpack, cast

from .pyqcec import ApplicationScheme, StateType
from .verify import verify

if TYPE_CHECKING:
    from qiskit.circuit import QuantumCircuit

    from .configuration_options import ConfigurationOptions

__all__ = ["verify_with_hard_timeout"]


def verify_with_hard_timeout(
    circ1: str | os.PathLike[str] | QuantumCircuit,
    circ2: str | os.PathLike[str] | QuantumCircuit,
    deadline: float,
    **kwargs: Unpack[ConfigurationOptions],
) -> dict[str, Any]:
    """Verify two circuits in a process that is killed after ``deadline`` seconds.

    Use this function only when a check needs forced termination. For other
    checks, prefer :func:`verify` with its cooperative ``timeout`` option. A
    worker process adds substantial overhead, especially for Qiskit inputs.

    Inputs may be file paths, OpenQASM text, or Qiskit circuits. Qiskit circuits
    are serialized with QPY before the worker starts. The timeout covers worker
    startup and circuit loading after process creation, but not serialization
    in the caller. Process creation itself may delay the timeout on some platforms.
    Successful checks return the JSON result of :func:`verify`, including checker
    statistics but no DD counterexamples. A deadline raises :class:`TimeoutError`;
    other worker failures raise :class:`RuntimeError`.

    Args:
        circ1: First circuit as a file path, OpenQASM text, or Qiskit circuit.
        circ2: Second circuit in the same supported forms.
        deadline: Positive worker timeout in seconds.
        **kwargs: Options accepted by :func:`verify`.

    Returns:
        The verification result as a JSON-style dictionary.
    """
    if not math.isfinite(deadline) or deadline <= 0:
        msg = "deadline must be a positive finite number"
        raise ValueError(msg)

    options = {
        key: value.name if isinstance(value, (ApplicationScheme, StateType)) else value for key, value in kwargs.items()
    }
    payload = json.dumps([[_encode_circuit(circ1), _encode_circuit(circ2)], options], allow_nan=False)
    command = [sys.executable, "-c", "from mqt.qcec.verify_hard_timeout import _run_worker; _run_worker()"]
    try:
        completed = subprocess.run(  # ruff: ignore[subprocess-without-shell-equals-true]  # Fixed code and interpreter; circuits travel over stdin.
            command, input=payload, text=True, capture_output=True, check=True, timeout=deadline, shell=False
        )
    except subprocess.TimeoutExpired as exc:
        msg = f"equivalence check exceeded the {deadline:g} s deadline"
        raise TimeoutError(msg) from exc
    except subprocess.CalledProcessError as exc:
        msg = exc.stderr.strip() or "equivalence check worker failed"
        raise RuntimeError(msg) from exc

    sys.stderr.write(completed.stderr)
    return cast("dict[str, Any]", json.loads(completed.stdout))


def _encode_circuit(circuit: str | os.PathLike[str] | QuantumCircuit) -> tuple[str, str]:
    if isinstance(circuit, (str, os.PathLike)):
        source = os.fspath(circuit)
        if not isinstance(source, str):
            msg = "circuit paths must be string paths"
            raise TypeError(msg)
        return "source", source

    from qiskit import qpy  # ruff: ignore[import-outside-top-level] optional dependency
    from qiskit.circuit import QuantumCircuit  # ruff: ignore[import-outside-top-level] optional dependency

    if not isinstance(circuit, QuantumCircuit):
        msg = "circuit must be a file path, OpenQASM text, or Qiskit QuantumCircuit"
        raise TypeError(msg)
    output = io.BytesIO()
    qpy.dump(circuit, output)
    return "qpy", base64.b64encode(output.getvalue()).decode("ascii")


def _decode_circuit(encoded: list[str]) -> str | QuantumCircuit:
    kind, data = encoded
    if kind == "source":
        return data
    if kind != "qpy":
        msg = f"unknown circuit format: {kind}"
        raise ValueError(msg)
    from qiskit import qpy  # ruff: ignore[import-outside-top-level] optional dependency

    return qpy.load(io.BytesIO(base64.b64decode(data)))[0]


def _run_worker() -> None:
    encoded, options = json.load(sys.stdin)
    circuits = [_decode_circuit(circuit) for circuit in encoded]
    for key in ("alternating_scheme", "construction_scheme", "simulation_scheme"):
        if key in options:
            options[key] = getattr(ApplicationScheme, options[key])
    if "state_type" in options:
        options["state_type"] = getattr(StateType, options["state_type"])
    json.dump(verify(circuits[0], circuits[1], **options).json(), sys.stdout)
