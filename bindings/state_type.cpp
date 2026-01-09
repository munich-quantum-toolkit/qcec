/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "checker/dd/simulation/StateType.hpp"

#include <pybind11/native_enum.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h> // NOLINT(misc-include-cleaner)

namespace ec {

namespace py = pybind11;
using namespace pybind11::literals;

// NOLINTNEXTLINE(misc-use-internal-linkage)
void registerStateType(const py::module& mod) {
  py::native_enum<StateType>(
      mod, "StateType", "enum.Enum",
      "Enumeration of state types for the simulation checker.")
      .value("computational_basis", StateType::ComputationalBasis)
      .value("classical", StateType::ComputationalBasis)
      .value("random_1Q_basis", StateType::Random1QBasis)
      .value("local_quantum", StateType::Random1QBasis)
      .value("stabilizer", StateType::Stabilizer)
      .value("global_quantum", StateType::Stabilizer)
      .finalize();
}

} // namespace ec
