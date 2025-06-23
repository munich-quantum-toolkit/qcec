# Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Test the configuration of the equivalence checker."""

from __future__ import annotations

import pytest

from mqt.qcec.pyqcec import ApplicationScheme, Configuration, StateType


@pytest.mark.parametrize(
    ("application_scheme_string", "application_scheme_enum"),
    [
        ("sequential", ApplicationScheme.sequential),
        ("reference", ApplicationScheme.sequential),
        ("one_to_one", ApplicationScheme.one_to_one),
        ("naive", ApplicationScheme.one_to_one),
        ("lookahead", ApplicationScheme.lookahead),
        ("gate_cost", ApplicationScheme.gate_cost),
        ("compilation_flow", ApplicationScheme.gate_cost),
        ("proportional", ApplicationScheme.proportional),
    ],
)
def test_application_scheme(application_scheme_string: str, application_scheme_enum: ApplicationScheme) -> None:
    """Test the application scheme enum."""
    assert ApplicationScheme(application_scheme_string) == application_scheme_enum

    config = Configuration()

    config.application.construction_scheme = application_scheme_enum
    config.application.simulation_scheme = application_scheme_enum
    config.application.alternating_scheme = application_scheme_enum


def test_timeout() -> None:
    """Test the timeout configuration."""
    config = Configuration()
    config.execution.timeout = 60
    config.execution.timeout = 60.0


@pytest.mark.parametrize(
    ("state_type_string", "state_type_enum"),
    [
        ("computational_basis", StateType.computational_basis),
        ("classical", StateType.computational_basis),
        ("random_1Q_basis", StateType.random_1Q_basis),
        ("local_quantum", StateType.random_1Q_basis),
        ("stabilizer", StateType.stabilizer),
        ("global_quantum", StateType.stabilizer),
    ],
)
def test_state_type(state_type_string: str, state_type_enum: StateType) -> None:
    """Test the state type enum."""
    assert StateType(state_type_string) == state_type_enum

    config = Configuration()
    config.simulation.state_type = state_type_enum
