<!-- Entries in each category are sorted by merge time, with the latest PRs appearing first. -->

# Changelog

All notable changes to this project will be documented in this file.

The format is based on a mixture of [Keep a Changelog] and [Common Changelog].
This project adheres to [Semantic Versioning], with the exception that minor releases may include breaking changes.

## [Unreleased]

## [3.1.0] - 2025-07-11

_If you are upgrading: please see [`UPGRADING.md`](UPGRADING.md#310)._

### Changed

- **Breaking**: ⬆️ Bump minimum required MQT Core version to `3.1.0` ([#646]) ([**@denialhaag**])
- **Breaking**: ⬆️ Bump minimum required `pybind11` version to `3.0.0` ([#646]) ([**@denialhaag**])
- ♻️ Move the C++ code for the Python bindings to the top-level `bindings` directory ([#618]) ([**@denialhaag**])
- ♻️ Move all Python code (no tests) to the top-level `python` directory ([#618]) ([**@denialhaag**])
- **Breaking**: 💥 ZX-calculus checker now reports that it can't handle circuits with non-garbage ancilla qubits ([#512]) ([**@pehamTom**])

### Deprecated

- 🗑️ Deprecate the `mode` argument of `generate_profile()` and the `ancilla_mode` argument of `verify_compilation()` ([#626]) ([**@denialhaag**])

### Fixed

- 🐛 Fix bug in ZX-calculus checker for circuits without data qubits ([#512]) ([**@pehamTom**])

## [3.0.0] - 2025-05-05

_If you are upgrading: please see [`UPGRADING.md`](UPGRADING.md#300)._

### Added

- ✨ Support Qiskit 2.0+ ([#571]) ([**@burgholzer**])

### Changed

- **Breaking**: 🚚 Move MQT QCEC to the [munich-quantum-toolkit] GitHub organization
- **Breaking**: ♻️ Use the `mqt-core` Python package for handling circuits ([#432]) ([**@burgholzer**])
- **Breaking**: ♻️ Return counterexamples as decision diagrams instead of dense arrays ([#566]) ([**@burgholzer**])
- **Breaking**: ♻️ Reduce and restructure public interface of the `EquivalenceCheckingManager` ([#566]) ([**@burgholzer**])
- **Breaking**: ⬆️ Bump minimum required CMake version to `3.24.0` ([#582]) ([**@burgholzer**])
- 📝 Rework existing project documentation ([#566]) ([**@burgholzer**])

### Removed

- **Breaking**: 🔥 Remove support for `.real`, `.qc`, `.tfc`, and `GRCS` files ([#582]) ([**@burgholzer**])
- **Breaking**: 🔥 Remove several re-exports from the top-level `mqt-qcec` package ([#566]) ([**@burgholzer**])

## [2.8.2] - 2025-02-18

_📚 Refer to the [GitHub Release Notes] for previous changelogs._

<!-- Version links -->

[unreleased]: https://github.com/munich-quantum-toolkit/qcec/compare/v3.1.0...HEAD
[3.1.0]: https://github.com/munich-quantum-toolkit/qcec/compare/v3.0.0...v3.1.0
[3.0.0]: https://github.com/munich-quantum-toolkit/qcec/compare/v2.8.2...v3.0.0
[2.8.2]: https://github.com/munich-quantum-toolkit/qcec/releases/tag/v2.8.2

<!-- PR links -->

[#646]: https://github.com/munich-quantum-toolkit/qcec/pull/646
[#626]: https://github.com/munich-quantum-toolkit/qcec/pull/626
[#618]: https://github.com/munich-quantum-toolkit/qcec/pull/618
[#582]: https://github.com/munich-quantum-toolkit/qcec/pull/582
[#571]: https://github.com/munich-quantum-toolkit/qcec/pull/571
[#566]: https://github.com/munich-quantum-toolkit/qcec/pull/566
[#512]: https://github.com/munich-quantum-toolkit/qcec/pull/512
[#432]: https://github.com/munich-quantum-toolkit/qcec/pull/432

<!-- Contributor -->

[**@burgholzer**]: https://github.com/burgholzer
[**@pehamTom**]: https://github.com/pehamTom
[**@denialhaag**]: https://github.com/denialhaag

<!-- General links -->

[Keep a Changelog]: https://keepachangelog.com/en/1.1.0/
[Common Changelog]: https://common-changelog.org
[Semantic Versioning]: https://semver.org/spec/v2.0.0.html
[GitHub Release Notes]: https://github.com/munich-quantum-toolkit/qcec/releases
[munich-quantum-toolkit]: https://github.com/munich-quantum-toolkit
