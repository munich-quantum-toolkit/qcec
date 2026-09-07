/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "checker/dd/DDHybridSchrodingerFeynmanChecker.hpp"

#include "Configuration.hpp"
#include "EquivalenceCriterion.hpp"
#include "checker/EquivalenceChecker.hpp"
#include "dd/ComplexValue.hpp"
#include "dd/DDDefinitions.hpp"
#include "dd/Operations.hpp"
#include "dd/Package.hpp"
#include "ir/Definitions.hpp"
#include "ir/Permutation.hpp"
#include "ir/QuantumComputation.hpp"
#include "ir/operations/Control.hpp"
#include "ir/operations/OpType.hpp"
#include "ir/operations/Operation.hpp"
#include "ir/operations/StandardOperation.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace ec {
namespace {
constexpr dd::GateMatrix ZERO_PROJECTOR{1, 0, 0, 0};
constexpr dd::GateMatrix ONE_PROJECTOR{0, 0, 0, 1};

[[nodiscard]] bool isIdentityPermutation(const qc::Permutation& permutation,
                                         const std::size_t nqubits) noexcept {
  if (permutation.size() != nqubits) {
    return false;
  }
  return std::ranges::all_of(permutation, [](const auto& entry) {
    return entry.first == entry.second;
  });
}
} // namespace

class DDHybridSchrodingerFeynmanChecker::Slice {
private:
  [[nodiscard]] bool getNextControl() {
    if (nextControlIdx >= MAX_DECISIONS) {
      throw std::overflow_error(
          "The HSF checker supports at most 63 split operations.");
    }
    const auto decision =
        ((controlIdx >> nextControlIdx) & std::uint64_t{1}) != 0U;
    ++nextControlIdx;
    return decision;
  }

  qc::Qubit start;
  qc::Qubit end;
  std::uint64_t controlIdx;
  std::uint64_t nextControlIdx = 0U;

public:
  explicit Slice(DDPackage& dd, const qc::Qubit startQ, const qc::Qubit endQ,
                 const std::uint64_t controlQ)
      : start(startQ), end(endQ), controlIdx(controlQ),
        nqubits(end - start + 1), matrix(DDPackage::makeIdent()) {
    dd.incRef(matrix);
  }

  bool apply(DDPackage& sliceDD, const qc::Operation& op);

  qc::Qubit nqubits;
  dd::MatrixDD matrix{};
};

DDHybridSchrodingerFeynmanChecker::DDHybridSchrodingerFeynmanChecker(
    const qc::QuantumComputation& circ1, const qc::QuantumComputation& circ2,
    ec::Configuration config)
    : EquivalenceChecker(circ1, circ2, std::move(config)), invertedQc2(circ2) {
  if (!configuration.functionality.checkApproximateEquivalence) {
    throw std::invalid_argument(
        "The HSF checker requires approximate equivalence checking.");
  }
  if (configuration.functionality.checkPartialEquivalence) {
    throw std::invalid_argument(
        "The HSF checker does not support partial equivalence checking.");
  }
  const auto threshold =
      configuration.functionality.approximateCheckingThreshold;
  if (!std::isfinite(threshold) || threshold < 0. || threshold > 1.) {
    throw std::invalid_argument(
        "The approximate equivalence checking threshold must be finite and "
        "within [0, 1].");
  }
  if (!std::isfinite(configuration.functionality.traceThreshold) ||
      configuration.functionality.traceThreshold < 0.) {
    throw std::invalid_argument(
        "The exact trace threshold must be finite and non-negative.");
  }

  nDecisions = validate(circ1, circ2);

  splitQubit = static_cast<qc::Qubit>(circ1.getNqubits() / 2U);
  globalPhaseDifference = circ1.getGlobalPhase() - circ2.getGlobalPhase();
  invertedQc2.invert();
  qc2 = &invertedQc2;
}

