/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "checker/zx/ZXChecker.hpp"

#include "Configuration.hpp"
#include "EquivalenceCriterion.hpp"
#include "checker/EquivalenceChecker.hpp"
#include "ir/Definitions.hpp"
#include "ir/QuantumComputation.hpp"
#include "zx/FunctionalityConstruction.hpp"
#include "zx/Rules.hpp"
#include "zx/ZXDefinitions.hpp"
#include "zx/ZXDiagram.hpp"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ec {
ZXEquivalenceChecker::ZXEquivalenceChecker(const qc::QuantumComputation& circ1,
                                           const qc::QuantumComputation& circ2,
                                           Configuration config) noexcept
    : EquivalenceChecker(circ1, circ2, std::move(config)),
      miter(zx::FunctionalityConstruction::buildFunctionality(qc1)),
      tolerance(configuration.functionality.traceThreshold) {
  zx::ZXDiagram dPrime = zx::FunctionalityConstruction::buildFunctionality(qc2);

  if ((qc1->getNancillae() != 0U) || (qc2->getNancillae() != 0U)) {
    ancilla = true;
  }

  const auto& p1 = invertPermutations(*qc1);
  const auto& p2 = invertPermutations(*qc2);

  /*
   * The ZX diagram is built with the assumption that all ancilla qubits are
   * garbage. Garbage ancilla qubits are initialized and post-selected to |0>.
   * Consequently, if there are no data qubits, the ZX-diagram is equivalent to
   * the empty diagram.
   */
  if (qc1->getNqubitsWithoutAncillae() == 0) {
    this->miter = zx::ZXDiagram();
  } else {
    const auto numQubits1 = static_cast<zx::Qubit>(qc1->getNqubits());
    for (zx::Qubit i = 0; i < static_cast<zx::Qubit>(qc1->getNancillae());
         ++i) {
      const auto anc = numQubits1 - i - 1;
      miter.makeAncilla(
          anc, static_cast<zx::Qubit>(p1.at(static_cast<qc::Qubit>(anc))));
    }
    miter.invert();
  }

  if (qc2->getNqubitsWithoutAncillae() == 0) {
    return;
  }
  const auto numQubits2 = static_cast<zx::Qubit>(qc2->getNqubits());
  for (zx::Qubit i = 0; i < static_cast<zx::Qubit>(qc2->getNancillae()); ++i) {
    const auto anc = numQubits2 - i - 1;
    dPrime.makeAncilla(
        anc, static_cast<zx::Qubit>(p2.at(static_cast<qc::Qubit>(anc))));
  }
  miter.concat(dPrime);
}

EquivalenceCriterion ZXEquivalenceChecker::run() {
  const auto start = std::chrono::steady_clock::now();
  if (miter.getNQubits() == 0) {
    if (miter.globalPhaseIsZero()) {
      equivalence = EquivalenceCriterion::Equivalent;
    } else {
      equivalence = EquivalenceCriterion::EquivalentUpToGlobalPhase;
    }
    return equivalence;
  }
  fullReduceApproximate();

  bool equivalent = true;

  if (miter.getNEdges() == miter.getNQubits()) {
    const auto& p1 = invert(invertPermutations(*qc1));
    const auto& p2 = invert(invertPermutations(*qc2));

    const auto nQubits = miter.getNQubits();
    for (std::size_t i = 0U; i < nQubits; ++i) {
      if (qc1->logicalQubitIsGarbage(static_cast<qc::Qubit>(i)) &&
          qc2->logicalQubitIsGarbage(static_cast<qc::Qubit>(i))) {
        continue;
      }

      const auto& in = miter.getInput(i);
      const auto& edge = miter.incidentEdge(in, 0U);

      if (edge.type == zx::EdgeType::Hadamard) {
        equivalent = false;
        break;
      }
      const auto& out = edge.to;
      const auto& q1 = miter.getVData(in);
      const auto& q2 = miter.getVData(out);
      assert(q1.has_value());
      assert(q2.has_value());
      if (p1.at(static_cast<qc::Qubit>(q1->qubit)) !=
          p2.at(static_cast<qc::Qubit>(q2->qubit))) {
        equivalent = false;
        break;
      }
    }
  } else {
    equivalent = false;
  }

  const auto end = std::chrono::steady_clock::now();
  runtime += std::chrono::duration<double>(end - start).count();

  // non-equivalence might be due to incorrect assumption about the state of
  // ancillaries or the check was aborted prematurely, so no information can be
  // given
  if ((!equivalent && ancilla) || isDone()) {
    equivalence = EquivalenceCriterion::NoInformation;
  } else {
    if (equivalent) {
      if (miter.globalPhaseIsZero()) {
        equivalence = EquivalenceCriterion::Equivalent;
      } else {
        equivalence = EquivalenceCriterion::EquivalentUpToGlobalPhase;
      }
    } else {
      equivalence = EquivalenceCriterion::ProbablyNotEquivalent;
    }
  }
  return equivalence;
}

