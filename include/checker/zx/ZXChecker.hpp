/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
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
#include "ir/Permutation.hpp"
#include "ir/QuantumComputation.hpp"
#include "zx/Rules.hpp"
#include "zx/ZXDefinitions.hpp"
#include "zx/ZXDiagram.hpp"

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
  zx::ZXDiagram miter;
  zx::fp tolerance;
  bool ancilla = false;

  // the following methods are adaptations of the core ZX simplification
  // routines that additionally check a criterion for early termination of the
  // simplification.
  bool fullReduceApproximate();
  bool fullReduce();

  bool gadgetSimp();
  bool interiorCliffordSimp();
  bool cliffordSimp();

  bool idSimp() { return simplifyVertices(zx::checkIdSimp, zx::removeId); }

  bool spiderSimp() {
    return simplifyEdges(zx::checkSpiderFusion, zx::fuseSpiders);
  }

  bool localCompSimp() {
    return simplifyVertices(zx::checkLocalComp, zx::localComp);
  }

  bool pivotPauliSimp() {
    return simplifyEdges(zx::checkPivotPauli, zx::pivotPauli);
  }

  bool pivotSimp() { return simplifyEdges(zx::checkPivot, zx::pivot); }

  bool pivotGadgetSimp() {
    return simplifyEdges(zx::checkPivotGadget, zx::pivotGadget);
  }

  template <class CheckFun, class RuleFun>
  bool simplifyVertices(CheckFun check, RuleFun rule) {
    auto simplified = false;
    while (!isDone()) {
      auto moreSimplified = false;
      for (const auto& [v, _] : miter.getVertices()) {
        if (isDone() || !check(miter, v)) {
          continue;
        }
        rule(miter, v);
        moreSimplified = true;
      }
      if (!moreSimplified) {
        break;
      }
      simplified = true;
    }
    return simplified;
  }

  template <class CheckFun, class RuleFun>
  bool simplifyEdges(CheckFun check, RuleFun rule) {
    auto simplified = false;
    while (!isDone()) {
      auto moreSimplified = false;
      for (const auto& [v0, v1] : miter.getEdges()) {
        if (isDone() || miter.isDeleted(v0) || miter.isDeleted(v1) ||
            !check(miter, v0, v1)) {
          continue;
        }
        rule(miter, v0, v1);
        moreSimplified = true;
      }
      if (!moreSimplified) {
        break;
      }
      simplified = true;
    }
    return simplified;
  }
};

qc::Permutation complete(const qc::Permutation& p, std::size_t n);
qc::Permutation concat(const qc::Permutation& p1, const qc::Permutation& p2);
qc::Permutation invert(const qc::Permutation& p);
qc::Permutation invertPermutations(const qc::QuantumComputation& qc);
} // namespace ec
