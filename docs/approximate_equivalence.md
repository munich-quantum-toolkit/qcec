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
the projective Hilbert--Schmidt distance

$$
D_\mathrm{HS}(U, V) =
\sqrt{1 - \left|\frac{\operatorname{Tr}(UV^\dagger)}{2^n}\right|^2},
$$

where $n$ is the number of qubits. The distance is invariant under global phase
and ranges from zero to one for unitary matrices. QCEC considers two circuits
approximately equivalent when their distance is at most the configured threshold
$\epsilon$.

## Supported checkers

Set `check_approximate_equivalence` to `True` to enable approximate checking.
The `approximate_checking_threshold` option controls the accepted distance and
defaults to `1e-8`. It must be finite and lie in the closed interval `[0, 1]`.

The construction and alternating checkers compute the normalized trace using
decision diagrams. At least one of these two checkers must be enabled. The
simulation checker compares individual output states, so its fidelity threshold
does not represent the configured process distance; QCEC disables it
automatically in approximate mode. QCEC also disables the ZX-calculus checker
because it cannot establish approximate non-equivalence.

Approximate checking currently supports fixed, full-unitary circuits without
ancillary or garbage qubits. Parameterized circuits and partial equivalence use
different equivalence relations and are rejected when approximate checking is
enabled.

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

The normalized trace magnitude of the Toffoli matrix is $0.75$, so its
projective Hilbert--Schmidt distance from the identity is
$\sqrt{1 - 0.75^2} = \sqrt{7}/4 \approx 0.6614$. A threshold of $0.7$ therefore
accepts the pair:

```{code-cell} ipython3
from mqt.qcec import verify
from mqt.qcec.pyqcec import Configuration

config = Configuration()
config.functionality.check_approximate_equivalence = True
config.functionality.approximate_checking_threshold = 0.7

verify(qc_lhs, qc_rhs, configuration=config)
```

A threshold below $\sqrt{7}/4$ rejects the same pair:

```{code-cell} ipython3
config.functionality.approximate_checking_threshold = 0.6
verify(qc_lhs, qc_rhs, configuration=config)
```

This is the unitary distance used by
[BQSKit](https://bqskit.readthedocs.io/en/latest/intro/synthesis.html). The
approach is also inspired by work on
[approximate equivalence checking](https://arxiv.org/abs/2103.11595) and
[approximate quantum-circuit synthesis](https://doi.org/10.1145/3503222.3507739).
