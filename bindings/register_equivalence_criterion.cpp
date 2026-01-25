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
  nb::enum_<EquivalenceCriterion>(
      m, "EquivalenceCriterion", nb::is_arithmetic(),
      R"pb(Captures all the different notions of equivalence that can be the result of a :meth:`~.EquivalenceCheckingManager.run`.)pb")

      .value("no_information", EquivalenceCriterion::NoInformation,
             R"pb(No information on the equivalence is available.

This can be because the check has not been run or that a timeout happened.)pb")

      .value("not_equivalent", EquivalenceCriterion::NotEquivalent,
             R"pb(Circuits are shown to be non-equivalent.)pb")

      .value("equivalent", EquivalenceCriterion::Equivalent,
             R"pb(Circuits are shown to be equivalent.)pb")

      .value("equivalent_up_to_phase",
             EquivalenceCriterion::EquivalentUpToPhase,
             R"pb(Circuits are equivalent up to a global phase factor.)pb")

      .value(
          "equivalent_up_to_global_phase",
          EquivalenceCriterion::EquivalentUpToGlobalPhase,
          R"pb(Circuits are equivalent up to a certain (global or relative) phase.)pb")

      .value("probably_equivalent", EquivalenceCriterion::ProbablyEquivalent,
             R"pb(Circuits are probably equivalent.

A result obtained whenever a couple of simulations did not show the non-equivalence in the simulation checker.)pb")

      .value("probably_not_equivalent",
             EquivalenceCriterion::ProbablyNotEquivalent,
             R"pb(Circuits are probably not equivalent.

A result obtained whenever the ZX-calculus checker could not reduce the combined circuit to the identity.)pb");
}

} // namespace ec
