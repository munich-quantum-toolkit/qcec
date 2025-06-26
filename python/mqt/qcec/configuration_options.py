# Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Configuration options for the equivalence checking manager."""

from __future__ import annotations

from typing import TYPE_CHECKING, TypedDict

if TYPE_CHECKING:
    from .pyqcec import ApplicationScheme, Configuration, StateType


__all__ = ["ConfigurationOptions", "augment_config_from_kwargs"]


def __dir__() -> list[str]:
    return __all__


class ConfigurationOptions(TypedDict, total=False):
    """A dictionary of configuration options.

    The keys of this dictionary are the names of the configuration options.
    The values are the values of the configuration options.
    """

    # Application
    alternating_scheme: ApplicationScheme | str
    construction_scheme: ApplicationScheme | str
    simulation_scheme: ApplicationScheme | str
    profile: str
    # Execution
    nthreads: int
    numerical_tolerance: float
    parallel: bool
    run_alternating_checker: bool
    run_construction_checker: bool
    run_simulation_checker: bool
    run_zx_checker: bool
    timeout: float
    # Functionality
    trace_threshold: float
    check_partial_equivalence: bool
    # Optimizations
    backpropagate_output_permutation: bool
    elide_permutations: bool
    fuse_single_qubit_gates: bool
    reconstruct_swaps: bool
    remove_diagonal_gates_before_measure: bool
    reorder_operations: bool
    transform_dynamic_circuit: bool
    # Parameterized
    additional_instantiations: int
    parameterized_tolerance: float
    # Simulation
    fidelity_threshold: float
    max_sims: int
    seed: int
    state_type: StateType | str


def augment_config_from_kwargs(config: Configuration, kwargs: ConfigurationOptions) -> None:
    """Augment an existing :class:`.Configuration` with options from a collection of keyword arguments.

    Args:
        config: The configuration to augment.
        kwargs: The arguments to build the configuration from.
    """
    for key, value in kwargs.items():
        if hasattr(config.application, key):
            setattr(config.application, key, value)
        elif hasattr(config.execution, key):
            setattr(config.execution, key, value)
        elif hasattr(config.functionality, key):
            setattr(config.functionality, key, value)
        elif hasattr(config.optimizations, key):
            setattr(config.optimizations, key, value)
        elif hasattr(config.parameterized, key):
            setattr(config.parameterized, key, value)
        elif hasattr(config.simulation, key):
            setattr(config.simulation, key, value)
        else:  # pragma: no cover
            msg = f"Invalid keyword argument: {key}"
            raise ValueError(msg)