std::size_t DDHybridSchrodingerFeynmanChecker::countDecisions(
    const qc::QuantumComputation& circuit) {
  if (circuit.getNqubits() < 2U) {
    throw std::invalid_argument(
        "The HSF checker requires circuits with at least two qubits.");
  }
  if (circuit.getNancillae() > 0U || circuit.getNgarbageQubits() > 0U) {
    throw std::invalid_argument(
        "The HSF checker does not support ancillary or garbage qubits.");
  }
  if (!isIdentityPermutation(circuit.initialLayout, circuit.getNqubits()) ||
      !isIdentityPermutation(circuit.outputPermutation, circuit.getNqubits())) {
    throw std::invalid_argument(
        "The HSF checker does not support non-identity initial layouts or "
        "output permutations.");
  }

  std::size_t ndecisions = 0;
  const auto splitQubit = static_cast<qc::Qubit>(circuit.getNqubits() / 2U);
  // calculate number of decisions
  for (const auto& op : circuit) {
    if (op->getType() == qc::Barrier) {
      continue;
    }
    if (!op->isUnitary()) {
      throw std::invalid_argument(
          "The HSF checker only supports unitary operations.");
    }
    if (!op->isStandardOperation()) {
      throw std::invalid_argument(
          "The HSF checker only supports standard operations.");
    }

    bool targetInLowerSlice = false;
    bool targetInUpperSlice = false;
    std::size_t nControlsInLowerSlice = 0U;
    std::size_t nControlsInUpperSlice = 0U;
    for (const auto& target : op->getTargets()) {
      targetInLowerSlice = targetInLowerSlice || target < splitQubit;
      targetInUpperSlice = targetInUpperSlice || target >= splitQubit;
    }
    for (const auto& control : op->getControls()) {
      if (control.qubit < splitQubit) {
        nControlsInLowerSlice++;
      } else {
        nControlsInUpperSlice++;
      }
    }

    if (!targetInLowerSlice && !targetInUpperSlice) {
      throw std::invalid_argument(
          "The HSF checker does not support operations without targets.");
    }

    if (targetInLowerSlice && targetInUpperSlice) {
      throw std::invalid_argument(
          "Multiple targets spread across the cut through the circuit are not "
          "supported at the moment as this would require actually computing "
          "the Schmidt decomposition of the gate being cut.");
    }

    if (targetInLowerSlice && nControlsInUpperSlice > 0U) {
      if (nControlsInUpperSlice > 1) {
        throw std::invalid_argument(
            "Multiple controls in the control part of the gate being cut are "
            "not supported at the moment as this would require actually "
            "computing the Schmidt decomposition of the gate being cut.");
      }
      ++ndecisions;
    } else if (targetInUpperSlice && nControlsInLowerSlice > 0U) {
      if (nControlsInLowerSlice > 1) {
        throw std::invalid_argument(
            "Multiple controls in the control part of the gate being cut are "
            "not supported at the moment as this would require actually "
            "computing the Schmidt decomposition of the gate being cut.");
      }
      ++ndecisions;
    }
  }
  return ndecisions;
}

std::size_t
DDHybridSchrodingerFeynmanChecker::validate(const qc::QuantumComputation& qc1,
                                            const qc::QuantumComputation& qc2) {
  if (qc1.getNqubits() != qc2.getNqubits()) {
    throw std::invalid_argument(
        "The HSF checker requires circuits with the same number of qubits.");
  }

  const auto firstDecisions = countDecisions(qc1);
  const auto secondDecisions = countDecisions(qc2);
  if (firstDecisions > MAX_DECISIONS ||
      secondDecisions > MAX_DECISIONS - firstDecisions) {
    throw std::overflow_error(
        "The HSF checker supports at most 63 split operations.");
  }
  return firstDecisions + secondDecisions;
}

bool DDHybridSchrodingerFeynmanChecker::canHandle(
    const qc::QuantumComputation& qc1, const qc::QuantumComputation& qc2) {
  try {
    static_cast<void>(validate(qc1, qc2));
    return true;
  } catch (const std::exception&) {
    return false;
  }
}

