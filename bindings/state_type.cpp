/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "checker/dd/simulation/StateType.hpp"

#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h> // NOLINT(misc-include-cleaner)
#include <string>

namespace ec {

namespace py = pybind11;
using namespace pybind11::literals;

// NOLINTNEXTLINE(misc-use-internal-linkage)
void registerStateType(const py::module& mod) {
  py::enum_<StateType>(mod, "StateType")
      .value("computational_basis", StateType::ComputationalBasis)
      .value("random_1Q_basis", StateType::Random1QBasis)
      .value("stabilizer", StateType::Stabilizer)
      // allow construction from a string
      .def(py::init([](const std::string& str) -> StateType {
             return stateTypeFromString(str);
           }),
           "state_type"_a)
      // provide a string representation of the enum
      .def(
          "__str__", [](const StateType type) { return toString(type); },
          py::prepend());
  // allow implicit conversion from string to StateType
  py::implicitly_convertible<std::string, StateType>();
}

} // namespace ec