qc::Permutation invert(const qc::Permutation& p) {
  qc::Permutation pInv{};
  for (const auto [v, w] : p) {
    pInv[w] = v;
  }
  return pInv;
}
qc::Permutation concat(const qc::Permutation& p1,
                       const qc::Permutation& p2) { // p2 after p1
  qc::Permutation pComb{};

  for (const auto [v, w] : p1) {
    if (p2.find(w) != p2.end()) {
      pComb[v] = p2.at(w);
    }
  }
  return pComb;
}

qc::Permutation complete(const qc::Permutation& p, const std::size_t n) {
  if (p.size() == n) {
    return p;
  }

  qc::Permutation pComp = p;

  std::vector<bool> mappedTo(n, false);
  std::unordered_map<std::size_t, bool> mappedFrom;
  for (const auto [k, v] : p) {
    mappedFrom[k] = true;
    mappedTo[v] = true;
  }

  // Map qubits greedily
  for (std::size_t i = 0; i < n; ++i) {
    // If qubit is already mapped, skip
    if (mappedTo[i]) {
      continue;
    }

    for (std::size_t j = 0; j < n; ++j) {
      if (mappedFrom.find(j) == mappedFrom.end()) {
        pComp[static_cast<qc::Qubit>(j)] = static_cast<qc::Qubit>(i);
        mappedTo[i] = true;
        mappedFrom[j] = true;
        break;
      }
    }
  }
  return pComp;
}

qc::Permutation invertPermutations(const qc::QuantumComputation& qc) {
  return concat(invert(complete(qc.outputPermutation, qc.getNqubits())),
                complete(qc.initialLayout, qc.getNqubits()));
}

bool ZXEquivalenceChecker::fullReduceApproximate() {
  auto simplified = fullReduce();
  while (!isDone()) {
    miter.approximateCliffords(tolerance);
    if (!fullReduce()) {
      break;
    }
    simplified = true;
  }
  return simplified;
}

bool ZXEquivalenceChecker::fullReduce() {
  if (!isDone()) {
    miter.toGraphlike();
  }
  auto simplified = interiorCliffordSimp();
  while (!isDone()) {
    auto moreSimplified = cliffordSimp();
    moreSimplified |= gadgetSimp();
    moreSimplified |= interiorCliffordSimp();
    moreSimplified |= pivotGadgetSimp();
    if (!moreSimplified) {
      break;
    }
    simplified = true;
  }
  if (!isDone()) {
    miter.removeDisconnectedSpiders();
  }
  return simplified;
}

bool ZXEquivalenceChecker::gadgetSimp() {
  auto simplified = false;
  while (!isDone()) {
    auto moreSimplified = false;
    for (const auto& [v, _] : miter.getVertices()) {
      if (miter.isDeleted(v)) {
        continue;
      }
      if (checkAndFuseGadget(miter, v)) {
        moreSimplified = true;
      }
    }
    if (!moreSimplified) {
      break;
    }
    simplified = true;
  }
  return simplified;
}

bool ZXEquivalenceChecker::interiorCliffordSimp() {
  auto simplified = spiderSimp();
  while (!isDone()) {
    auto moreSimplified = idSimp();
    moreSimplified |= spiderSimp();
    moreSimplified |= pivotPauliSimp();
    moreSimplified |= localCompSimp();
    if (!moreSimplified) {
      break;
    }
    simplified = true;
  }
  return simplified;
}

bool ZXEquivalenceChecker::cliffordSimp() {
  auto simplified = false;
  while (!isDone()) {
    auto moreSimplified = interiorCliffordSimp();
    moreSimplified |= pivotSimp();
    if (!moreSimplified) {
      break;
    }
    simplified = true;
  }
  return simplified;
}

bool ZXEquivalenceChecker::canHandle(const qc::QuantumComputation& qc1,
                                     const qc::QuantumComputation& qc2) {
  // no non-garbage ancillas allowed
  if (qc1.getNancillae() - qc1.getNgarbageQubits() != 0U ||
      qc2.getNancillae() - qc2.getNgarbageQubits() != 0U) {
    return false;
  }
  return zx::FunctionalityConstruction::transformableToZX(&qc1) &&
         zx::FunctionalityConstruction::transformableToZX(&qc2);
}
} // namespace ec
