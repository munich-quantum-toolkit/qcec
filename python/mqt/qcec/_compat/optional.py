# Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Optional dependency tester.

Inspired by Qiskit's `LazyDependencyManager <https://github.com/Qiskit/qiskit/blob/f13673b05edf98263f80a174d2e13a118b4acda7/qiskit/utils/lazy_tester.py#L44>`_.
"""

from __future__ import annotations

import contextlib
import importlib
import typing
import warnings

__all__ = ["HAS_QISKIT", "OptionalDependencyTester"]


def __dir__() -> list[str]:
    return __all__


class OptionalDependencyTester:
    """A manager for optional dependencies to assert their availability.

    This class is used to lazily test for the availability of optional dependencies.
    It can be used in Boolean contexts to check if the dependency is available.
    """

    def __init__(self, module: str, *, msg: str | None = None) -> None:
        """Construct a new optional dependency tester.

        Args:
            module: the name of the module to test for.
            msg: an extra message to include in the error raised if this is required.
        """
        self._module = module
        self._bool: bool | None = None
        self._msg = msg

    def _is_available(self) -> bool:
        """Test the availability of the module.

        Returns:
            ``True`` if the module is available, ``False`` otherwise.
        """
        try:
            importlib.import_module(self._module)
        except ImportError as exc:  # pragma: no cover
            warnings.warn(
                f"Module '{self._module}' failed to import with: {exc!r}",
                category=UserWarning,
                stacklevel=2,
            )
            return False
        else:
            return True

    def __bool__(self) -> bool:
        """Check if the dependency is available.

        Returns:
            ``True`` if the dependency is available, ``False`` otherwise.
        """
        if self._bool is None:
            self._bool = self._is_available()
        return self._bool

    def require_now(self, feature: str) -> None:
        """Eagerly attempt to import the dependency and raise an exception if it cannot be imported.

        Args:
            feature: the feature that is requiring this dependency.

        Raises:
            ImportError: if the dependency cannot be imported.
        """
        if self:
            return
        message = f"The '{self._module}' library is required to {feature}."
        if self._msg:
            message += f" {self._msg}."
        raise ImportError(message)

    @contextlib.contextmanager
    def disable_locally(self) -> typing.Generator[None, None, None]:
        """Create a context during which the value of the dependency manager will be ``False``.

        Yields:
            None
        """
        previous = self._bool
        self._bool = False
        try:
            yield
        finally:
            self._bool = previous


HAS_QISKIT = OptionalDependencyTester(
    "qiskit",
    msg="Please install the `mqt.qcec[qiskit]` extra or a compatible version of Qiskit to use functionality related to its functionality.",
)
