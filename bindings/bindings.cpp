/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h> // NOLINT(misc-include-cleaner)

namespace ec {

namespace py = pybind11;
using namespace pybind11::literals;

// forward declarations
void registerApplicationSchema(const py::module& mod);
void registerConfiguration(const py::module& mod);
void registerEquivalenceCheckingManager(const py::module& mod);
void registerEquivalenceCriterion(const py::module& mod);
void registerStateType(const py::module& mod);

// NOLINTNEXTLINE(misc-include-cleaner)
PYBIND11_MODULE(MQT_QCEC_MODULE_NAME, mod, py::mod_gil_not_used()) {
  registerApplicationSchema(mod);
  registerConfiguration(mod);
  registerEquivalenceCheckingManager(mod);
  registerEquivalenceCriterion(mod);
  registerStateType(mod);
}

} // namespace ec
