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
#include "dd/RealNumber.hpp"
#include "ir/QuantumComputation.hpp"

#include <algorithm>
#include <exception>
#include <memory>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11_json/pybind11_json.hpp>
#include <stdexcept>
#include <string>
#include <thread>

namespace py = pybind11;
using namespace pybind11::literals;

namespace ec {

PYBIND11_MODULE(MQT_QCEC_MODULE_NAME, m, py::mod_gil_not_used()) {
  py::enum_<StateType>(m, "StateType")
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