std::optional<dd::ComplexValue>
DDHybridSchrodingerFeynmanChecker::simulateSlicing(DDPackage& sliceDD,
                                                   const std::uint64_t i) {
  if (isDone()) {
    return std::nullopt;
  }
  Slice lower(sliceDD, 0, splitQubit - 1, i);
  Slice upper(sliceDD, splitQubit,
              static_cast<qc::Qubit>(this->qc1->getNqubits() - 1), i);
  for (const auto& op : *qc1) {
    if (isDone()) {
      return std::nullopt;
    }
    applyLowerUpper(sliceDD, *op, lower, upper);
  }
  for (const auto& op : *qc2) {
    if (isDone()) {
      return std::nullopt;
    }
    applyLowerUpper(sliceDD, *op, lower, upper);
  }
  if (isDone()) {
    return std::nullopt;
  }
  auto traceLower = sliceDD.trace(lower.matrix, lower.nqubits);
  auto traceUpper = sliceDD.trace(upper.matrix, upper.nqubits);
  if (isDone()) {
    return std::nullopt;
  }
  return traceLower * traceUpper;
}

void DDHybridSchrodingerFeynmanChecker::applyLowerUpper(DDPackage& sliceDD,
                                                        const qc::Operation& op,
                                                        Slice& lower,
                                                        Slice& upper) {
  if (op.getType() == qc::Barrier) {
    return;
  }
  const auto lowerIsSplit = lower.apply(sliceDD, op);
  const auto upperIsSplit = upper.apply(sliceDD, op);
  if (lowerIsSplit != upperIsSplit) {
    throw std::logic_error(
        "Inconsistent HSF decomposition between circuit slices.");
  }
  sliceDD.garbageCollect();
}

bool DDHybridSchrodingerFeynmanChecker::Slice::apply(DDPackage& sliceDD,
                                                     const qc::Operation& op) {
  bool isSplitOp = false;
  qc::Targets opTargets{};
  qc::Controls opControls{};

  // check targets
  bool targetInSplit = false;
  bool targetInOtherSplit = false;
  for (const auto& target : op.getTargets()) {
    if (start <= target && target <= end) {
      opTargets.emplace_back(target - start);
      targetInSplit = true;
    } else {
      targetInOtherSplit = true;
    }
  }

  if (targetInSplit && targetInOtherSplit) {
    throw std::invalid_argument(
        "The HSF checker does not support operation targets that cross the "
        "circuit cut.");
  }

  // check controls
  for (const auto& control : op.getControls()) {
    if (start <= control.qubit && control.qubit <= end) {
      opControls.emplace(control.qubit - start, control.type);
    } else { // other controls are set to the corresponding value
      if (targetInSplit) {
        isSplitOp = true;
        const bool nextControl = getNextControl();
        // break if control is not activated
        if ((control.type == qc::Control::Type::Pos && !nextControl) ||
            (control.type == qc::Control::Type::Neg && nextControl)) {
          return true;
        }
      }
    }
  }

  if (targetInOtherSplit && !opControls.empty()) { // control slice for split
    if (opControls.size() != 1U) {
      throw std::invalid_argument(
          "The HSF checker supports only one cross-cut control per split "
          "operation.");
    }

    isSplitOp = true;
    const bool control = getNextControl();
    for (const auto& c : opControls) {
      auto tmp = matrix;
      // The summand selects the physical control state. Gate polarity only
      // determines which summand applies the target operation; it must not
      // swap the projectors on the control slice.
      auto projMatrix = control ? sliceDD.makeGateDD(ONE_PROJECTOR, c.qubit)
                                : sliceDD.makeGateDD(ZERO_PROJECTOR, c.qubit);
      matrix = sliceDD.multiply(projMatrix, matrix);
      sliceDD.incRef(matrix);
      sliceDD.decRef(tmp);
    }
  } else if (targetInSplit) { // target slice for split or operation in split
    const auto& param = op.getParameter();
    const qc::StandardOperation newOp(opControls, opTargets, op.getType(),
                                      param);
    auto tmp = matrix;
    matrix = sliceDD.multiply(dd::getDD(newOp, sliceDD), matrix);
    sliceDD.incRef(matrix);
    sliceDD.decRef(tmp);
  }
  return isSplitOp;
}

EquivalenceCriterion DDHybridSchrodingerFeynmanChecker::run() {
  const auto start = std::chrono::steady_clock::now();
  equivalence = checkEquivalence();
  const auto end = std::chrono::steady_clock::now();
  runtime += std::chrono::duration<double>(end - start).count();
  return equivalence;
}

