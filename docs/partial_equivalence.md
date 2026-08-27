---
file_format: mystnb
kernelspec:
  name: python3
mystnb:
  number_source_lines: true
---

# Partial Equivalence Checking

## Partial Equivalence vs. Total Equivalence

Different definitions of quantum circuit equivalence have been proposed, with
the most commonly used being total equivalence. Two circuits are considered
totally equivalent when their matrix representations are exactly the same.
However, it is often sufficient to consider observational equivalence, because
the only way to extract information from a quantum circuit is through
measurement. Therefore, partial equivalence has been defined, which is a weaker
equality than total equivalence. Two circuits are considered partially
equivalent if, for each possible input state, they have the same probabilities
for each measurement outcome. In particular, partial equivalence doesn't
consider qubits that are not measured or different phase angles for each
measurement.

This definition is especially useful for circuits that do not measure all of
their qubits at the end, or where some qubits are initialized to a certain value
at the beginning. We refer to the qubits that are initialized to a certain value
at the beginning of the computation as _ancillary_ qubits. Without loss of
generality, we assume that these qubits are initially set to $|0\rangle$. Any
other initial state can be reached from the $|0\rangle$ state by applying the
appropriate gates. The remaining qubits are the qubits that hold the input
state, and they are referred to as _data_ qubits.

Similarly, we differentiate between _measured_ and _garbage_ qubits. In order to
read the output at the end of a circuit, a measurement is performed on some
qubits, but not necessarily all of them. These qubits are the _measured_ qubits.
All qubits that are not measured are called the _garbage_ qubits. Their final
state doesn't influence the output we obtain from the computation.

For a circuit $C$ we denote $P(t | \psi, C)$ as the probability that the quantum
circuit $C$ collapses to state $|t\rangle$ upon a measurement on the measured
qubits, given that the data qubits have the initial state $|\psi\rangle$. Two
circuits $C_1$ and $C_2$ are considered partially equivalent if, for each
initial state $|\psi\rangle$ of the data qubits and each final state $|t\rangle$
of the measured qubits, it holds that $P(t|\psi, C_1) = P(t|\psi, C_2)$.

## Checking Partial Equivalence of Quantum Circuits

Partial equivalence checking is a bit more costly than regular equivalence
checking. It is necessary to reduce the contribution of garbage qubits and
normalize the phase of each measurement outcome. Therefore, it is not enabled by
default in QCEC. It can be activated using an option in the configuration
parameters. If the option `check_partial_equivalence` is set to `True`, the
equivalence checker will return `equivalent` not only for totally equivalent
circuits, but also for partially equivalent circuits. The result will be
`not_equivalent` if and only if the circuits are not partially equivalent. On
the other hand, if the option `check_partial_equivalence` is set to `False`,
then the equivalence checker considers total equivalence modulo ancillary
qubits. This means that the garbage qubits are considered as if they were
measured qubits, while ancillary qubits are set to zero, as in the partial
equivalence check. The equivalence checker will return `equivalent` for totally
equivalent circuits or `equivalent_up_to_global_phase` for circuits which differ
only in their global phase and `not_equivalent` for other circuits.

The following is a summary of the behaviour of each type of equivalence checker
when the `check_partial_equivalence` option is set to `True`.

1. **Construction Equivalence Checker:** The construction checker supports
   partial equivalence checking by using decision diagrams to calculate the
   normalized phases of the possible outputs and then summing up the
   contribution of garbage qubits, in order to consider only the measurement
   probabilities of measured qubits. Then it compares the reduced decision
   diagrams of the two circuits.

1. **Alternating Equivalence Checker:** The alternating checker can only be used
   for circuits where at least one of the two does not contain ancillary qubits.
   It computes the composition of one circuit with the inverse of the other and
   verifies that the result resembles the identity. When checking for partial
   equivalence, it suffices to verify that the result resembles the identity
   modulo garbage qubits.

1. **Simulation Equivalence Checker:** The simulation checker computes a
   representation of the state vector resulting from simulating the circuit with
   certain initial states. Partial equivalence is enabled by summing up the
   contributions of garbage qubits.

1. **ZX-Calculus Equivalence Checker:** The ZX-calculus checker doesn't directly
   support partial equivalence, which is not a problem for the equivalence
   checking workflow, given that the ZX-calculus checker cannot demonstrate
   non-equivalence of circuits due to its incompleteness. Therefore it will
   simply output 'No Information' for circuits that are partially but not
   totally equivalent.

+++

## Using QCEC to Check for Partial Equivalence

Consider the following quantum circuit with three qubits, which are all data
qubits, but only one of them is measured.

```{code-cell} ipython3
from qiskit import QuantumCircuit

qc_lhs = QuantumCircuit(3, 1)
qc_lhs.cswap(1, 0, 2)
qc_lhs.h(0)
qc_lhs.z(2)
qc_lhs.cswap(1, 0, 2)
qc_lhs.measure(0, 0)

qc_lhs.draw(output="mpl", style="iqp")
```

Additionally, consider the following circuit, which only acts on two qubits.

```{code-cell} ipython3
qc_rhs = QuantumCircuit(3, 1)
qc_rhs.x(1)
qc_rhs.ch(1, 0)
qc_rhs.measure(0, 0)
qc_rhs.draw(output="mpl", style="iqp")
```

Then, these circuits are not totally equivalent, as can be shown using QCEC. The
library even emits a handy warning in this case that indicates that two circuits
have been shown to be non-equivalent, but at least one of the circuits does not
measure all its qubits (i.e., has garbage qubits) and partial equivalence
checking support has not been enabled.

```{code-cell} ipython3
from mqt.qcec import verify

verify(qc_lhs, qc_rhs)
```

However, both circuits are partially equivalent, because they have the same
probability distribution of measured values for any given initial input state.
This equality can be proven with QCEC by enabling the
`check_partial_equivalence` option:

```{code-cell} ipython3
verify(qc_lhs, qc_rhs, check_partial_equivalence=True)
```

Source: The definitions and example for partial equivalence are described in
{cite:p}`chen2022PartialEquivalenceChecking`.
