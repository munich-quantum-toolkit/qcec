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
class OneToOneApplicationScheme final : public ApplicationScheme<DDType> {
public:
  OneToOneApplicationScheme(TaskManager<DDType>& tm1,
                            TaskManager<DDType>& tm2) noexcept
      : ApplicationScheme<DDType>(tm1, tm2) {}

  std::pair<size_t, size_t> operator()() noexcept override { return {1U, 1U}; }
};
} // namespace ec
