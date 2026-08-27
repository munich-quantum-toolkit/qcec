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
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <taskflow/core/async.hpp> // IWYU pragma: keep
#include <taskflow/core/executor.hpp>
#include <thread>

namespace ec {
std::size_t DDHybridSchrodingerFeynmanChecker::getNDecisions(
    const qc::QuantumComputation& qc) {
  std::size_t ndecisions = 0;
  // calculate number of decisions
  for (const auto& op : qc) {
    if (op->getType() == qc::Barrier) {
      continue;
    }
    assert(op->isStandardOperation());

    bool targetInLowerSlice = false;
    bool targetInUpperSlice = false;
    bool controlInLowerSlice = false;
    size_t nControlsInLowerSlice = 0;
    bool controlInUpperSlice = false;
    size_t nControlsInUpperSlice = 0;
    auto spQubit = static_cast<qc::Qubit>(qc.getNqubits() / 2);
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
    const auto ndecisions =
        getNDecisions(qc1) + getNDecisions(*invertCircuit(qc2));
    if (ndecisions > 63) {
      std::clog << "[QCEC] Warning: Number of split operations exceeds the "
                   "maximum allowed number: "
                << ndecisions << "\n";
      return false;
    }
    return true;
  } catch (const std::invalid_argument& e) {
    std::clog << "[QCEC] Warning: " << e.what() << "\n";
    return false;
  }
}

dd::ComplexValue DDHybridSchrodingerFeynmanChecker::simulateSlicing(
    std::unique_ptr<DDPackage>& sliceDD1, std::unique_ptr<DDPackage>& sliceDD2,
    size_t i) {
  Slice lower(sliceDD1, 0, splitQubit - 1, i);
  Slice upper(sliceDD2, splitQubit,
              static_cast<qc::Qubit>(this->qc1->getNqubits() - 1), i);
  for (const auto& op : *qc1) {
    applyLowerUpper(sliceDD1, sliceDD2, op, lower, upper);
  }
  for (const auto& op : *qc2) {
    applyLowerUpper(sliceDD1, sliceDD2, op, lower, upper);
  }
  auto traceLower = sliceDD1->trace(lower.matrix, lower.nqubits);
  auto traceUpper = sliceDD2->trace(upper.matrix, upper.nqubits);
  auto result = traceLower * traceUpper;
  return result;
}

bool DDHybridSchrodingerFeynmanChecker::Slice::apply(
    std::unique_ptr<DDPackage>& sliceDD,
    const std::unique_ptr<qc::Operation>& op) {
  bool isSplitOp = false;
  assert(op->isStandardOperation());
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

  // Ensured in the getNDecisions function
  assert(!(targetInSplit && targetInOtherSplit));

  // check controls
  for (const auto& control : op->getControls()) {
    if (start <= control.qubit && control.qubit <= end) {
      opControls.emplace(control.qubit - start, control.type);
    } else { // other controls are set to the corresponding value
      if (targetInSplit) {
        isSplitOp = true;
        const bool nextControl = getNextControl() != 0U;
        // break if control is not activated
        if ((control.type == qc::Control::Type::Pos && !nextControl) ||
            (control.type == qc::Control::Type::Neg && nextControl)) {
          nDecisionsExecuted++;
          return true;
        }
      }
    }
  }

  if (targetInOtherSplit && !opControls.empty()) { // control slice for split
    // Ensured in the getNDecisions function
    assert(opControls.size() == 1);

    isSplitOp = true;
    const bool control = getNextControl() != 0U;
    for (const auto& c : opControls) {
      auto tmp = matrix;
      auto projMatrix = control == (c.type == qc::Control::Type::Neg)
                            ? sliceDD->makeGateDD(dd::MEAS_ZERO_MAT, c.qubit)
                            : sliceDD->makeGateDD(dd::MEAS_ONE_MAT, c.qubit);
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
  if (isSplitOp) {
    nDecisionsExecuted++;
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
  if (ndecisions > 63) {
    throw std::overflow_error(
        "Number of split operations exceeds the maximum allowed number of 63.");
  }
  const auto maxControl = 1ULL << ndecisions;
  const std::size_t nthreads =
      std::max(2U, std::thread::hardware_concurrency());
  const auto actuallyUsedThreads = std::min<std::size_t>(maxControl, nthreads);
  const auto chunkSize = static_cast<std::size_t>(
      std::ceil(static_cast<double>(maxControl) /
                static_cast<double>(actuallyUsedThreads)));
  const auto nslicesOnOneCpu = std::min<std::size_t>(64, chunkSize);
  dd::ComplexValue trace{0.0, 0.0};
  std::mutex traceMutex;
  tf::Executor executor(actuallyUsedThreads);
  for (std::size_t control = 0; control < maxControl;
       control += nslicesOnOneCpu) {
    executor.silent_async([this, &trace, nslicesOnOneCpu, control, maxControl,
                           &traceMutex]() {
      for (std::size_t localControl = 0; localControl < nslicesOnOneCpu;
           localControl++) {
        const std::size_t totalControl = control + localControl;
        if (totalControl >= maxControl) {
          break;
        }
        std::unique_ptr<DDPackage> sliceDD1 =
            std::make_unique<DDPackage>(splitQubit);
        std::unique_ptr<DDPackage> sliceDD2 =
            std::make_unique<DDPackage>(this->qc1->getNqubits() - splitQubit);
        auto result = simulateSlicing(sliceDD1, sliceDD2, totalControl);
        const std::lock_guard<std::mutex> guard(traceMutex);
        trace += result;
      }
    });
  }
  executor.wait_for_all();
  if (std::abs(trace.mag() - 1.) <
      configuration.functionality.approximateCheckingThreshold) {
    return EquivalenceCriterion::Equivalent;
  }
  return EquivalenceCriterion::NotEquivalent;
}

void DDHybridSchrodingerFeynmanChecker::json(
    nlohmann::basic_json<>& j) const noexcept {
  EquivalenceChecker::json(j);
  j["checker"] = "hsf";
}
} // namespace ec