EquivalenceCriterion DDHybridSchrodingerFeynmanChecker::checkEquivalence() {
  if (isDone()) {
    return EquivalenceCriterion::NoInformation;
  }

  const auto maxControl = std::uint64_t{1} << nDecisions;
  const auto requestedThreads =
      configuration.execution.parallel
          ? std::max<std::size_t>(1U, configuration.execution.nthreads)
          : 1U;
  const auto workerCount = static_cast<std::size_t>(
      std::min<std::uint64_t>(maxControl, requestedThreads));

  std::atomic<std::uint64_t> nextControl{0U};
  std::atomic<bool> workerFailed{false};
  std::exception_ptr workerException{};
  std::mutex exceptionMutex{};
  std::vector<dd::ComplexValue> partialTraces(workerCount);

  {
    std::vector<std::jthread> workers{};
    workers.reserve(workerCount);
    for (std::size_t worker = 0U; worker < workerCount; ++worker) {
      workers.emplace_back([this, worker, maxControl, &nextControl,
                            &workerFailed, &workerException, &exceptionMutex,
                            &partialTraces]() {
        try {
          dd::ComplexValue localTrace{};
          const auto maxSliceQubits = std::max<std::size_t>(
              splitQubit, this->qc1->getNqubits() - splitQubit);
          auto sliceDD = std::make_unique<DDPackage>(maxSliceQubits);
          while (!isDone() && !workerFailed.load(std::memory_order_relaxed)) {
            const auto control =
                nextControl.fetch_add(1U, std::memory_order_relaxed);
            if (control >= maxControl) {
              break;
            }

            const auto result = simulateSlicing(*sliceDD, control);
            if (!result.has_value()) {
              break;
            }
            localTrace += *result;
            sliceDD->reset();
          }
          partialTraces.at(worker) = localTrace;
        } catch (...) {
          {
            const std::scoped_lock lock(exceptionMutex);
            if (!workerException) {
              workerException = std::current_exception();
            }
          }
          workerFailed.store(true, std::memory_order_relaxed);
        }
      });
    }
  }

  if (isDone()) {
    return EquivalenceCriterion::NoInformation;
  }
  if (workerException) {
    std::rethrow_exception(workerException);
  }

  dd::ComplexValue trace{};
  for (const auto& partialTrace : partialTraces) {
    trace += partialTrace;
  }
  const auto exactThreshold = configuration.functionality.traceThreshold;
  // MQT Core returns the trace normalized by the matrix dimension. Clamp small
  // floating-point excursions before evaluating the projective
  // Hilbert--Schmidt distance D_HS^2 = 1 - |Tr(U V^dagger) / d|^2. Evaluate
  // this phase-invariant quantity before restoring the circuit global phase.
  const auto normalizedOverlapSquared = std::clamp(trace.mag2(), 0., 1.);
  const auto distanceSquared = 1. - normalizedOverlapSquared;
  trace = trace * dd::ComplexValue{std::cos(globalPhaseDifference),
                                   std::sin(globalPhaseDifference)};
  const auto differenceToOneSquared =
      ((trace.r - 1.) * (trace.r - 1.)) + (trace.i * trace.i);
  const auto approximateThreshold =
      configuration.functionality.approximateCheckingThreshold;
  const auto exactlyEquivalent =
      differenceToOneSquared <= exactThreshold * exactThreshold;
  const auto equivalentUpToGlobalPhase =
      distanceSquared <= exactThreshold * exactThreshold;
  const auto approximatelyEquivalent =
      distanceSquared <= approximateThreshold * approximateThreshold;
  auto result = EquivalenceCriterion::NotEquivalent;
  if (exactlyEquivalent ||
      (!equivalentUpToGlobalPhase && approximatelyEquivalent)) {
    result = EquivalenceCriterion::Equivalent;
  } else if (equivalentUpToGlobalPhase) {
    result = EquivalenceCriterion::EquivalentUpToGlobalPhase;
  }
  return isDone() ? EquivalenceCriterion::NoInformation : result;
}

void DDHybridSchrodingerFeynmanChecker::json(
    nlohmann::basic_json<>& j) const noexcept {
  EquivalenceChecker::json(j);
  j["checker"] = "decision_diagram_hybrid_schrodinger_feynman";
}
} // namespace ec
