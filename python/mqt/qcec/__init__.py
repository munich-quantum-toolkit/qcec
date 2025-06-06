# Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
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
if sys.platform == "win32":

    def _dll_patch() -> None:
        """Add the DLL directory to the PATH."""
        import os
        import sysconfig
        from pathlib import Path

        bin_dir = Path(sysconfig.get_paths()["purelib"]) / "mqt" / "core" / "bin"
        os.add_dll_directory(str(bin_dir))

    _dll_patch()
    del _dll_patch

from mqt.qcec.application_scheme import ApplicationScheme
from mqt.qcec.configuration import Configuration
from mqt.qcec.configuration_options import augment_config_from_kwargs
from mqt.qcec.equivalence_checking_manager import EquivalenceCheckingManager
from mqt.qcec.equivalence_criterion import EquivalenceCriterion
from mqt.qcec.state_type import StateType
from mqt.qcec.verify import verify
from mqt.qcec.verify_compilation_flow import verify_compilation

from ._version import version as __version__

__all__ = [
    "ApplicationScheme",
    "Configuration",
    "EquivalenceCheckingManager",
    "EquivalenceCriterion",
    "StateType",
    "__version__",
    "augment_config_from_kwargs",
    "verify",
    "verify_compilation",
]


def __dir__() -> list[str]:
    return __all__
