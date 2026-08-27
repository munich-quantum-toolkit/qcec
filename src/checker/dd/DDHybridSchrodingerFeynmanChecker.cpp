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

#include "EquivalenceCriterion.hpp"
#include "checker/EquivalenceChecker.hpp"
#include "dd/ComplexValue.hpp"
#include "dd/GateMatrixDefinitions.hpp"
#include "dd/Operations.hpp"
#include "dd/Package.hpp"
#include "ir/Definitions.hpp"
#include "ir/QuantumComputation.hpp"
#include "ir/operations/Control.hpp"
#include "ir/operations/OpType.hpp"
#include "ir/operations/StandardOperation.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace ec {
namespace {
constexpr std::size_t MAX_DECISIONS = 63U;

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

DDHybridSchrodingerFeynmanChecker::DDHybridSchrodingerFeynmanChecker(
    const qc::QuantumComputation& circ1, const qc::QuantumComputation& circ2,
    ec::Configuration config)
    : EquivalenceChecker(circ1, circ2, std::move(config)) {
  if (circ1.getNqubits() != circ2.getNqubits()) {
    throw std::invalid_argument(
        "The HSF checker requires circuits with the same number of qubits.");
  }
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

  const auto ndecisions = getNDecisions(circ1) + getNDecisions(circ2);
  if (ndecisions > MAX_DECISIONS) {
    throw std::overflow_error(
        "The HSF checker supports at most 63 split operations.");
  }

  splitQubit = static_cast<qc::Qubit>(circ1.getNqubits() / 2U);
  globalPhaseDifference = circ1.getGlobalPhase() - circ2.getGlobalPhase();
  invertedQc2 = std::make_unique<qc::QuantumComputation>(circ2);
  invertedQc2->invert();
  qc2 = invertedQc2.get();
}

std::size_t DDHybridSchrodingerFeynmanChecker::getNDecisions(
    const qc::QuantumComputation& qc) {
  if (qc.getNqubits() < 2U) {
    throw std::invalid_argument(
        "The HSF checker requires circuits with at least two qubits.");
  }
  if (qc.getNancillae() > 0U || qc.getNgarbageQubits() > 0U) {
    throw std::invalid_argument(
        "The HSF checker does not support ancillary or garbage qubits.");
  }
  if (!isIdentityPermutation(qc.initialLayout, qc.getNqubits()) ||
      !isIdentityPermutation(qc.outputPermutation, qc.getNqubits())) {
    throw std::invalid_argument(
        "The HSF checker does not support non-identity initial layouts or "
        "output permutations.");
  }

  std::size_t ndecisions = 0;
  // calculate number of decisions
  for (const auto& op : qc) {
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
    bool controlInLowerSlice = false;
    std::size_t nControlsInLowerSlice = 0U;
    bool controlInUpperSlice = false;
    std::size_t nControlsInUpperSlice = 0U;
    const auto spQubit = static_cast<qc::Qubit>(qc.getNqubits() / 2U);
    for (const auto& target : op->getTargets()) {
      targetInLowerSlice = targetInLowerSlice || target < spQubit;
      targetInUpperSlice = targetInUpperSlice || target >= spQubit;
    }
    for (const auto& control : op->getControls()) {
      if (control.qubit < spQubit) {
        controlInLowerSlice = true;
        nControlsInLowerSlice++;
      } else {
        controlInUpperSlice = true;
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

    if (targetInLowerSlice && controlInUpperSlice) {
      if (nControlsInUpperSlice > 1) {
        throw std::invalid_argument(
            "Multiple controls in the control part of the gate being cut are "
            "not supported at the moment as this would require actually "
            "computing the Schmidt decomposition of the gate being cut.");
      }
      ++ndecisions;
    } else if (targetInUpperSlice && controlInLowerSlice) {
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

bool DDHybridSchrodingerFeynmanChecker::canHandle(
    const qc::QuantumComputation& qc1, const qc::QuantumComputation& qc2) {
  try {
    if (qc1.getNqubits() != qc2.getNqubits()) {
      throw std::invalid_argument(
          "The HSF checker requires circuits with the same number of "
          "qubits.");
    }
    const auto ndecisions = getNDecisions(qc1) + getNDecisions(qc2);
    if (ndecisions > MAX_DECISIONS) {
      std::clog << "[QCEC] Warning: Number of split operations exceeds the "
                   "maximum allowed number: "
                << ndecisions << "\n";
      return false;
    }
    return true;
  } catch (const std::exception& e) {
    std::clog << "[QCEC] Warning: " << e.what() << "\n";
    return false;
  }
}

std::optional<dd::ComplexValue>
DDHybridSchrodingerFeynmanChecker::simulateSlicing(
    std::unique_ptr<DDPackage>& sliceDD1, std::unique_ptr<DDPackage>& sliceDD2,
    const std::uint64_t i) {
  if (isDone()) {
    return std::nullopt;
  }
  Slice lower(sliceDD1, 0, splitQubit - 1, i);
  Slice upper(sliceDD2, splitQubit,
              static_cast<qc::Qubit>(this->qc1->getNqubits() - 1), i);
  for (const auto& op : *qc1) {
    if (isDone()) {
      return std::nullopt;
    }
    applyLowerUpper(sliceDD1, sliceDD2, op, lower, upper);
  }
  for (const auto& op : *qc2) {
    if (isDone()) {
      return std::nullopt;
    }
    applyLowerUpper(sliceDD1, sliceDD2, op, lower, upper);
  }
  if (isDone()) {
    return std::nullopt;
  }
  auto traceLower = sliceDD1->trace(lower.matrix, lower.nqubits);
  auto traceUpper = sliceDD2->trace(upper.matrix, upper.nqubits);
  if (isDone()) {
    return std::nullopt;
  }
  return traceLower * traceUpper;
}

bool DDHybridSchrodingerFeynmanChecker::Slice::apply(
    std::unique_ptr<DDPackage>& sliceDD,
    const std::unique_ptr<qc::Operation>& op) {
  bool isSplitOp = false;
  if (!op->isUnitary() || !op->isStandardOperation()) {
    throw std::invalid_argument(
        "The HSF checker only supports unitary standard operations.");
  }
  qc::Targets opTargets{};
  qc::Controls opControls{};

  // check targets
  bool targetInSplit = false;
  bool targetInOtherSplit = false;
  for (const auto& target : op->getTargets()) {
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
  for (const auto& control : op->getControls()) {
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
      auto projMatrix = control
                            ? sliceDD->makeGateDD(dd::MEAS_ONE_MAT, c.qubit)
                            : sliceDD->makeGateDD(dd::MEAS_ZERO_MAT, c.qubit);
      matrix = sliceDD->multiply(projMatrix, matrix);
      sliceDD->incRef(matrix);
      sliceDD->decRef(tmp);
    }
  } else if (targetInSplit) { // target slice for split or operation in split
    const auto& param = op->getParameter();
    qc::StandardOperation newOp(opControls, opTargets, op->getType(), param);
    auto tmp = matrix;
    matrix = sliceDD->multiply(dd::getDD(newOp, *sliceDD), matrix);
    sliceDD->incRef(matrix);
    sliceDD->decRef(tmp);
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
  const auto ndecisions = getNDecisions(*qc1) + getNDecisions(*qc2);
  if (ndecisions > MAX_DECISIONS) {
    throw std::overflow_error(
        "Number of split operations exceeds the maximum allowed number of 63.");
  }
  if (isDone()) {
    return EquivalenceCriterion::NoInformation;
  }

  const auto maxControl = std::uint64_t{1} << ndecisions;
  const auto requestedThreads =
      std::max<std::size_t>(1U, configuration.execution.nthreads);
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
          while (!isDone() && !workerFailed.load(std::memory_order_relaxed)) {
            const auto control =
                nextControl.fetch_add(1U, std::memory_order_relaxed);
            if (control >= maxControl) {
              break;
            }

            auto sliceDD1 = std::make_unique<DDPackage>(splitQubit);
            auto sliceDD2 = std::make_unique<DDPackage>(
                this->qc1->getNqubits() - splitQubit);
            const auto result = simulateSlicing(sliceDD1, sliceDD2, control);
            if (!result.has_value()) {
              break;
            }
            localTrace += *result;
          }
          partialTraces[worker] = localTrace;
        } catch (...) {
          {
            const std::lock_guard<std::mutex> lock(exceptionMutex);
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
  trace = trace * dd::ComplexValue{std::cos(globalPhaseDifference),
                                   std::sin(globalPhaseDifference)};

  const auto exactThreshold = configuration.functionality.traceThreshold;
  const auto differenceToOneSquared =
      ((trace.r - 1.) * (trace.r - 1.)) + (trace.i * trace.i);
  // MQT Core returns the trace normalized by the matrix dimension. Clamp small
  // floating-point excursions before evaluating the projective
  // Hilbert--Schmidt distance D_HS^2 = 1 - |Tr(U V^dagger) / d|^2.
  const auto normalizedOverlapSquared = std::clamp(trace.mag2(), 0., 1.);
  const auto distanceSquared = 1. - normalizedOverlapSquared;
  const auto approximateThreshold =
      configuration.functionality.approximateCheckingThreshold;
  auto result = EquivalenceCriterion::NotEquivalent;
  if (differenceToOneSquared <= exactThreshold * exactThreshold) {
    result = EquivalenceCriterion::Equivalent;
  } else if (distanceSquared <= exactThreshold * exactThreshold) {
    result = EquivalenceCriterion::EquivalentUpToGlobalPhase;
  } else if (distanceSquared <= approximateThreshold * approximateThreshold) {
    result = EquivalenceCriterion::Equivalent;
  }
  return isDone() ? EquivalenceCriterion::NoInformation : result;
}

void DDHybridSchrodingerFeynmanChecker::json(
    nlohmann::basic_json<>& j) const noexcept {
  EquivalenceChecker::json(j);
  j["checker"] = "hsf";
}
} // namespace ec
