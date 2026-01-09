/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include <nanobind/nanobind.h>

namespace ec {

namespace nb = nanobind;
using namespace nb::literals;

// forward declarations
void registerApplicationSchema(const nb::module_& m);
void registerConfiguration(const nb::module_& m);
void registerEquivalenceCheckingManager(const nb::module_& m);
void registerEquivalenceCriterion(const nb::module_& m);
void registerStateType(const nb::module_& m);

// NOLINTNEXTLINE(misc-include-cleaner)
NB_MODULE(MQT_QCEC_MODULE_NAME, m) {
  registerApplicationSchema(m);
  registerConfiguration(m);
  registerEquivalenceCheckingManager(m);
  registerEquivalenceCriterion(m);
  registerStateType(m);
}

} // namespace ec
