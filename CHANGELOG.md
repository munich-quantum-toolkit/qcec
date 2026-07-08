<!-- Entries in each category are sorted by merge time, with the latest PRs appearing first. -->

# Changelog

All notable changes to this project will be documented in this file.

The format is based on a mixture of [Keep a Changelog] and [Common Changelog].
This project adheres to [Semantic Versioning], with the exception that minor releases may include breaking changes.

## [Unreleased]

### Added

- 🚸 Add [CMake presets] to provide a standardized and reproducible way to configure builds ([#936]) ([**@denialhaag**])

### Changed

- ⬆️ Update `mqt-core` to version 3.7.0 ([#976]) ([**@denialhaag**])
- ⬆️ Update `nanobind` to version 2.13.0 ([#976]) ([**@denialhaag**])
- ⬆️ Update [munich-quantum-toolkit/workflows] to version `v2.0.1` ([#936]) ([**@denialhaag**])

## [3.6.1] - 2026-05-21

### Fixed

- 🐛 Fix segfaults on permutation mismatches in the ZX checker ([#929]) ([**@denialhaag**])

## [3.6.0] - 2026-05-13

_If you are upgrading: please see [`UPGRADING.md`](UPGRADING.md#360)._

### Added

- ✨ Add support for many multi-controlled gates to the ZX-calculus checker ([#907]) ([**@burgholzer**])

### Changed

- ⬆️ Update `mqt-core` to version 3.6.0 ([#907]) ([**@burgholzer**])
- ⬆️ Update `nanobind` to version 2.12.0 ([#907]) ([**@burgholzer**])

## [3.5.0] - 2026-02-02

_If you are upgrading: please see [`UPGRADING.md`](UPGRADING.md#350)._

### Changed

- ⬆️ Update `mqt-core` to version 3.4.1 ([#837]) ([**@denialhaag**])
- ⬆️ Update `nanobind` to version 2.11.0 ([#837]) ([**@denialhaag**])
- ♻️ Auto-generate `pyqcec.pyi` via `nanobind.stubgen` ([#831]) ([**@denialhaag**])
- 🔧 Replace `mypy` with `ty` ([#775], [#825]) ([**@denialhaag**])

## [3.4.0] - 2026-01-13

_If you are upgrading: please see [`UPGRADING.md`](UPGRADING.md#340)._

### Changed

- ♻️ Migrate Python bindings from `pybind11` to `nanobind` ([#817]) ([**@denialhaag**])
- 📦️ Provide Stable ABI wheels for Python 3.12+ ([#817]) ([**@denialhaag**])
- ⬆️ Bump minimum required `mqt-core` version to `3.4.0` ([#817]) ([**@denialhaag**])
- 👷 Stop testing on `ubuntu-22.04` and `ubuntu-22.04-arm` runners ([#796]) ([**@denialhaag**])
- 👷 Stop testing with `clang-19` and start testing with `clang-21` ([#796]) ([**@denialhaag**])
- 👷 Fix macOS tests with Homebrew Clang via new `munich-quantum-toolkit/workflows` version ([#796]) ([**@denialhaag**])
- 👷 Re-enable macOS tests with GCC by disabling module scanning ([#796]) ([**@denialhaag**])

### Removed

- 🔥 Remove wheel builds for Python 3.13t ([#796]) ([**@denialhaag**])

## [3.3.0] - 2025-10-14

_If you are upgrading: please see [`UPGRADING.md`](UPGRADING.md#330)._

### Added

- 👷 Enable testing on Python 3.14 ([#730]) ([**@denialhaag**])

### Changed

- ⬆️ Bump minimum required `mqt-core` version to `3.3.1` ([#735]) ([**@denialhaag**])

### Removed

- 🔥 Drop support for Python 3.9 ([#704]) ([**@denialhaag**])

## [3.2.0] - 2025-08-01

_If you are upgrading: please see [`UPGRADING.md`](UPGRADING.md#320)._

### Added

- 🐍 Build Python 3.14 wheels ([#665]) ([**@denialhaag**])

### Changed

- ⬆️ Bump minimum required `mqt-core` version to `3.2.1` ([#668]) ([**@denialhaag**])
- ⬆️ Bump minimum required `mqt-core` version to `3.2.0` ([#667]) ([**@denialhaag**])
- ⬆️ Require C++20 ([#667]) ([**@denialhaag**])
- ✨ Expose enums to Python via `pybind11`'s new (`enum.Enum`-compatible) `py::native_enum` ([#663]) ([**@denialhaag**])

### Fixed

- 🚸 Increase binary compatibility between `mqt-qcec` and `mqt-core` ([#662]) ([**@denialhaag**])

## [3.1.0] - 2025-07-11

_If you are upgrading: please see [`UPGRADING.md`](UPGRADING.md#310)._

### Changed

- ⬆️ Bump minimum required `mqt-core` version to `3.1.0` ([#646]) ([**@denialhaag**])
- ⬆️ Bump minimum required `pybind11` version to `3.0.0` ([#646]) ([**@denialhaag**])
- ♻️ Move the C++ code for the Python bindings to the top-level `bindings` directory ([#618]) ([**@denialhaag**])
- ♻️ Move all Python code (no tests) to the top-level `python` directory ([#618]) ([**@denialhaag**])
- 💥 ZX-calculus checker now reports that it can't handle circuits with non-garbage ancilla qubits ([#512]) ([**@pehamTom**])

### Deprecated

- 🗑️ Deprecate the `mode` argument of `generate_profile()` and the `ancilla_mode` argument of `verify_compilation()` ([#626]) ([**@denialhaag**])

### Fixed

- 🐛 Fix bug in ZX-calculus checker for circuits without data qubits ([#512]) ([**@pehamTom**])

## [3.0.0] - 2025-05-05

_If you are upgrading: please see [`UPGRADING.md`](UPGRADING.md#300)._

### Added

- ✨ Support Qiskit 2.0+ ([#571]) ([**@burgholzer**])

### Changed

- 🚚 Move MQT QCEC to the [munich-quantum-toolkit] GitHub organization
- ♻️ Use the `mqt-core` Python package for handling circuits ([#432]) ([**@burgholzer**])
- ♻️ Return counterexamples as decision diagrams instead of dense arrays ([#566]) ([**@burgholzer**])
- ♻️ Reduce and restructure public interface of the `EquivalenceCheckingManager` ([#566]) ([**@burgholzer**])
- ⬆️ Bump minimum required CMake version to `3.24.0` ([#582]) ([**@burgholzer**])
- 📝 Rework existing project documentation ([#566]) ([**@burgholzer**])

### Removed

- 🔥 Remove support for `.real`, `.qc`, `.tfc`, and `GRCS` files ([#582]) ([**@burgholzer**])
- 🔥 Remove several re-exports from the top-level `mqt-qcec` package ([#566]) ([**@burgholzer**])

## [2.8.2] - 2025-02-18

_📚 Refer to the [GitHub Release Notes] for previous changelogs._

<!-- Version links -->

[unreleased]: https://github.com/munich-quantum-toolkit/qcec/compare/v3.6.1...HEAD
[3.6.1]: https://github.com/munich-quantum-toolkit/qcec/releases/tag/v3.6.1
[3.6.0]: https://github.com/munich-quantum-toolkit/qcec/releases/tag/v3.6.0
[3.5.0]: https://github.com/munich-quantum-toolkit/qcec/releases/tag/v3.5.0
[3.4.0]: https://github.com/munich-quantum-toolkit/qcec/releases/tag/v3.4.0
[3.3.0]: https://github.com/munich-quantum-toolkit/qcec/releases/tag/v3.3.0
[3.2.0]: https://github.com/munich-quantum-toolkit/qcec/releases/tag/v3.2.0
[3.1.0]: https://github.com/munich-quantum-toolkit/qcec/releases/tag/v3.1.0
[3.0.0]: https://github.com/munich-quantum-toolkit/qcec/releases/tag/v3.0.0
[2.8.2]: https://github.com/munich-quantum-toolkit/qcec/releases/tag/v2.8.2

<!-- PR links -->

[#976]: https://github.com/munich-quantum-toolkit/qcec/pull/976
[#936]: https://github.com/munich-quantum-toolkit/qcec/pull/936
[#929]: https://github.com/munich-quantum-toolkit/qcec/pull/929
[#907]: https://github.com/munich-quantum-toolkit/qcec/pull/907
[#837]: https://github.com/munich-quantum-toolkit/qcec/pull/837
[#831]: https://github.com/munich-quantum-toolkit/qcec/pull/831
[#825]: https://github.com/munich-quantum-toolkit/qcec/pull/825
[#817]: https://github.com/munich-quantum-toolkit/qcec/pull/817
[#796]: https://github.com/munich-quantum-toolkit/qcec/pull/796
[#775]: https://github.com/munich-quantum-toolkit/qcec/pull/775
[#735]: https://github.com/munich-quantum-toolkit/qcec/pull/735
[#730]: https://github.com/munich-quantum-toolkit/qcec/pull/730
[#704]: https://github.com/munich-quantum-toolkit/qcec/pull/704
[#699]: https://github.com/munich-quantum-toolkit/qcec/pull/699
[#668]: https://github.com/munich-quantum-toolkit/qcec/pull/668
[#667]: https://github.com/munich-quantum-toolkit/qcec/pull/667
[#665]: https://github.com/munich-quantum-toolkit/qcec/pull/663
[#663]: https://github.com/munich-quantum-toolkit/qcec/pull/663
[#662]: https://github.com/munich-quantum-toolkit/qcec/pull/662
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
[CMake presets]: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html
[munich-quantum-toolkit/workflows]: https://github.com/munich-quantum-toolkit/workflows
