/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#pragma once

#include "ir/Definitions.hpp"

#include <cstddef>

namespace qc {
class QuantumComputation;
} // namespace qc

namespace ec::detail {
[[nodiscard]] auto createRandomCliffordCircuit(qc::Qubit nq, std::size_t depth,
                                               std::size_t seed)
    -> qc::QuantumComputation;
} // namespace ec::detail
