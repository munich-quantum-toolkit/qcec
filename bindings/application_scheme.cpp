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
  nb::enum_<ApplicationSchemeType>(m, "ApplicationScheme", nb::is_arithmetic())
      .value("sequential", ApplicationSchemeType::Sequential)
      .value("reference", ApplicationSchemeType::Sequential)
      .value("one_to_one", ApplicationSchemeType::OneToOne)
      .value("naive", ApplicationSchemeType::OneToOne)
      .value("lookahead", ApplicationSchemeType::Lookahead)
      .value(
          "gate_cost", ApplicationSchemeType::GateCost,
          "Each gate of the first circuit is associated with a corresponding "
          "cost according to some cost function *f(...)*. Whenever a gate *g* "
          "from the first circuit is applied *f(g)* gates are applied from the "
          "second circuit. Referred to as *compilation_flow* in "
          ":cite:p:`burgholzer2020verifyingResultsIBM`.")
      .value("compilation_flow", ApplicationSchemeType::GateCost)
      .value("proportional", ApplicationSchemeType::Proportional);
}

} // namespace ec
