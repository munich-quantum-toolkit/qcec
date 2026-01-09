/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "checker/dd/simulation/StateType.hpp"

#include <nanobind/nanobind.h>

namespace ec {

namespace nb = nanobind;
using namespace nb::literals;

// NOLINTNEXTLINE(misc-use-internal-linkage)
void registerStateType(const nb::module_& m) {
  nb::enum_<StateType>(m, "StateType", nb::is_arithmetic())
      .value("computational_basis", StateType::ComputationalBasis)
      .value("classical", StateType::ComputationalBasis)
      .value("random_1Q_basis", StateType::Random1QBasis)
      .value("local_quantum", StateType::Random1QBasis)
      .value("stabilizer", StateType::Stabilizer)
      .value("global_quantum", StateType::Stabilizer);
}

} // namespace ec
