/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "checker/dd/DDAlternatingChecker.hpp"

#include "EquivalenceCriterion.hpp"
#include "checker/dd/DDEquivalenceChecker.hpp"
#include "checker/dd/DDPackageConfigs.hpp"
#include "checker/dd/applicationscheme/ApplicationScheme.hpp"
#include "checker/dd/applicationscheme/LookaheadApplicationScheme.hpp"
#include "dd/Package.hpp"
#include "ir/Definitions.hpp"

#include <cassert>
#include <cstddef>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace ec {
void DDAlternatingChecker::initialize() {
  DDEquivalenceChecker::initialize();
  // create the full identity matrix
  functionality = dd::Package::makeIdent();
  dd->incRef(functionality);

  // Only count ancillaries that are present in but not acted upon in both of
  // the circuits. Otherwise, the alternating checker must not be used.
  // Counter-example: H |0><0| H^-1 = [[0.5, 0.5], [0.5, 0.5]] != |0><0|
  if (!canHandle(*qc1, *qc2)) {
    throw std::invalid_argument(
        "Alternating checker must not be used for "
        "circuits that both have non-idle ancillary "
        "qubits. Use the construction checker instead.");
  }

  std::vector<bool> ancillary(nqubits);
  for (qc::Qubit q = 0U; q < nqubits; ++q) {
    ancillary[static_cast<std::size_t>(q)] =
        qc1->logicalQubitIsAncillary(q) && qc2->logicalQubitIsAncillary(q);
  }

  // reduce the ancillary qubit contributions
  // [1 0] if the qubit is no ancillary, or it is acted upon by both circuits
  // [0 1]
  //
  // [1 0] (= |0><0|) for an ancillary only acted on in one circuit
  // [0 0]
  functionality = dd->reduceAncillae(functionality, ancillary);
}

void DDAlternatingChecker::execute() {
  while (!taskManager1.finished() && !taskManager2.finished()) {
    // skip over any SWAP operations
    taskManager1.applySwapOperations();
    taskManager2.applySwapOperations();

    if (!taskManager1.finished() && !taskManager2.finished()) {
      // whenever the current functionality resembles the identity, identical
      // gates on both sides cancel
      if (functionality.isIdentity() &&
          (configuration.application.alternatingScheme !=
           ApplicationSchemeType::Lookahead) &&
          gatesAreIdentical()) {
        taskManager1.advanceIterator();
        taskManager2.advanceIterator();
        continue;
      }
      // query application scheme on how to proceed
      const auto [apply1, apply2] = (*applicationScheme)();

      // advance both tasks correspondingly
      taskManager1.advance(functionality, apply1);
      taskManager2.advance(functionality, apply2);
    }
  }
}

void DDAlternatingChecker::finish() {
  taskManager1.finish(functionality);
  taskManager2.finish(functionality);
}

void DDAlternatingChecker::postprocess() {
  // ensure that the permutations that were tracked throughout the circuit match
  // the expected output permutations
  taskManager1.changePermutation(functionality);
  taskManager2.changePermutation(functionality);
}

EquivalenceCriterion DDAlternatingChecker::checkEquivalence() {
  std::vector<bool> garbage(nqubits);
  for (qc::Qubit q = 0U; q < nqubits; ++q) {
    garbage[static_cast<std::size_t>(q)] =
        qc1->logicalQubitIsGarbage(q) && qc2->logicalQubitIsGarbage(q);
  }
  const bool isClose =
      configuration.functionality.checkPartialEquivalence
          ? dd->isCloseToIdentity(functionality,
                                  configuration.functionality.traceThreshold,
                                  garbage, false)
          : dd->isCloseToIdentity(functionality,
                                  configuration.functionality.traceThreshold);

  if (isClose) {
    // whenever the top edge weight is not one, both decision diagrams are only
    // equivalent up to a global phase
    if (!functionality.w.approximatelyEquals(dd::Complex::one())) {
      return EquivalenceCriterion::EquivalentUpToGlobalPhase;
    }
    return EquivalenceCriterion::Equivalent;
  }
  return EquivalenceCriterion::NotEquivalent;
}

[[nodiscard]] bool DDAlternatingChecker::gatesAreIdentical() const {
  if (taskManager1.finished() || taskManager2.finished()) {
    return false;
  }

  const auto& op1 = *taskManager1();
  const auto& op2 = *taskManager2();

  return op1.equals(op2);
}

bool DDAlternatingChecker::canHandle(const qc::QuantumComputation& qc1,
                                     const qc::QuantumComputation& qc2) {
  assert(qc1.getNqubits() == qc2.getNqubits());
  const auto nqubits = qc1.getNqubits();

  for (auto qubit = static_cast<std::make_signed_t<qc::Qubit>>(nqubits - 1U);
       qubit >= 0; --qubit) {
    const auto q = static_cast<qc::Qubit>(qubit);
    if (qc1.logicalQubitIsAncillary(q) && qc2.logicalQubitIsAncillary(q)) {
      const auto [found1, physical1] = qc1.containsLogicalQubit(q);
      const auto [found2, physical2] = qc2.containsLogicalQubit(q);

      // just continue, if a qubit is not found in both circuits
      if (found1 != found2) {
        continue;
      }

      const auto isIdle1 = found1 && qc1.isIdleQubit(*physical1);
      const auto isIdle2 = found2 && qc2.isIdleQubit(*physical2);

      // if an ancillary qubit is acted on in both circuits,
      // the alternating checker cannot be used.
      if (!isIdle1 && !isIdle2) {
        return false;
      }
    }
  }
  return true;
}

DDAlternatingChecker::DDAlternatingChecker(const qc::QuantumComputation& circ1,
                                           const qc::QuantumComputation& circ2,
                                           Configuration config)
    : DDEquivalenceChecker(circ1, circ2, std::move(config),
                           AlternatingDDPackageConfig{}) {
  // gates from the second circuit shall be applied "from the right"
  taskManager2.flipDirection();

  initializeApplicationScheme(configuration.application.alternatingScheme);

  // special treatment for the lookahead application scheme
  if (auto* lookahead =
          dynamic_cast<LookaheadApplicationScheme*>(applicationScheme.get())) {
    // initialize links for the internal state and the package of the
    // lookahead scheme
    lookahead->setInternalState(functionality);
    lookahead->setPackage(dd.get());
  }
}

void DDAlternatingChecker::json(nlohmann::basic_json<>& j) const noexcept {
  DDEquivalenceChecker::json(j);
  j["checker"] = "decision_diagram_alternating";
}

} // namespace ec
