# Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Numerical failure must not establish equivalence or non-equivalence."""

from __future__ import annotations

import pytest
from mqt.core import load
from qiskit import QuantumCircuit, transpile

from mqt.qcec.pyqcec import Configuration, EquivalenceCheckingManager, EquivalenceCriterion


@pytest.fixture(scope="module", params=[82, 83])
def ghz_circuits(request: pytest.FixtureRequest) -> tuple[QuantumCircuit, QuantumCircuit, QuantumCircuit]:
    """Build GHZ and GHZ followed by X, with the translation from issue #1100."""
    circuit = QuantumCircuit(request.param)
    circuit.h(0)
    for qubit in range(request.param - 1):
        circuit.cx(qubit, qubit + 1)
    changed = circuit.copy()
    changed.x(request.param - 1)
    circuit.measure_all()
    changed.measure_all()
    translated, translated_changed = (
        transpile(qc, basis_gates=["cz", "rz", "sx", "x"], optimization_level=0, seed_transpiler=0)
        for qc in (circuit, changed)
    )
    return circuit, translated, translated_changed


@pytest.mark.parametrize(("partial", "ancillary"), [(False, False), (False, True), (True, True)])
@pytest.mark.parametrize("pair", [(1, 2), (0, 1), (1, 0)])
def test_construction_numerical_failure(
    ghz_circuits: tuple[QuantumCircuit, QuantumCircuit, QuantumCircuit],
    ancillary: bool,
    partial: bool,
    pair: tuple[int, int],
) -> None:
    """A collapsed DD gives no information, including before garbage reduction."""
    circuits = [load(ghz_circuits[index]) for index in pair]
    if ancillary:
        for circuit in circuits:
            for qubit in range(circuit.num_qubits):
                circuit.set_circuit_qubit_ancillary(qubit)
    config = Configuration()
    config.execution.run_construction_checker = True
    config.execution.run_alternating_checker = False
    config.execution.run_simulation_checker = False
    config.execution.run_zx_checker = False
    config.functionality.check_partial_equivalence = partial
    manager = EquivalenceCheckingManager(circuits[0], circuits[1], config)
    manager.run()
    result = manager.results.equivalence
    if pair == (1, 2):
        assert result in {EquivalenceCriterion.not_equivalent, EquivalenceCriterion.no_information}
    else:
        assert result == EquivalenceCriterion.no_information or manager.results.considered_equivalent()


@pytest.mark.parametrize("parallel", [False, True])
def test_numerical_failure_allows_simulation(
    ghz_circuits: tuple[QuantumCircuit, QuantumCircuit, QuantumCircuit], parallel: bool
) -> None:
    """An independent simulation can reject circuits after construction fails."""
    circuits = [load(circuit) for circuit in ghz_circuits[1:]]
    for circuit in circuits:
        for qubit in range(circuit.num_qubits):
            circuit.set_circuit_qubit_ancillary(qubit)
    config = Configuration()
    config.execution.parallel = parallel
    config.execution.nthreads = 2
    config.execution.run_construction_checker = True
    config.execution.run_alternating_checker = False
    config.execution.run_zx_checker = False
    config.simulation.max_sims = 1
    manager = EquivalenceCheckingManager(circuits[0], circuits[1], config)
    manager.run()
    assert manager.results.equivalence == EquivalenceCriterion.not_equivalent
