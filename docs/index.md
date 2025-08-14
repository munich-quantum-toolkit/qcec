# MQT QCEC - A tool for Quantum Circuit Equivalence Checking

```{raw} latex
\begin{abstract}
```

MQT QCEC is an open-source C++20 and Python library for {doc}`quantum circuit equivalence checking <equivalence_checking>` developed as part of the _{doc}`Munich Quantum Toolkit (MQT) <mqt:index>`_.

This documentation provides a comprehensive guide to the MQT QCEC library, including {doc}`installation instructions <installation>`, a {doc}`quickstart guide <quickstart>`, and detailed {doc}`API documentation <api/mqt/qcec/index>`.
The source code of MQT QCEC is publicly available on GitHub at [munich-quantum-toolkit/qcec](https://github.com/munich-quantum-toolkit/qcec), while pre-built binaries are available via [PyPI](https://pypi.org/project/mqt.qcec/) for all major operating systems and all modern Python versions.
MQT QCEC is fully compatible with Qiskit 1.0 and above.

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

```{only} html
## Contributors and Supporters

The _[Munich Quantum Toolkit (MQT)](https://mqt.readthedocs.io)_ is developed by the [Chair for Design Automation](https://www.cda.cit.tum.de/) at the [Technical University of Munich](https://www.tum.de/) and supported by the [Munich Quantum Software Company (MQSC)](https://munichquantum.software).
Among others, it is part of the [Munich Quantum Software Stack (MQSS)](https://www.munich-quantum-valley.de/research/research-areas/mqss) ecosystem, which is being developed as part of the [Munich Quantum Valley (MQV)](https://www.munich-quantum-valley.de) initiative.

<div style="margin-top: 0.5em">
<div class="only-light" align="center">
  <img src="https://raw.githubusercontent.com/munich-quantum-toolkit/.github/refs/heads/main/docs/_static/mqt-logo-banner-light.svg" width="90%" alt="MQT Banner">
</div>
<div class="only-dark" align="center">
  <img src="https://raw.githubusercontent.com/munich-quantum-toolkit/.github/refs/heads/main/docs/_static/mqt-logo-banner-dark.svg" width="90%" alt="MQT Banner">
</div>
</div>

Thank you to all the contributors who have helped make MQT QCEC a reality!

<p align="center">
<a href="https://github.com/munich-quantum-toolkit/qcec/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=munich-quantum-toolkit/qcec" />
</a>
</p>

The MQT will remain free, open-source, and permissively licensednow—and in the future.
We are firmly committed to keeping it open and actively maintained for the quantum computing community.

To support this endeavor, please consider:

- [Sponsoring us on GitHub](https://github.com/sponsors/munich-quantum-toolkit)
- [Starring and sharing our repositories](https://github.com/munich-quantum-toolkit)
- Contributing code, documentation, tests, or examples via issues and pull requests
- Citing the MQT in your publications (see [Cite This](#cite-this))
- Using the MQT in research and teaching, and sharing feedback and use cases

<p align="center">
<iframe src="https://github.com/sponsors/munich-quantum-toolkit/button" title="Sponsor munich-quantum-toolkit" height="32" width="114" style="border: 0; border-radius: 6px;"></iframe>
</p>

## Cite This

If you want to cite MQT QCEC, please use the following BibTeX entry:

TODO
```
