/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "checker/dd/DDEquivalenceChecker.hpp"

#include "EquivalenceCriterion.hpp"
#include "checker/dd/TaskManager.hpp"
#include "checker/dd/applicationscheme/ApplicationScheme.hpp"
#include "checker/dd/applicationscheme/GateCostApplicationScheme.hpp"
#include "checker/dd/applicationscheme/LookaheadApplicationScheme.hpp"
#include "checker/dd/applicationscheme/OneToOneApplicationScheme.hpp"
#include "checker/dd/applicationscheme/ProportionalApplicationScheme.hpp"
#include "checker/dd/applicationscheme/SequentialApplicationScheme.hpp"
#include "dd/Node.hpp"

#include <chrono>
#include <stdexcept>

namespace ec {

template <class DDType>
EquivalenceCriterion DDEquivalenceChecker<DDType>::equals(const DDType& e,
                                                          const DDType& f) {
  // both node pointers being equivalent is the strongest indication that the
  // two decision diagrams are equivalent
  if (e.p == f.p) {
    // whenever the top edge weights differ, both decision diagrams are only
    // equivalent up to a global phase
    if (!e.w.approximatelyEquals(f.w)) {
      if constexpr (std::is_same_v<DDType, dd::MatrixDD>) {
        return EquivalenceCriterion::EquivalentUpToGlobalPhase;
      } else {
        return EquivalenceCriterion::EquivalentUpToPhase;
      }
    }
    return EquivalenceCriterion::Equivalent;
  }

  // in general, decision diagrams are canonic. This implies that if their top
  // nodes differ, they are not equivalent. However, numerical instabilities
  // might create a scenario where two nodes differ besides their underlying
  // decision diagrams being extremely close (for some definition of `close`).
  if constexpr (std::is_same_v<DDType, dd::MatrixDD>) {
    // for matrices this can be resolved by calculating their Frobenius inner
    // product trace(U V^-1) and comparing it to some threshold. in a similar
    // fashion, we can simply compare U V^-1 with the identity, which results in
    // a much simpler check that is not prone to overflow.
    bool isClose{};
    const bool eIsClose =
        dd->isCloseToIdentity(e, configuration.functionality.traceThreshold);
    const bool fIsClose =
        dd->isCloseToIdentity(f, configuration.functionality.traceThreshold);
    if (eIsClose || fIsClose) {
      // if any of the DDs is close to the identity (structure), the result can
      // be decided by whether both DDs are close enough to the identity.
      isClose = eIsClose && fIsClose;
    } else {
      // otherwise, one DD needs to be inverted before multiplying both of them
      // together and checking whether the resulting DD is close enough to the
      // identity.
      auto g = dd->multiply(e, dd->conjugateTranspose(f));
      isClose =
          dd->isCloseToIdentity(g, configuration.functionality.traceThreshold);
    }

    if (isClose) {
      // whenever the top edge weights differ, both decision diagrams are only
      // equivalent up to a global phase
      if (!e.w.approximatelyEquals(f.w)) {
        return EquivalenceCriterion::EquivalentUpToGlobalPhase;
      }
      return EquivalenceCriterion::Equivalent;
    }
  } else {
    // for vectors this is resolved by computing the inner product (or fidelity)
    // between both decision diagrams and comparing it to some threshold
    const auto innerProduct = dd->innerProduct(e, f);

    // whenever <e,f> ≃ 1, both decision diagrams should be considered
    // equivalent
    if (std::abs(innerProduct.r - 1.) <
        configuration.simulation.fidelityThreshold) {
      return EquivalenceCriterion::Equivalent;
    }

    // whenever |<e,f>|^2 ≃ 1, both decision diagrams should be considered
    // equivalent up to a phase
    const auto fidelity =
        (innerProduct.r * innerProduct.r) + (innerProduct.i * innerProduct.i);
    if (std::abs(fidelity - 1.0) < configuration.simulation.fidelityThreshold) {
      return EquivalenceCriterion::EquivalentUpToPhase;
    }
  }

  return EquivalenceCriterion::NotEquivalent;
}

template <class DDType>
EquivalenceCriterion DDEquivalenceChecker<DDType>::run() {
  const auto start = std::chrono::steady_clock::now();

  // initialize the internal representation (initial state, initial matrix,
  // etc.)
  initialize();

  // execute the equivalence checking scheme
  execute();

  // finish off both circuits
  finish();

  // postprocess the result
  postprocess();

  if (isDone()) {
    return equivalence;
  }

  // check the equivalence
  equivalence = checkEquivalence();

  const auto end = std::chrono::steady_clock::now();
  runtime += std::chrono::duration<double>(end - start).count();

  return equivalence;
}

template <class DDType>
void DDEquivalenceChecker<DDType>::initializeTask(
    TaskManager<DDType>& taskManager) {
  taskManager.reset();
}

template <class DDType> void DDEquivalenceChecker<DDType>::initialize() {
  initializeTask(taskManager1);
  initializeTask(taskManager2);
}

template <class DDType> void DDEquivalenceChecker<DDType>::execute() {
  while (!taskManager1.finished() && !taskManager2.finished() && !isDone()) {
    // skip over any SWAP operations
    taskManager1.applySwapOperations();
    taskManager2.applySwapOperations();

    if (!taskManager1.finished() && !taskManager2.finished() && !isDone()) {
      // query application scheme on how to proceed
      const auto [apply1, apply2] = (*applicationScheme)();

      // advance both tasks correspondingly
      if (!isDone()) {
        taskManager1.advance(apply1);
      }
      if (!isDone()) {
        taskManager2.advance(apply2);
      }
    }
  }
}

template <class DDType> void DDEquivalenceChecker<DDType>::finish() {
  if (!isDone()) {
    taskManager1.finish();
  }
  if (!isDone()) {
    taskManager2.finish();
  }
}

template <class DDType>
void DDEquivalenceChecker<DDType>::postprocessTask(TaskManager<DDType>& task) {
  // ensure that the permutation that was tracked throughout the circuit matches
  // the expected output permutation
  task.changePermutation();
  if (isDone()) {
    return;
  }
  // eliminate the superfluous contributions of ancillary qubits (this only has
  // an effect on matrices)
  task.reduceAncillae();
  if (isDone()) {
    return;
  }
  // sum up the contributions of garbage qubits if we want to check for partial
  // equivalence
  if (configuration.functionality.checkPartialEquivalence) {
    task.reduceGarbage();
  }
}

template <class DDType> void DDEquivalenceChecker<DDType>::postprocess() {
  if (!isDone()) {
    postprocessTask(taskManager1);
  }
  if (!isDone()) {
    postprocessTask(taskManager2);
  }
}

template <class DDType>
EquivalenceCriterion DDEquivalenceChecker<DDType>::checkEquivalence() {
  return equals(taskManager1.getInternalState(),
                taskManager2.getInternalState());
}

template <class DDType>
void DDEquivalenceChecker<DDType>::initializeApplicationScheme(
    const ApplicationSchemeType scheme) {
  switch (scheme) {
  case ApplicationSchemeType::Sequential:
    applicationScheme = std::make_unique<SequentialApplicationScheme<DDType>>(
        taskManager1, taskManager2);
    break;
  case ApplicationSchemeType::OneToOne:
    applicationScheme = std::make_unique<OneToOneApplicationScheme<DDType>>(
        taskManager1, taskManager2);
    break;
  case ApplicationSchemeType::Lookahead:
    if constexpr (std::is_same_v<DDType, dd::MatrixDD>) {
      applicationScheme = std::make_unique<LookaheadApplicationScheme>(
          taskManager1, taskManager2);
    } else {
      throw std::invalid_argument(
          "Lookahead application scheme can only be used for matrices.");
    }
    break;
  case ApplicationSchemeType::GateCost:
    if (!configuration.application.profile.empty()) {
      applicationScheme = std::make_unique<GateCostApplicationScheme<DDType>>(
          taskManager1, taskManager2, configuration.application.profile,
          configuration.optimizations.fuseSingleQubitGates);
    } else {
      applicationScheme = std::make_unique<GateCostApplicationScheme<DDType>>(
          taskManager1, taskManager2, configuration.application.costFunction,
          configuration.optimizations.fuseSingleQubitGates);
    }
    break;
  default:
    applicationScheme = std::make_unique<ProportionalApplicationScheme<DDType>>(
        taskManager1, taskManager2,
        configuration.optimizations.fuseSingleQubitGates);
    break;
  }
}

template class DDEquivalenceChecker<dd::VectorDD>;
template class DDEquivalenceChecker<dd::MatrixDD>;
} // namespace ec
