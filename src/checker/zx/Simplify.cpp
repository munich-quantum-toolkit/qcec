/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "checker/zx/Simplify.hpp"

#include "checker/zx/Rules.hpp"
#include "checker/zx/ZXDefinitions.hpp"
#include "checker/zx/ZXDiagram.hpp"

#include <cstddef>

namespace ec::zx {

std::size_t gadgetSimp(ZXDiagram& diag,
                       const CancellationPredicate& cancelled) {
  std::size_t nSimplifications = 0;
  bool newMatches = true;

  while (newMatches && !cancellationRequested(cancelled)) {
    newMatches = false;
    for (auto [v, _] : diag.getVertices()) {
      if (cancellationRequested(cancelled)) {
        break;
      }
      if (diag.isDeleted(v)) {
        continue;
      }

      if (checkAndFuseGadget(diag, v)) {
        newMatches = true;
        nSimplifications++;
      }
    }
  }
  return nSimplifications;
}

std::size_t idSimp(ZXDiagram& diag, const CancellationPredicate& cancelled) {
  return simplifyVertices(diag, checkIdSimp, removeId, cancelled);
}

std::size_t spiderSimp(ZXDiagram& diag,
                       const CancellationPredicate& cancelled) {
  return simplifyEdges(diag, checkSpiderFusion, fuseSpiders, cancelled);
}

std::size_t localCompSimp(ZXDiagram& diag,
                          const CancellationPredicate& cancelled) {
  return simplifyVertices(diag, checkLocalComp, localComp, cancelled);
}

std::size_t pivotSimp(ZXDiagram& diag, const CancellationPredicate& cancelled) {
  return simplifyEdges(diag, checkPivot, pivot, cancelled);
}

std::size_t pivotPauliSimp(ZXDiagram& diag,
                           const CancellationPredicate& cancelled) {
  return simplifyEdges(diag, checkPivotPauli, pivotPauli, cancelled);
}

std::size_t interiorCliffordSimp(ZXDiagram& diag,
                                 const CancellationPredicate& cancelled) {
  auto nSimplifications = spiderSimp(diag, cancelled);

  bool newMatches = true;
  while (newMatches && !cancellationRequested(cancelled)) {
    newMatches = false;
    const auto nId = idSimp(diag, cancelled);
    const auto nSpider = spiderSimp(diag, cancelled);
    const auto nPivot = pivotPauliSimp(diag, cancelled);
    const auto nLocalComp = localCompSimp(diag, cancelled);

    const auto nNewSimplifications = nId + nSpider + nPivot + nLocalComp;
    if (nNewSimplifications != 0) {
      newMatches = true;
      nSimplifications += nNewSimplifications;
    }
  }
  return nSimplifications;
}

std::size_t cliffordSimp(ZXDiagram& diag,
                         const CancellationPredicate& cancelled) {
  bool newMatches = true;
  std::size_t nSimplifications = 0;
  while (newMatches && !cancellationRequested(cancelled)) {
    newMatches = false;
    const auto nClifford = interiorCliffordSimp(diag, cancelled);
    const auto nPivot = pivotSimp(diag, cancelled);
    const auto nNewSimplifications = nClifford + nPivot;
    if (nNewSimplifications != 0) {
      newMatches = true;
      nSimplifications += nNewSimplifications;
    }
  }
  return nSimplifications;
}

std::size_t pivotgadgetSimp(ZXDiagram& diag,
                            const CancellationPredicate& cancelled) {
  return simplifyEdges(diag, checkPivotGadget, pivotGadget, cancelled);
}

std::size_t fullReduce(ZXDiagram& diag,
                       const CancellationPredicate& cancelled) {
  if (cancellationRequested(cancelled)) {
    return 0;
  }
  diag.toGraphlike();
  auto nSimplifications = interiorCliffordSimp(diag, cancelled);

  while (!cancellationRequested(cancelled)) {
    const auto nClifford = cliffordSimp(diag, cancelled);
    const auto nGadget = gadgetSimp(diag, cancelled);
    const auto nInterior = interiorCliffordSimp(diag, cancelled);
    const auto nPivot = pivotgadgetSimp(diag, cancelled);
    const auto nNewSimplifications = nClifford + nGadget + nInterior + nPivot;
    if (nNewSimplifications == 0) {
      break;
    }
    nSimplifications += nNewSimplifications;
  }
  if (!cancellationRequested(cancelled)) {
    diag.removeDisconnectedSpiders();
  }

  return nSimplifications;
}

std::size_t fullReduceApproximate(ZXDiagram& diag, const fp tolerance,
                                  const CancellationPredicate& cancelled) {
  auto nSimplifications = fullReduce(diag, cancelled);
  while (!cancellationRequested(cancelled)) {
    diag.approximateCliffords(tolerance);
    const auto newSimps = fullReduce(diag, cancelled);
    if (newSimps == 0U) {
      break;
    }
    nSimplifications += newSimps;
  }
  return nSimplifications;
}
} // namespace ec::zx
