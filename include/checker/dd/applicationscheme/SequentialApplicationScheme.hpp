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

#include <cstddef>
#include <utility>

namespace ec {
template <class DDType>
class SequentialApplicationScheme final : public ApplicationScheme<DDType> {
public:
  SequentialApplicationScheme(TaskManager<DDType>& tm1,
                              TaskManager<DDType>& tm2) noexcept
      : ApplicationScheme<DDType>(tm1, tm2),
        gates1(tm1.getCircuit()->getNops()),
        gates2(tm2.getCircuit()->getNops()) {}

  std::pair<size_t, size_t> operator()() noexcept override {
    return {gates1, gates2};
  }

private:
  std::size_t gates1;
  std::size_t gates2;
};
} // namespace ec
