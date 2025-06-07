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

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(MQT_QCEC_MODULE_NAME, m, py::mod_gil_not_used()) {
  py::enum_<ec::StateType>(m, "StateType")
      .value("computational_basis", ec::StateType::ComputationalBasis)
      .value("random_1Q_basis", ec::StateType::Random1QBasis)
      .value("stabilizer", ec::StateType::Stabilizer)
      // allow construction from a string
      .def(py::init([](const std::string& str) -> ec::StateType {
             return ec::stateTypeFromString(str);
           }),
           "state_type"_a)
      // provide a string representation of the enum
      .def(
          "__str__", [](const ec::StateType type) { return toString(type); },
          py::prepend());
  // allow implicit conversion from string to StateType
  py::implicitly_convertible<std::string, ec::StateType>();
}
