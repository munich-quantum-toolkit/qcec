# Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""MQT QCEC library.

This file is part of the MQT QCEC library released under the MIT license.
See README.md or go to https://github.com/munich-quantum-toolkit/qcec for more information.
"""

from __future__ import annotations

import sys

# On Windows, add MQT Core's DLL directory to the search path.
if sys.platform == "win32":  # ruff:ignore[non-empty-init-module] This is actually required on Windows
    import os
    import sysconfig
    from pathlib import Path

    def _dll_patch() -> None:
        bin_dir = Path(sysconfig.get_paths()["purelib"]) / "mqt" / "core" / "bin"
        os.add_dll_directory(str(bin_dir))

    _dll_patch()
    del _dll_patch


from ._version import version as __version__
from .verify import verify
from .verify_compilation_flow import verify_compilation
from .verify_hard_timeout import verify_with_hard_timeout

__all__ = [
    "__version__",
    "verify",
    "verify_compilation",
    "verify_with_hard_timeout",
]
