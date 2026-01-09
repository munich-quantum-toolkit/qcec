/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "checker/dd/applicationscheme/LookaheadApplicationScheme.hpp"

#include "checker/dd/TaskManager.hpp"
#include "checker/dd/applicationscheme/ApplicationScheme.hpp"
#include "dd/Node.hpp"
#include "dd/Package.hpp"

#include <cassert>
#include <cstddef>
#include <utility>

namespace ec {

LookaheadApplicationScheme::LookaheadApplicationScheme(
    TaskManager<dd::MatrixDD>& tm1, TaskManager<dd::MatrixDD>& tm2) noexcept
    : ApplicationScheme(tm1, tm2) {}
void LookaheadApplicationScheme::setInternalState(
    dd::MatrixDD& state) noexcept {
  internalState = &state;
}
void LookaheadApplicationScheme::setPackage(dd::Package* dd) noexcept {
  package = dd;
}
std::pair<size_t, size_t> LookaheadApplicationScheme::operator()() {
  assert(internalState != nullptr);
  assert(package != nullptr);

  if (!cached1) {
    // cache the current operation
    op1 = taskManager1->getDD();
    package->incRef(op1);
    cached1 = true;
  }

  if (!cached2) {
    // cache the current operation
    op2 = taskManager2->getInverseDD();
    package->incRef(op2);
    cached2 = true;
  }

  // compute both possible applications and measure the resulting size
  auto saved = *internalState;
  const auto dd1 = package->multiply(op1, saved);
  const auto size1 = dd1.size();
  const auto dd2 = package->multiply(saved, op2);

  // greedily chose the smaller resulting decision diagram
  if (const auto size2 = dd2.size(); size1 <= size2) {
    assert(!taskManager1->finished());
    *internalState = dd1;
    package->decRef(op1);
    cached1 = false;
    taskManager1->advanceIterator();
  } else {
    assert(!taskManager2->finished());
    *internalState = dd2;
    package->decRef(op2);
    cached2 = false;
    taskManager2->advanceIterator();
  }

  // properly track reference counts
  package->incRef(*internalState);
  package->decRef(saved);
  package->garbageCollect();

  // no operations shall be applied by the outer loop in which the application
  // scheme is invoked
  return {0U, 0U};
}
} // namespace ec
