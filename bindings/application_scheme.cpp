/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "checker/dd/applicationscheme/ApplicationScheme.hpp"

#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h> // NOLINT(misc-include-cleaner)
#include <string>

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(MQT_QCEC_MODULE_NAME, m, py::mod_gil_not_used()) {
  py::enum_<ec::ApplicationSchemeType>(m, "ApplicationScheme")
      .value("sequential", ec::ApplicationSchemeType::Sequential)
      .value("one_to_one", ec::ApplicationSchemeType::OneToOne)
      .value("proportional", ec::ApplicationSchemeType::Proportional)
      .value("lookahead", ec::ApplicationSchemeType::Lookahead)
      .value(
          "gate_cost", ec::ApplicationSchemeType::GateCost,
          "Each gate of the first circuit is associated with a corresponding "
          "cost according to some cost function *f(...)*. Whenever a gate *g* "
          "from the first circuit is applied *f(g)* gates are applied from the "
          "second circuit. Referred to as *compilation_flow* in "
          ":cite:p:`burgholzer2020verifyingResultsIBM`.")
      // allow construction from a string
      .def(py::init([](const std::string& str) -> ec::ApplicationSchemeType {
             return ec::applicationSchemeFromString(str);
           }),
           "scheme"_a)
      // provide a string representation of the enum
      .def(
          "__str__",
          [](const ec::ApplicationSchemeType scheme) {
            return toString(scheme);
          },
          py::prepend());
  // allow implicit conversion from string to ApplicationSchemeType
  py::implicitly_convertible<std::string, ec::ApplicationSchemeType>();
}
