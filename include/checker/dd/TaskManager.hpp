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

#include "dd/DDpackageConfig.hpp"
#include "dd/Node.hpp"
#include "dd/Operations.hpp"
#include "dd/Package.hpp"
#include "ir/QuantumComputation.hpp"
#include "ir/operations/OpType.hpp"
#include "ir/operations/Operation.hpp"

#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

namespace ec {
enum class Direction : bool { Left = true, Right = false };

template <class DDType> class TaskManager {
public:
  TaskManager(const qc::QuantumComputation& circ, dd::Package& dd,
              const Direction& dir) noexcept
      : qc(&circ), package(&dd), direction(dir),
        permutation(circ.initialLayout), iterator(circ.begin()),
        end(circ.end()) {}
  TaskManager(const qc::QuantumComputation& circ, dd::Package& dd) noexcept
      : TaskManager(circ, dd, Direction::Left) {}

  void reset() noexcept {
    iterator = qc->begin();
    permutation = qc->initialLayout;
  }

  [[nodiscard]] bool finished() const noexcept { return iterator == end; }

  const std::unique_ptr<qc::Operation>& operator()() const { return *iterator; }

  [[nodiscard]] const DDType& getInternalState() const noexcept {
    return internalState;
  }
  void setInternalState(const DDType& state) noexcept { internalState = state; }
  void flipDirection() noexcept {
    if (direction == Direction::Left) {
      direction = Direction::Right;
    } else {
      direction = Direction::Left;
    }
  }

  [[nodiscard]] dd::MatrixDD getDD() {
    return dd::getDD(**iterator, *package, permutation);
  }
  [[nodiscard]] dd::MatrixDD getInverseDD() {
    return dd::getInverseDD(**iterator, *package, permutation);
  }

  [[nodiscard]] const qc::QuantumComputation* getCircuit() const noexcept {
    return qc;
  }

  qc::QuantumComputation::const_iterator getIterator() const {
    return iterator;
  }

  void advanceIterator() { ++iterator; }

  void applyGate(DDType& to) {
    auto saved = to;
    if constexpr (std::is_same_v<DDType, dd::VectorDD>) {
      // direction has no effect on state vector DDs
      to = package->multiply(getDD(), to);
    } else {
      if (direction == Direction::Left) {
        to = package->multiply(getDD(), to);
      } else {
        to = package->multiply(to, getInverseDD());
      }
    }
    package->incRef(to);
    package->decRef(saved);
    package->garbageCollect();
    ++iterator;
  }

  void applySwapOperations() {
    while (!finished() && (*iterator)->getType() == qc::SWAP &&
           !(*iterator)->isControlled()) {
      const auto& targets = (*iterator)->getTargets();
      assert(targets.size() == 2);
      const auto t1 = targets[0];
      const auto t2 = targets[1];
      std::swap(permutation.at(t1), permutation.at(t2));
      ++iterator;
    }
  }

  void advance(DDType& state, const std::size_t steps) {
    for (std::size_t i = 0U; i < steps && !finished(); ++i) {
      applyGate(state);
      applySwapOperations();
    }
  }
  void advance(DDType& state) { advance(state, 1U); }
  void advance(std::size_t steps) { advance(internalState, steps); }
  void advance() { advance(1U); }

  void finish(DDType& state) {
    while (!finished()) {
      advance(state);
    }
  }
  void finish() { finish(internalState); }

  void changePermutation(DDType& state) {
    dd::changePermutation(state, permutation, qc->outputPermutation, *package,
                          static_cast<bool>(direction));
  }
  void changePermutation() { changePermutation(internalState); }

  void reduceAncillae(DDType& state) {
    if constexpr (std::is_same_v<DDType, dd::MatrixDD>) {
      state = package->reduceAncillae(state, qc->getAncillary(),
                                      static_cast<bool>(direction));
    }
  }
  void reduceAncillae() { reduceAncillae(internalState); }

  /**
   Reduces garbage qubits such that the matrix will be equal to another reduced
   matrix iff the two underlying circuits are partially equivalent.
   **/
  void reduceGarbage(DDType& state) {
    if constexpr (std::is_same_v<DDType, dd::VectorDD>) {
      state = package->reduceGarbage(state, qc->getGarbage(), true);
    } else if constexpr (std::is_same_v<DDType, dd::MatrixDD>) {
      state = package->reduceGarbage(state, qc->getGarbage(),
                                     static_cast<bool>(direction), true);
    }
  }
  void reduceGarbage() { reduceGarbage(internalState); }

  void incRef(DDType& state) { package->incRef(state); }
  void incRef() { incRef(internalState); }

  void decRef(DDType& state) { package->decRef(state); }
  void decRef() { decRef(internalState); }

private:
  const qc::QuantumComputation* qc{};
  dd::Package* package;
  Direction direction = Direction::Left;
  qc::Permutation permutation{};
  decltype(qc->begin()) iterator;
  decltype(qc->end()) end;
  DDType internalState{};
};
} // namespace ec
