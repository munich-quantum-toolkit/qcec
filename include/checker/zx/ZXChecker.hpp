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
#include "checker/EquivalenceChecker.hpp"
#include "checker/zx/Simplify.hpp"
#include "checker/zx/ZXDefinitions.hpp"
#include "checker/zx/ZXDiagram.hpp"
#include "ir/Permutation.hpp"
#include "ir/QuantumComputation.hpp"

#include <cstddef>
#include <nlohmann/json.hpp>

namespace ec {
class ZXEquivalenceChecker : public EquivalenceChecker {
public:
  ZXEquivalenceChecker(const qc::QuantumComputation& circ1,
                       const qc::QuantumComputation& circ2,
                       Configuration config) noexcept;

  EquivalenceCriterion run() override;

  static bool canHandle(const qc::QuantumComputation& qc1,
                        const qc::QuantumComputation& qc2);

  void json(nlohmann::basic_json<>& j) const noexcept override {
    EquivalenceChecker::json(j);
    j["checker"] = "zx";
  }

private:
  ::ec::zx::ZXDiagram miter;
  ::ec::zx::fp tolerance;
  bool ancilla = false;
};

qc::Permutation complete(const qc::Permutation& p,
                         const qc::Permutation& reference);
qc::Permutation concat(const qc::Permutation& p1, const qc::Permutation& p2);
qc::Permutation invert(const qc::Permutation& p);
qc::Permutation invertPermutations(const qc::QuantumComputation& qc);
} // namespace ec
