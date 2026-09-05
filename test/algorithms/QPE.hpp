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

namespace ec::test {
[[nodiscard]] auto createQPE(qc::Qubit nq, bool exact = true,
                             std::size_t seed = 0) -> qc::QuantumComputation;

[[nodiscard]] auto createQPE(qc::fp lambda, qc::Qubit precision)
    -> qc::QuantumComputation;

[[nodiscard]] auto createIterativeQPE(qc::Qubit nq, bool exact = true,
                                      std::size_t seed = 0)
    -> qc::QuantumComputation;

[[nodiscard]] auto createIterativeQPE(qc::fp lambda, qc::Qubit precision)
    -> qc::QuantumComputation;
} // namespace ec::test
