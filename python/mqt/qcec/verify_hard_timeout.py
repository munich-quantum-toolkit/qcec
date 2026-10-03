# Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Run file-based equivalence checks in a terminable process."""

from __future__ import annotations

import json
import math
import os
import subprocess  # ruff: ignore[suspicious-subprocess-import]  # A child process is required for forced termination.
import sys
from typing import TYPE_CHECKING, Any, Unpack, cast

from .pyqcec import ApplicationScheme, StateType
from .verify import verify

if TYPE_CHECKING:
    from .configuration_options import ConfigurationOptions

__all__ = ["verify_with_hard_timeout"]


def verify_with_hard_timeout(
    circ1: str | os.PathLike[str],
    circ2: str | os.PathLike[str],
    deadline: float,
    **kwargs: Unpack[ConfigurationOptions],
) -> dict[str, Any]:
    """Verify two circuit files in a process that is killed after ``deadline`` seconds.

    The timeout covers worker startup and circuit loading after process creation.
    Process creation itself may delay the timeout on some platforms. Successful checks
    return the JSON result of :func:`verify`, including checker statistics. The
    JSON result does not contain DD counterexamples. A deadline raises
    :class:`TimeoutError`; other worker failures raise :class:`RuntimeError`.

    Args:
        circ1: Path to the first circuit.
        circ2: Path to the second circuit.
        deadline: Positive worker timeout in seconds.
        **kwargs: Options accepted by :func:`verify`.

    Returns:
        The verification result as a JSON-style dictionary.
    """
    if not math.isfinite(deadline) or deadline <= 0:
        msg = "deadline must be a positive finite number"
        raise ValueError(msg)

    paths = [os.fspath(circ1), os.fspath(circ2)]
    if any(not isinstance(path, str) for path in paths):
        msg = "circuit paths must be strings or path-like strings"
        raise TypeError(msg)

    options = {
        key: value.name if isinstance(value, (ApplicationScheme, StateType)) else value for key, value in kwargs.items()
    }
    payload = json.dumps([paths, options], allow_nan=False)
    command = [sys.executable, "-c", "from mqt.qcec.verify_hard_timeout import _run_worker; _run_worker()"]
    try:
        completed = subprocess.run(  # ruff: ignore[subprocess-without-shell-equals-true]  # Fixed code and interpreter; paths travel over stdin.
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


def _run_worker() -> None:
    paths, options = json.load(sys.stdin)
    for key in ("alternating_scheme", "construction_scheme", "simulation_scheme"):
        if key in options:
            options[key] = getattr(ApplicationScheme, options[key])
    if "state_type" in options:
        options["state_type"] = getattr(StateType, options["state_type"])
    json.dump(verify(*paths, **options).json(), sys.stdout)
