---
file_format: mystnb
kernelspec:
  name: python3
mystnb:
  number_source_lines: true
---

# Quickstart

Assume you want to prove that the following two circuits are equivalent:

```{code-cell} ipython3
from qiskit import QuantumCircuit

qc1 = QuantumCircuit(2)
qc1.cx(0, 1)
qc1.draw(output="mpl", style="iqp")
```

```{code-cell} ipython3
from qiskit import QuantumCircuit

qc2 = QuantumCircuit(2)
qc2.h(0)
qc2.h(1)
qc2.cx(1, 0)
qc2.h(1)
qc2.h(0)
qc2.draw(output="mpl", style="iqp")
```

Then, using QCEC to check the equivalence of these two circuits is as easy as

```{code-cell} ipython3
from mqt import qcec

qcec.verify(qc1, qc2)
```

Check out the {py:func}`reference documentation <.verify>` for more information.
