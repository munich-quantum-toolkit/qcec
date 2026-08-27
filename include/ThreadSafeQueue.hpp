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

namespace ec {
template <typename T> class ThreadSafeQueue {
public:
  ThreadSafeQueue() : tail(head.get()) {}

  ThreadSafeQueue(const ThreadSafeQueue& other) = delete;

  ThreadSafeQueue& operator=(const ThreadSafeQueue& other) = delete;

  std::shared_ptr<T> waitAndPop() {
    auto const oldHead = waitPopHead();
    return oldHead->data;
  }

  template <typename Clock, typename Dur>
  std::shared_ptr<T>
  waitAndPopUntil(const std::chrono::time_point<Clock, Dur>& timepoint) {
    if (const auto oldHead = waitPopHeadUntil(timepoint)) {
      return oldHead->data;
    }
    return nullptr;
  }

  void push(T value) {
    auto data(std::make_shared<T>(std::move(value)));
    auto p = std::make_unique<Node>();
    {
      const std::lock_guard tailLock(tailMutex);
      tail->data = data;
      Node* const newTail = p.get();
      tail->next = std::move(p);
      tail = newTail;
    }
    dataCond.notify_one();
  }
  [[nodiscard]] bool empty() {
    const std::lock_guard headLock(headMutex);
    return head.get() == getTail();
  }

private:
  struct Node {
    std::shared_ptr<T> data;
    std::unique_ptr<Node> next;
  };
  std::mutex headMutex;
  std::unique_ptr<Node> head = std::make_unique<Node>();
  std::mutex tailMutex;
  Node* tail;
  std::condition_variable dataCond;

  auto getTail() {
    const std::lock_guard tailLock(tailMutex);
    return tail;
  }

  auto popHead() {
    auto oldHead = std::move(head);
    head = std::move(oldHead->next);
    return oldHead;
  }

  auto waitForData() {
    std::unique_lock headLock(headMutex);
    dataCond.wait(headLock, [this] { return head.get() != getTail(); });
    return headLock;
  }

  template <typename Clock, typename Dur>
  auto waitForDataUntil(const std::chrono::time_point<Clock, Dur>& timepoint) {
    std::unique_lock headLock(headMutex);
    dataCond.wait_until(headLock, timepoint,
                        [this] { return head.get() != getTail(); });
    return headLock;
  }

  auto waitPopHead() {
    [[maybe_unused]] auto headLock(waitForData());
    return popHead();
  }

  template <typename Clock, typename Dur>
  auto waitPopHeadUntil(const std::chrono::time_point<Clock, Dur>& timepoint) {
    [[maybe_unused]] auto headLock(waitForDataUntil(timepoint));
    if (head.get() == getTail()) {
      return std::unique_ptr<Node>();
    }
    return popHead();
  }
};
} // namespace ec
