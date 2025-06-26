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

namespace ec {

namespace py = pybind11;
using namespace pybind11::literals;

// NOLINTNEXTLINE(misc-use-internal-linkage)
void registerEquivalenceCriterion(const py::module& mod) {
  py::enum_<EquivalenceCriterion>(mod, "EquivalenceCriterion")
      .value("no_information", EquivalenceCriterion::NoInformation)
      .value("not_equivalent", EquivalenceCriterion::NotEquivalent)
      .value("equivalent", EquivalenceCriterion::Equivalent)
      .value("equivalent_up_to_phase",
             EquivalenceCriterion::EquivalentUpToPhase)
      .value("equivalent_up_to_global_phase",
             EquivalenceCriterion::EquivalentUpToGlobalPhase)
      .value("probably_equivalent", EquivalenceCriterion::ProbablyEquivalent)
      .value("probably_not_equivalent",
             EquivalenceCriterion::ProbablyNotEquivalent)
      // allow construction from a string
      .def(py::init([](const std::string& str) -> EquivalenceCriterion {
             return fromString(str);
           }),
           "criterion"_a)
      // provide a string representation of the enum
      .def(
          "__str__",
          [](const EquivalenceCriterion crit) { return toString(crit); },
          py::prepend());
  // allow implicit conversion from string to EquivalenceCriterion
  py::implicitly_convertible<std::string, EquivalenceCriterion>();
}

} // namespace ec
