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

#include "Configuration.hpp"
#include "EquivalenceCriterion.hpp"
#include "ir/QuantumComputation.hpp"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <nlohmann/json_fwd.hpp>
#include <utility>

namespace ec {
class EquivalenceChecker {
public:
  EquivalenceChecker(const qc::QuantumComputation& circ1,
                     const qc::QuantumComputation& circ2,
                     Configuration config) noexcept
      : qc1(&circ1), qc2(&circ2),
        nqubits(std::max(qc1->getNqubits(), qc2->getNqubits())),
        configuration(std::move(config)) {};

  virtual ~EquivalenceChecker() = default;

  virtual EquivalenceCriterion run() = 0;

  [[nodiscard]] EquivalenceCriterion getEquivalence() const noexcept {
    return equivalence;
  }
  [[nodiscard]] double getRuntime() const noexcept { return runtime; }

  virtual void json(nlohmann::json& j) const noexcept;

  void signalDone() { done.store(true, std::memory_order_relaxed); }
  [[nodiscard]] auto isDone() const {
    return done.load(std::memory_order_relaxed);
  }

protected:
  qc::QuantumComputation const* qc1;
  qc::QuantumComputation const* qc2;

  std::size_t nqubits{};

  Configuration configuration;

  EquivalenceCriterion equivalence = EquivalenceCriterion::NoInformation;
  double runtime{};

private:
  std::atomic<bool> done{false};
};

} // namespace ec
