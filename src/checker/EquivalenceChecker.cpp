/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "checker/EquivalenceChecker.hpp"

#include "EquivalenceCriterion.hpp"

#include <nlohmann/json.hpp>

// this function is mainly placed here in order to have an out-of-line
// virtual method definition which avoids emitting the classes' vtable in
// every translation unit.
void ec::EquivalenceChecker::json(nlohmann::basic_json<>& j) const noexcept {
  j["equivalence"] = toString(equivalence);
  j["runtime"] = getRuntime();
}
