```{raw} latex
\begingroup
\renewcommand\section[1]{\endgroup}
\phantomsection
```

````{only} html
# References

*MQT QCEC* is academic software. Thus, many of its built-in algorithms have been published as scientific papers.
If you use *MQT QCEC* in your work, we would appreciate if you cited {cite:p}`burgholzer2021advanced` (which subsumes {cite:p}`burgholzer2020ImprovedDDbasedEquivalence` and {cite:p}`burgholzer2020PowerSimulationEquivalence`).

Furthermore, if you use any of the particular algorithms such as

- the compilation flow result verification scheme {cite:p}`burgholzer2020verifyingResultsIBM`,
- the dedicated stimuli generation schemes {cite:p}`burgholzer2021randomStimuliGenerationQuantum`,
- the transformation scheme for circuits containing non-unitaries {cite:p}`burgholzer2022handlingNonUnitaries`,
- the equivalence checker based on ZX-diagrams {cite:p}`peham2022equivalenceCheckingZXCalculus`, or
- the method for checking equivalence of parameterized circuits {cite:p}`peham2023EquivalenceCheckingParameterizedCircuits`

please consider citing their respective papers as well.

*MQT QCEC* is part of the Munich Quantum Toolkit, which is described in {cite:p}`mqt`.
If you want to cite the Munich Quantum Toolkit, please use the following BibTeX entry:

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

A full list of references is given below.
````

```{bibliography}

```
