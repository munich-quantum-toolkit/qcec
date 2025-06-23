# Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Functionality for checking equivalence of parameterized quantum circuits."""

from __future__ import annotations

import time
from itertools import chain
from typing import TYPE_CHECKING, cast

import numpy as np
from mqt.core.ir.symbolic import Expression

from .pyqcec import EquivalenceCheckingManager, EquivalenceCriterion

if TYPE_CHECKING:
    from mqt.core.ir import QuantumComputation
    from mqt.core.ir.operations import SymbolicOperation
    from mqt.core.ir.symbolic import Variable
    from numpy.typing import NDArray

    from .pyqcec import Configuration

__all__ = [
    "check_instantiated",
    "check_instantiated_random",
    "check_parameterized",
    "check_parameterized_zx",
]


def __dir__() -> list[str]:
    return __all__


def __adjust_timeout(curr_timeout: float, res: EquivalenceCheckingManager.Results | float) -> float:
    if curr_timeout == 0:
        return 0

    if isinstance(res, EquivalenceCheckingManager.Results):
        return curr_timeout - (res.check_time + res.preprocessing_time)
    return curr_timeout - res


def check_parameterized_zx(
    circ1: QuantumComputation, circ2: QuantumComputation, configuration: Configuration
) -> EquivalenceCheckingManager.Results:
    """Check circuits for equivalence with the ZX-calculus."""
    ecm = EquivalenceCheckingManager(circ1, circ2, configuration)
    ecm.disable_all_checkers()
    ecm.configuration.execution.run_zx_checker = True
    ecm.run()
    return ecm.results


def __extract_params(
    circ1: QuantumComputation, circ2: QuantumComputation
) -> tuple[list[Variable], NDArray[np.float64], NDArray[np.float64]]:
    """Extract parameters and equations of parameterized circuits."""
    symbolic_params = list(circ1.variables.union(circ2.variables))
    num_symbolic_params = len(symbolic_params)

    symbolic_expressions = [
        param
        for op in chain(circ1, circ2)
        if op.is_symbolic_operation()
        for param in cast("SymbolicOperation", op).get_parameters()
        if isinstance(param, Expression) and not param.is_constant()
    ]
    num_symbolic_expressions = len(symbolic_expressions)

    offsets = np.zeros(num_symbolic_expressions)
    for row, expr in enumerate(symbolic_expressions):
        zero_map = dict.fromkeys(expr.variables, 0.0)
        offsets[row] = -float(expr.evaluate(zero_map))

    equations = np.zeros((num_symbolic_expressions, num_symbolic_params))
    for col, param in enumerate(symbolic_params):
        for row, expr in enumerate(symbolic_expressions):
            one_map = dict.fromkeys(expr.variables, 0.0)
            if param in expr.variables:
                one_map[param] = 1.0
                equations[row, col] = (expr + offsets[row]).evaluate(one_map)

    return symbolic_params, equations, offsets


def check_instantiated(
    circ1: QuantumComputation, circ2: QuantumComputation, configuration: Configuration
) -> EquivalenceCheckingManager.Results:
    """Check circuits for equivalence with DD equivalence checker."""
    ecm = EquivalenceCheckingManager(circ1, circ2, configuration)
    ecm.configuration.execution.run_zx_checker = False
    ecm.run()
    return ecm.results


def check_instantiated_random(
    circ1: QuantumComputation, circ2: QuantumComputation, params: list[Variable], configuration: Configuration
) -> EquivalenceCheckingManager.Results:
    """Check whether circuits are equivalent for random instantiation of symbolic parameters."""
    param_map = {}
    rng = np.random.default_rng()
    for p in params:
        param_map[p] = rng.random() * 2 * np.pi

    circ1_inst = circ1.instantiate(param_map)
    circ2_inst = circ2.instantiate(param_map)

    return check_instantiated(circ1_inst, circ2_inst, configuration)


