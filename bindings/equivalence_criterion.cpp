/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "EquivalenceCriterion.hpp"

#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h> // NOLINT(misc-include-cleaner)
#include <string>

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(MQT_QCEC_MODULE_NAME, m, py::mod_gil_not_used()) {
  py::enum_<ec::EquivalenceCriterion>(m, "EquivalenceCriterion")
      .value("no_information", ec::EquivalenceCriterion::NoInformation)
      .value("not_equivalent", ec::EquivalenceCriterion::NotEquivalent)
      .value("equivalent", ec::EquivalenceCriterion::Equivalent)
      .value("equivalent_up_to_phase",
             ec::EquivalenceCriterion::EquivalentUpToPhase)
      .value("equivalent_up_to_global_phase",
             ec::EquivalenceCriterion::EquivalentUpToGlobalPhase)
      .value("probably_equivalent",
             ec::EquivalenceCriterion::ProbablyEquivalent)
      .value("probably_not_equivalent",
             ec::EquivalenceCriterion::ProbablyNotEquivalent)
      // allow construction from a string
      .def(py::init([](const std::string& str) -> ec::EquivalenceCriterion {
             return ec::fromString(str);
           }),
           "criterion"_a)
      // provide a string representation of the enum
      .def(
          "__str__",
          [](const ec::EquivalenceCriterion crit) { return toString(crit); },
          py::prepend());
  // allow implicit conversion from string to EquivalenceCriterion
  py::implicitly_convertible<std::string, ec::EquivalenceCriterion>();
}
