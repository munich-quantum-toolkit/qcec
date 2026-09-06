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

namespace qc {
class QuantumComputation;
} // namespace qc

namespace ec::test {
[[nodiscard]] auto createQFT(qc::Qubit nq, bool includeMeasurements = true)
    -> qc::QuantumComputation;

[[nodiscard]] auto createIterativeQFT(qc::Qubit nq) -> qc::QuantumComputation;
} // namespace ec::test