def check_parameterized(
    circ1: QuantumComputation, circ2: QuantumComputation, configuration: Configuration
) -> EquivalenceCheckingManager.Results:
    """Equivalence checking flow for parameterized circuit."""
    total_preprocessing_time = 0.0
    total_runtime = 0.0
    total_simulations_started = 0
    total_simulations_finished = 0

    def __update_stats(result: EquivalenceCheckingManager.Results) -> None:
        nonlocal total_preprocessing_time, total_runtime, total_simulations_started, total_simulations_finished
        total_preprocessing_time += result.preprocessing_time
        total_runtime += result.check_time
        total_simulations_started += result.started_simulations
        total_simulations_finished += result.performed_simulations

    def __write_stats(instantiation: int, result: EquivalenceCheckingManager.Results) -> None:
        nonlocal total_preprocessing_time, total_runtime, total_simulations_started, total_simulations_finished
        result.check_time = total_runtime
        result.preprocessing_time = total_preprocessing_time
        result.started_simulations = total_simulations_started
        result.performed_simulations = total_simulations_finished
        result.performed_instantiations = instantiation

    res = check_parameterized_zx(circ1, circ2, configuration)

    if res.considered_equivalent():
        return res

    __update_stats(res)

    timeout = __adjust_timeout(configuration.execution.timeout, res)
    n_checks = configuration.parameterized.additional_instantiations
    tol = configuration.parameterized.parameterized_tolerance

    parameters, mat, offsets = __extract_params(circ1, circ2)

    def __instantiate_params(
        qc1: QuantumComputation, qc2: QuantumComputation, b: NDArray[np.float64]
    ) -> tuple[QuantumComputation, QuantumComputation, float]:
        start_time = time.time()
        mat_pinv = np.linalg.pinv(mat)
        x = np.dot(mat_pinv, b)
        param_map = {param: x[i] for i, param in enumerate(parameters)}
        qc1_bound = qc1.instantiate(param_map)
        qc2_bound = qc2.instantiate(param_map)

        def __round_zero_params(qc: QuantumComputation) -> None:
            for op in qc:
                if len(op.parameter) == 0:
                    continue

                for i, param in enumerate(op.parameter):
                    if np.abs(param) < tol:
                        op.parameter[i] = 0

        __round_zero_params(qc1_bound)
        __round_zero_params(qc2_bound)
        return qc1_bound, qc2_bound, time.time() - start_time

    def __instantiate_params_zero(
        qc1: QuantumComputation, qc2: QuantumComputation
    ) -> tuple[QuantumComputation, QuantumComputation, float]:
        return __instantiate_params(qc1, qc2, offsets)

    circ1_inst, circ2_inst, runtime = __instantiate_params_zero(circ1, circ2)
    timeout = __adjust_timeout(timeout, runtime)

    if timeout < 0:
        __write_stats(1, res)
        res.equivalence = EquivalenceCriterion.no_information
        return res

    res = check_instantiated(circ1_inst, circ2_inst, configuration)
    __update_stats(res)
    if res.equivalence == EquivalenceCriterion.not_equivalent:
        __write_stats(1, res)
        return res

    def __instantiate_params_phases(
        qc1: QuantumComputation, qc2: QuantumComputation
    ) -> tuple[QuantumComputation, QuantumComputation, float]:
        phases = [0.0, np.pi, np.pi / 2, -np.pi / 2, np.pi / 4, -np.pi / 4]
        rng = np.random.default_rng()
        b = rng.choice(phases, size=len(offsets)) + offsets
        return __instantiate_params(qc1, qc2, b)

    for i in range(n_checks):
        circ1_inst, circ2_inst, runtime = __instantiate_params_phases(circ1, circ2)
        timeout = __adjust_timeout(timeout, runtime)
        res = check_instantiated(circ1_inst, circ2_inst, configuration)
        timeout = __adjust_timeout(timeout, res)

        if timeout < 0:
            __write_stats(i + 2, res)
            res.equivalence = EquivalenceCriterion.no_information
            return res

        __update_stats(res)

        if res.equivalence == EquivalenceCriterion.not_equivalent:
            __write_stats(i + 2, res)
            return res

    res = check_instantiated_random(circ1, circ2, parameters, configuration)
    timeout = __adjust_timeout(timeout, runtime)
    if timeout < 0:
        res.equivalence = EquivalenceCriterion.no_information

    __update_stats(res)
    __write_stats(n_checks + 2, res)
    return res
