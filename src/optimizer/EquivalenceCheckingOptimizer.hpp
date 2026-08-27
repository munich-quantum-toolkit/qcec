/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

/** @file EquivalenceCheckingOptimizer.hpp
 * @brief Internal circuit transformations used during equivalence checking.
 */

#pragma once

namespace qc {
class QuantumComputation;
}

namespace ec::detail {

void swapReconstruction(qc::QuantumComputation& qc);
void removeDiagonalGatesBeforeMeasure(qc::QuantumComputation& qc);
void eliminateResets(qc::QuantumComputation& qc);
void deferMeasurements(qc::QuantumComputation& qc);
void backpropagateOutputPermutation(qc::QuantumComputation& qc);
void elidePermutations(qc::QuantumComputation& qc);

} // namespace ec::detail
