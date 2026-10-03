/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "ThreadSafeQueue.hpp"

#include <chrono>
#include <future>
#include <gtest/gtest.h>

TEST(ThreadSafeQueueTest, WaitsForProducerAndHonorsDeadline) {
  ec::ThreadSafeQueue<int> queue;
  EXPECT_EQ(queue.waitAndPopUntil(std::chrono::steady_clock::now()), nullptr);

  auto producer = std::async(std::launch::async, [&queue] { queue.push(42); });
  EXPECT_EQ(*queue.waitAndPop(), 42);
  producer.get();
}
