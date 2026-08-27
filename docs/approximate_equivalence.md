---
file_format: mystnb
kernelspec:
  name: python3
mystnb:
  number_source_lines: true
---

# Approximate Equivalence Checking

## Approximate and exact equivalence

Two quantum circuits are exactly equivalent if their unitary matrix
representations, $U$ and $V$, are identical up to the equivalence relation of
interest. For full unitary equivalence up to global phase, this can be checked
by determining whether $UV^\dagger$ is the identity up to global phase.

In approximate synthesis and optimization, it is often useful to accept a
circuit that is sufficiently close to the original. QCEC quantifies this using
the normalized-trace distance

$$
\Delta(U, V) = 1 - \left|\frac{\operatorname{Tr}(UV^\dagger)}{2^n}\right|,
$$

where $n$ is the number of qubits. The distance is invariant under global phase
and ranges from zero to one for unitary matrices. QCEC considers two circuits
approximately equivalent when their distance is smaller than the configured
threshold $\epsilon$.

## Supported checkers

Set `check_approximate_equivalence` to `True` to enable approximate checking.
The `approximate_checking_threshold` option controls the accepted distance and
defaults to `1e-8`.

The construction and alternating checkers compute the normalized trace using
decision diagrams. The simulation checker compares individual output states, so
its fidelity threshold is not an approximate process-distance threshold. Disable
the simulation checker when using approximate equivalence checking. The
ZX-calculus checker does not directly support approximate equivalence and may
return `no_information` for circuits that are close but not exactly equivalent.

+++

## Example

Consider a three-qubit Toffoli gate and the identity circuit.

```{code-cell} ipython3
from qiskit import QuantumCircuit

qc_lhs = QuantumCircuit(3)
qc_lhs.mcx([0, 1], 2)

qc_rhs = QuantumCircuit(3)
qc_rhs.id(range(3))

qc_lhs.draw(output="mpl", style="iqp")
```

The normalized trace of the Toffoli matrix is $0.75$, so its normalized-trace
distance from the identity is $0.25$. A threshold of $0.3$ therefore accepts the
pair:

```{code-cell} ipython3
from mqt.qcec import verify
from mqt.qcec.pyqcec import Configuration

config = Configuration()
config.execution.run_simulation_checker = False
config.functionality.check_approximate_equivalence = True
config.functionality.approximate_checking_threshold = 0.3

verify(qc_lhs, qc_rhs, configuration=config)
```

A threshold below $0.25$ rejects the same pair:

```{code-cell} ipython3
config.functionality.approximate_checking_threshold = 0.2
verify(qc_lhs, qc_rhs, configuration=config)
```

The approach is inspired by work on
[approximate equivalence checking](https://arxiv.org/abs/2103.11595) and
[approximate quantum-circuit synthesis](https://doi.org/10.1145/3503222.3507739).
