# MQT QCEC - A tool for Quantum Circuit Equivalence Checking

```{raw} latex
\begin{abstract}
```

MQT QCEC is an open-source C++20 and Python library for {doc}`quantum circuit equivalence checking <equivalence_checking>` developed as part of the _{doc}`Munich Quantum Toolkit (MQT) <mqt:index>`_ [^1].

This documentation provides a comprehensive guide to the MQT QCEC library, including {doc}`installation instructions <installation>`, a {doc}`quickstart guide <quickstart>`, and detailed {doc}`API documentation <api/mqt/qcec/index>`.
The source code of MQT QCEC is publicly available on GitHub at [munich-quantum-toolkit/qcec](https://github.com/munich-quantum-toolkit/qcec), while pre-built binaries are available via [PyPI](https://pypi.org/project/mqt.qcec/) for all major operating systems and all modern Python versions.
MQT QCEC is fully compatible with Qiskit 1.0 and above.

[^1]:
    The _[Munich Quantum Toolkit (MQT)](https://mqt.readthedocs.io)_ is a collection of software tools for quantum computing developed by the [Chair for Design Automation](https://www.cda.cit.tum.de/) at the [Technical University of Munich](https://www.tum.de/) as well as the [Munich Quantum Software Company (MQSC)](https://munichquantum.software).
    Among others, it is part of the [Munich Quantum Software Stack (MQSS)](https://www.munich-quantum-valley.de/research/research-areas/mqss) ecosystem, which is being developed as part of the [Munich Quantum Valley (MQV)](https://www.munich-quantum-valley.de) initiative.

````{only} latex
```{note}
A live version of this document is available at [mqt.readthedocs.io/projects/qcec](https://mqt.readthedocs.io/projects/qcec).
```
````

```{raw} latex
\end{abstract}

\sphinxtableofcontents
```

```{toctree}
:hidden:

self
```

```{toctree}
:maxdepth: 2
:caption: User Guide

installation
quickstart
equivalence_checking
compilation_flow_verification
parametrized_circuits
partial_equivalence
references
CHANGELOG
UPGRADING
```

````{only} not latex
```{toctree}
:maxdepth: 2
:titlesonly:
:caption: Developers
:glob:

contributing
support
development_guide
```
````

```{toctree}
:hidden:
:maxdepth: 6
:caption: API Reference

api/mqt/qcec/index
```
