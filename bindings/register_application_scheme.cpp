/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "checker/dd/applicationscheme/ApplicationScheme.hpp"

#include <nanobind/nanobind.h>

namespace ec {

namespace nb = nanobind;
using namespace nb::literals;

// NOLINTNEXTLINE(misc-use-internal-linkage)
void registerApplicationSchema(const nb::module_& m) {
  nb::enum_<ApplicationSchemeType>(
      m, "ApplicationScheme", nb::is_arithmetic(),
      R"pb(Describes the order in which the individual operations of both circuits are applied during the equivalence check.

In case of the alternating equivalence checker, this is the key component to allow the intermediate decision diagrams to remain close to the identity (as proposed in :cite:p:`burgholzer2021advanced`).
See :doc:`/compilation_flow_verification` for more information on the dedicated application scheme for verifying compilation flow results (as proposed in :cite:p:`burgholzer2020verifyingResultsIBM`).

In case of the other checkers, which consider both circuits individually, using a non-sequential application scheme can significantly boost the operation caching performance in the underlying decision diagram package.)pb")

      .value(
          "sequential", ApplicationSchemeType::Sequential,
          R"pb(Applies all gates from the first circuit, before proceeding with the second circuit.

    Referred to as *"reference"* in :cite:p:`burgholzer2021advanced`.)pb")

      .value("reference", ApplicationSchemeType::Sequential,
             R"pb(Alias for :attr:`~ApplicationScheme.sequential`.)pb")

      .value(
          "one_to_one", ApplicationSchemeType::OneToOne,
          R"pb(Alternates between applications from the first and the second circuit.

Referred to as *"naive"* in :cite:p:`burgholzer2021advanced`.)pb")

      .value("naive", ApplicationSchemeType::OneToOne,
             R"pb(Alias for :attr:`~ApplicationScheme.one_to_one`.)pb")

      .value(
          "lookahead", ApplicationSchemeType::Lookahead,
          R"pb(Looks whether an application from the first circuit or the second circuit yields the smaller decision diagram.

Only works for the alternating equivalence checker.)pb")

      .value(
          "gate_cost", ApplicationSchemeType::GateCost,
          R"pb(Each gate of the first circuit is associated with a corresponding cost according to some cost function *f(...)*.
Whenever a gate *g* from the first circuit is applied *f(g)* gates are applied from the second circuit.

Referred to as *"compilation_flow"* in :cite:p:`burgholzer2020verifyingResultsIBM`.)pb")

      .value("compilation_flow", ApplicationSchemeType::GateCost,
             R"pb(Alias for :attr:`~ApplicationScheme.gate_cost`.)pb")

      .value(
          "proportional", ApplicationSchemeType::Proportional,
          R"pb(Alternates between applications from the first and the second circuit, but applies the gates in proportion to the number of gates in each circuit.)pb");
}

} // namespace ec
