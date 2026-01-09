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

#include "ApplicationScheme.hpp"
#include "checker/dd/TaskManager.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <utility>

namespace ec {
template <class DDType>
class ProportionalApplicationScheme final : public ApplicationScheme<DDType> {
public:
  ProportionalApplicationScheme(TaskManager<DDType>& tm1,
                                TaskManager<DDType>& tm2,
                                const bool singleQubitGateFusion) noexcept
      : ApplicationScheme<DDType>(tm1, tm2),
        singleQubitGateFusionEnabled(singleQubitGateFusion) {}

  std::pair<size_t, size_t> operator()() noexcept override {
    // compute the remaining size of the circuits
    const auto size1 = static_cast<std::size_t>(
        std::distance(this->taskManager1->getIterator(),
                      this->taskManager1->getCircuit()->cend()));
    const auto size2 = static_cast<std::size_t>(
        std::distance(this->taskManager2->getIterator(),
                      this->taskManager2->getCircuit()->cend()));
    assert(size1 > 0U && size2 > 0U);

    if (singleQubitGateFusionEnabled) {
      // when single qubit gates are fused, any single-qubit gate should have a
      // single (compound) gate in the other circuit as a counterpart.
      if (const auto& op = (*this->taskManager1)();
          op->getUsedQubits().size() == 1U) {
        return {1U, 1U};
      }
    }

    const auto [min, max] = std::minmax(size1, size2);
    const auto gateRatio =
        std::max((max + (min / 2U)) / min, static_cast<std::size_t>(1U));
    if (size1 >= size2) {
      return {gateRatio, 1U};
    }
    return {1U, gateRatio};
  }

private:
  bool singleQubitGateFusionEnabled;
};
} // namespace ec
