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

#include <pybind11/native_enum.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h> // NOLINT(misc-include-cleaner)

namespace ec {

namespace py = pybind11;
using namespace pybind11::literals;

// NOLINTNEXTLINE(misc-use-internal-linkage)
void registerEquivalenceCriterion(const py::module& mod) {
  py::native_enum<EquivalenceCriterion>(
      mod, "EquivalenceCriterion", "enum.Enum",
      "Enumeration of notions of equivalence.")
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
      .finalize();
}

} // namespace ec
