/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "EquivalenceCriterion.hpp"

#include <nanobind/nanobind.h>

namespace ec {

namespace nb = nanobind;
using namespace nb::literals;

// NOLINTNEXTLINE(misc-use-internal-linkage)
void registerEquivalenceCriterion(const nb::module_& m) {
  nb::enum_<EquivalenceCriterion>(m, "EquivalenceCriterion",
                                  nb::is_arithmetic())
      .value("no_information", EquivalenceCriterion::NoInformation)
      .value("not_equivalent", EquivalenceCriterion::NotEquivalent)
      .value("equivalent", EquivalenceCriterion::Equivalent)
      .value("equivalent_up_to_phase",
             EquivalenceCriterion::EquivalentUpToPhase)
      .value("equivalent_up_to_global_phase",
             EquivalenceCriterion::EquivalentUpToGlobalPhase)
      .value("probably_equivalent", EquivalenceCriterion::ProbablyEquivalent)
      .value("probably_not_equivalent",
             EquivalenceCriterion::ProbablyNotEquivalent);
}

} // namespace ec
