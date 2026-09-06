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

#include <bitset>
#include <cstddef>

namespace qc {
class QuantumComputation;
} // namespace qc

namespace ec::test {
using BVBitString = std::bitset<4096>;

[[nodiscard]] auto createBernsteinVazirani(const BVBitString& hiddenString)
    -> qc::QuantumComputation;
[[nodiscard]] auto createBernsteinVazirani(qc::Qubit nq, std::size_t seed = 0)
    -> qc::QuantumComputation;
[[nodiscard]] auto createBernsteinVazirani(const BVBitString& hiddenString,
                                           qc::Qubit nq)
    -> qc::QuantumComputation;

[[nodiscard]] auto
createIterativeBernsteinVazirani(const BVBitString& hiddenString)
    -> qc::QuantumComputation;
[[nodiscard]] auto createIterativeBernsteinVazirani(qc::Qubit nq,
                                                    std::size_t seed = 0)
    -> qc::QuantumComputation;
[[nodiscard]] auto
createIterativeBernsteinVazirani(const BVBitString& hiddenString, qc::Qubit nq)
    -> qc::QuantumComputation;
} // namespace ec::test
