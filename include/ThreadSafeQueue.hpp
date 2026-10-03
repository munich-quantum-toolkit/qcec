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

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <utility>

namespace ec {
template <typename T> class ThreadSafeQueue {
public:
  std::shared_ptr<T> waitAndPop() {
    std::unique_lock lock(mutex);
    dataCond.wait(lock, [this] { return !data.empty(); });
    const auto value = data.front();
    data.pop();
    return value;
  }

  template <typename Clock, typename Dur>
  std::shared_ptr<T>
  waitAndPopUntil(const std::chrono::time_point<Clock, Dur>& timepoint) {
    std::unique_lock lock(mutex);
    if (!dataCond.wait_until(lock, timepoint,
                             [this] { return !data.empty(); })) {
      return nullptr;
    }
    const auto value = data.front();
    data.pop();
    return value;
  }

  void push(T value) {
    auto item = std::make_shared<T>(std::move(value));
    {
      const std::scoped_lock lock(mutex);
      data.push(std::move(item));
    }
    dataCond.notify_one();
  }
  [[nodiscard]] bool empty() {
    const std::scoped_lock lock(mutex);
    return data.empty();
  }

private:
  std::mutex mutex;
  std::queue<std::shared_ptr<T>> data;
  std::condition_variable dataCond;
};
} // namespace ec
