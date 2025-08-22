[![PyPI](https://img.shields.io/pypi/v/mqt.qcec?logo=pypi&style=flat-square)](https://pypi.org/project/mqt.qcec/)
![OS](https://img.shields.io/badge/os-linux%20%7C%20macos%20%7C%20windows-blue?style=flat-square)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg?style=flat-square)](https://opensource.org/licenses/MIT)
[![CI](https://img.shields.io/github/actions/workflow/status/munich-quantum-toolkit/qcec/ci.yml?branch=main&style=flat-square&logo=github&label=ci)](https://github.com/munich-quantum-toolkit/qcec/actions/workflows/ci.yml)
[![CD](https://img.shields.io/github/actions/workflow/status/munich-quantum-toolkit/qcec/cd.yml?style=flat-square&logo=github&label=cd)](https://github.com/munich-quantum-toolkit/qcec/actions/workflows/cd.yml)
[![Documentation](https://img.shields.io/readthedocs/qcec?logo=readthedocs&style=flat-square)](https://mqt.readthedocs.io/projects/qcec)
[![codecov](https://img.shields.io/codecov/c/github/munich-quantum-toolkit/qcec?style=flat-square&logo=codecov)](https://codecov.io/gh/munich-quantum-toolkit/qcec)

<p align="center">
  <a href="https://mqt.readthedocs.io">
    <picture>
      <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/munich-quantum-toolkit/.github/refs/heads/main/docs/_static/mqt-banner-dark.svg" width="90%">
      <img src="https://raw.githubusercontent.com/munich-quantum-toolkit/.github/refs/heads/main/docs/_static/mqt-banner-light.svg" width="90%" alt="MQT Banner">
    </picture>
  </a>
</p>

# MQT QCEC - A tool for Quantum Circuit Equivalence Checking

A tool for quantum circuit equivalence checking developed as part of the [_Munich Quantum Toolkit (MQT)_](https://mqt.readthedocs.io).
It builds upon [MQT Core](https://github.com/munich-quantum-toolkit/core), which forms the backbone of the MQT.

<p align="center">
  <a href="https://mqt.readthedocs.io/projects/qcec">
  <img width=30% src="https://img.shields.io/badge/documentation-blue?style=for-the-badge&logo=read%20the%20docs" alt="Documentation" />
  </a>
</p>

## Key Features

- Point 1
- Point 2
- Point 3

If you have any questions, feel free to create a [discussion](https://github.com/munich-quantum-toolkit/qcec/discussions) or an [issue](https://github.com/munich-quantum-toolkit/qcec/issues) on [GitHub](https://github.com/munich-quantum-toolkit/qcec).

## Contributors and Supporters

The _[Munich Quantum Toolkit (MQT)](https://mqt.readthedocs.io)_ is developed by the [Chair for Design Automation](https://www.cda.cit.tum.de/) at the [Technical University of Munich](https://www.tum.de/) and supported by the [Munich Quantum Software Company (MQSC)](https://munichquantum.software).
Among others, it is part of the [Munich Quantum Software Stack (MQSS)](https://www.munich-quantum-valley.de/research/research-areas/mqss) ecosystem, which is being developed as part of the [Munich Quantum Valley (MQV)](https://www.munich-quantum-valley.de) initiative.

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/munich-quantum-toolkit/.github/refs/heads/main/docs/_static/mqt-logo-banner-dark.svg" width="90%">
    <img src="https://raw.githubusercontent.com/munich-quantum-toolkit/.github/refs/heads/main/docs/_static/mqt-logo-banner-light.svg" width="90%" alt="MQT Partner Logos">
  </picture>
</p>

Thank you to all the contributors who have helped make MQT QCEC a reality!

<p align="center">
  <a href="https://github.com/munich-quantum-toolkit/qcec/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=munich-quantum-toolkit/qcec" />
  </a>
</p>

The MQT will remain free, open-source, and permissively licensed—now and in the future.
We are firmly committed to keeping it open and actively maintained for the quantum computing community.

To support this endeavor, please consider:

- Starring and sharing our repositories: https://github.com/munich-quantum-toolkit
- Contributing code, documentation, tests, or examples via issues and pull requests
- Citing the MQT in your publications (see [Cite This](#cite-this))
- Citing our research in your publications (see [References](https://mqt.readthedocs.io/projects/qcec/en/latest/references.html))
- Using the MQT in research and teaching, and sharing feedback and use cases
- Sponsoring us on GitHub: https://github.com/sponsors/munich-quantum-toolkit

<p align="center">
  <a href="https://github.com/sponsors/munich-quantum-toolkit">
  <img width=20% src="https://img.shields.io/badge/Sponsor-white?style=for-the-badge&logo=githubsponsors&labelColor=black&color=blue" alt="Sponsor the MQT" />
  </a>
</p>

## Getting Started

MQT QCEC is available via [PyPI](https://pypi.org/project/mqt.qcec/) for Linux, macOS, and Windows and supports Python 3.9 to 3.14.

```console
(venv) $ pip install mqt.qcec
```

The following code gives an example on the usage:

```python3
from mqt import qcec

# verify the equivalence of two circuits provided as qasm files
result = qcec.verify("circ1.qasm", "circ2.qasm")

# print the result
print(result.equivalence)
```

**Detailed documentation on all available methods, options, and input formats is available at [ReadTheDocs](https://mqt.readthedocs.io/projects/qcec).**

## System Requirements and Building

The implementation is compatible with any C++20 compiler, a minimum CMake version of 3.24, and Python 3.9+.
Please refer to the [documentation](https://mqt.readthedocs.io/projects/qcec) on how to build the project.

Building (and running) is continuously tested under Linux, macOS, and Windows using the [latest available system versions for GitHub Actions](https://github.com/actions/virtual-environments).

## Cite This

When citing the software itself or results produced with it, please cite the MQT Handbook:

```bibtex
@inproceedings{mqt,
    title        = {The {{MQT}} Handbook: {{A}} Summary of Design Automation Tools and Software for Quantum Computing},
    shorttitle   = {{The MQT Handbook}},
    author       = {Robert Wille and Lucas Berent and Tobias Forster and Jagatheesan Kunasaikaran and Kevin Mato and Tom Peham and Nils Quetschlich and Damian Rovara and Aaron Sander and Ludwig Schmid and Daniel Schoenberger and Yannick Stade and Lukas Burgholzer},
    booktitle    = {IEEE International Conference on Quantum Software (QSW)},
    doi          = {10.1109/QSW62656.2024.00013},
    year         = 2024,
    eprint       = {2405.17543},
    eprinttype   = {arxiv},
    addendum     = {A live version of this document is available at \url{https://mqt.readthedocs.io}},
}
```

---

## Acknowledgements

The Munich Quantum Toolkit has been supported by the European Research Council (ERC) under the European Union's Horizon 2020 research and innovation program (grant agreement No. 101001318), the Bavarian State Ministry for Science and Arts through the Distinguished Professorship Program, as well as the Munich Quantum Valley, which is supported by the Bavarian state government with funds from the Hightech Agenda Bayern Plus.

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/munich-quantum-toolkit/.github/refs/heads/main/docs/_static/mqt-funding-footer-dark.svg" width="90%">
    <img src="https://raw.githubusercontent.com/munich-quantum-toolkit/.github/refs/heads/main/docs/_static/mqt-funding-footer-light.svg" width="90%" alt="MQT Funding Footer">
  </picture>
</p>
