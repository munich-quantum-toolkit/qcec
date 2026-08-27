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

#include "StateType.hpp"
#include "dd/Package_fwd.hpp"

#include <cstddef>
#include <random>
#include <unordered_set>

namespace ec {
class StateGenerator {
public:
  explicit StateGenerator(const std::size_t s) : seed(s) {
    seedGenerator(seed);
  }
  StateGenerator() : StateGenerator(0U) {}

  dd::VectorDD
  generateRandomState(dd::Package& dd, std::size_t totalQubits,
                      std::size_t ancillaryQubits = 0U,
                      StateType type = StateType::ComputationalBasis);

  dd::VectorDD
  generateRandomComputationalBasisState(dd::Package& dd,
                                        std::size_t totalQubits,
                                        std::size_t ancillaryQubits = 0U);

  dd::VectorDD generateRandom1QBasisState(dd::Package& dd,
                                          std::size_t totalQubits,
                                          std::size_t ancillaryQubits = 0U);

  dd::VectorDD generateRandomStabilizerState(dd::Package& dd,
                                             std::size_t totalQubits,
                                             std::size_t ancillaryQubits = 0U);

  void seedGenerator(std::size_t s);

  void clear() { generatedComputationalBasisStates.clear(); }

private:
  std::size_t seed = 0U;
  std::mt19937_64 mt;

  std::unordered_set<std::size_t> generatedComputationalBasisStates;
  constexpr static std::size_t ONE_QUBIT_BASE_ELEMENTS = 6U;
  // this generator produces random bases from the set { |0>, |1>, |+>, |->,
  // |L>, |R> }
  std::uniform_int_distribution<std::size_t> random1QBasisDistribution =
      std::uniform_int_distribution<std::size_t>(0U,
                                                 ONE_QUBIT_BASE_ELEMENTS - 1U);
};
} // namespace ec
