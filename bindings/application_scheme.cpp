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

namespace ec {

namespace py = pybind11;
using namespace pybind11::literals;

// NOLINTNEXTLINE(misc-use-internal-linkage)
void registerApplicationSchema(const py::module& mod) {
  py::enum_<ApplicationSchemeType>(mod, "ApplicationScheme")
      .value("sequential", ApplicationSchemeType::Sequential)
      .value("one_to_one", ApplicationSchemeType::OneToOne)
      .value("proportional", ApplicationSchemeType::Proportional)
      .value("lookahead", ApplicationSchemeType::Lookahead)
      .value(
          "gate_cost", ApplicationSchemeType::GateCost,
          "Each gate of the first circuit is associated with a corresponding "
          "cost according to some cost function *f(...)*. Whenever a gate *g* "
          "from the first circuit is applied *f(g)* gates are applied from the "
          "second circuit. Referred to as *compilation_flow* in "
          ":cite:p:`burgholzer2020verifyingResultsIBM`.")
      // allow construction from a string
      .def(py::init([](const std::string& str) -> ApplicationSchemeType {
             return applicationSchemeFromString(str);
           }),
           "scheme"_a)
      // provide a string representation of the enum
      .def(
          "__str__",
          [](const ApplicationSchemeType scheme) { return toString(scheme); },
          py::prepend());
  // allow implicit conversion from string to ApplicationSchemeType
  py::implicitly_convertible<std::string, ApplicationSchemeType>();
}

} // namespace ec
