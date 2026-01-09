# Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""MQT QCEC library.

This file is part of the MQT QCEC library released under the MIT license.
See README.md or go to https://github.com/cda-tum/qcec for more information.
"""

from __future__ import annotations

import sys

# under Windows, make sure to add the appropriate DLL directory to the PATH
if sys.platform == "win32":  # noqa: RUF067 This is actually required on Windows

    def _dll_patch() -> None:
        """Add the DLL directory to the PATH."""
        import os  # noqa: PLC0415 because only needed on Windows
        import sysconfig  # noqa: PLC0415 because only needed on Windows
        from pathlib import Path  # noqa: PLC0415 because only needed on Windows

        bin_dir = Path(sysconfig.get_paths()["purelib"]) / "mqt" / "core" / "bin"
        os.add_dll_directory(str(bin_dir))

    _dll_patch()
    del _dll_patch


from ._version import version as __version__
from .verify import verify
from .verify_compilation_flow import verify_compilation

__all__ = [
    "__version__",
    "verify",
    "verify_compilation",
]
