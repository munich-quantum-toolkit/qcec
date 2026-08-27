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
#include "dd/ComplexValue.hpp"
#include "dd/DDpackageConfig.hpp"
#include "dd/Package.hpp"
#include "dd/Package_fwd.hpp"
#include "ir/Definitions.hpp"
#include "ir/QuantumComputation.hpp"
#include "nlohmann/json_fwd.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>

namespace ec {
/**
 * @brief Approximate Equivalence Checking with the
 * DDHybridSchrodingerFeynmanChecker This checker divides a circuit horizontally
 * into two halves: a lower part and an upper part. This is achieved by
 * decomposing controlled gates, acting across both halves, according to the
 * Schmidt decomposition. By leveraging key trace equalities - specifically,
 *
 * tr[L ⊗ U] = tr[L] ⋅ tr[U]
 *
 * and
 *
 * tr[A + B] = tr[A] + tr[B],
 *
 * we can treat the lower and upper circuit parts, as well as the summands from
 * the Schmidt decomposition, independently. This enables parallel trace
 * computation, allowing to check the equivalence of larger, yet shallow
 * circuits.
 * @note Only suitable for shallow circuits with a maximum number of 63
 * controlled gates acting on both circuit parts (decisions).
 */
class DDHybridSchrodingerFeynmanChecker final : public EquivalenceChecker {
public:
  DDHybridSchrodingerFeynmanChecker(const qc::QuantumComputation& circ1,
                                    const qc::QuantumComputation& circ2,
                                    ec::Configuration config);

  EquivalenceCriterion run() override;

  void json(nlohmann::json& j) const noexcept override;

  /**
   * @brief Get the number of decisions for the fixed horizontal cut, where the
   * lower slice is `0 <= i < splitQubit` and the upper slice is
   * `splitQubit <= i < nqubits`.
   * @details The number of decisions is determined by the number of controlled
   * gates that operate across both halves.
   * @param qc
   * @return std::size_t
   */
  [[nodiscard]] static std::size_t
  getNDecisions(const qc::QuantumComputation& qc);

  /**
   * @brief Check whether the HSF checker can handle the given circuits.
   *
   * The function returns `false` if any of the following conditions are met:
   * - The circuits contain multi-qubit gates that are not supported by the HSF
   * checker.
   * - The total number of decisions exceeds the maximum allowable limit of 63.
   *
   * @param qc1
   * @param qc2
   * @return `true` if both circuits can be handled by the HSF checker,
   * otherwise `false`.
   */
  [[nodiscard]] static bool canHandle(const qc::QuantumComputation& qc1,
                                      const qc::QuantumComputation& qc2);

private:
  qc::Qubit splitQubit{};
  std::unique_ptr<qc::QuantumComputation> invertedQc2{};
  double globalPhaseDifference{};

  using DDPackage = dd::Package;

  /**
   * @brief Computing the Frobenius inner product trace(U * V^-1) and comparing
   * it to the desired threshold.
   * @return The exact, global-phase, approximate, or non-equivalent result.
   */
  EquivalenceCriterion checkEquivalence();

  /**
   * @brief Computes the trace for the i-th summand after applying the Schmidt
   * decomposition for all control decisions.
   *
   * @details The Schmidt decomposition allows decomposing a controlled gate
   * into a sum of circuits, each consisting of only single-qubit gates. By
   * recursively applying this decomposition to all decisions, we generate a
   * total of 2^decisions circuits, which do not contain controlled operations
   * acting on both halves. This enables independent investigation of the
   * lower and upper circuit parts.
   *
   * This function computes the trace for the i-th summand, where the index 'i'
   * determines, for each gate, whether the 0 or 1 projection is considered. See
   * getNextControl() and apply() for more details on how these projections are
   * managed.
   *
   * @param sliceDD1 Decision diagram for the lower circuit part.
   * @param sliceDD2 Decision diagram for the upper circuit part.
   * @param i Index of the summand for which the trace is computed.
   * @return The trace value, or `std::nullopt` if checking was canceled.
   */
  [[nodiscard]] std::optional<dd::ComplexValue>
  simulateSlicing(std::unique_ptr<DDPackage>& sliceDD1,
                  std::unique_ptr<DDPackage>& sliceDD2, std::uint64_t i);

  class Slice;

  /**
   * @brief Applies a single operation to the lower and upper circuit parts
   * according to the Schmidt decomposition, for the summand specified by
   * `controlIdx` from the class Slice.
   *
   * @param sliceDD1 Decision diagram for the lower circuit part.
   * @param sliceDD2 Decision diagram for the upper circuit part.
   * @param op Current operation to be applied
   * @param lower
   * @param upper
   */
  static void applyLowerUpper(std::unique_ptr<DDPackage>& sliceDD1,
                              std::unique_ptr<DDPackage>& sliceDD2,
                              const std::unique_ptr<qc::Operation>& op,
                              Slice& lower, Slice& upper) {
    if (op->getType() == qc::Barrier) {
      return;
    }
    if (!op->isUnitary() || !op->isStandardOperation()) {
      throw std::invalid_argument(
          "The HSF checker only supports unitary standard operations.");
    }
    const auto lowerIsSplit = lower.apply(sliceDD1, op);
    const auto upperIsSplit = upper.apply(sliceDD2, op);
    if (lowerIsSplit != upperIsSplit) {
      throw std::logic_error(
          "Inconsistent HSF decomposition between circuit slices.");
    }
    sliceDD1->garbageCollect();
    sliceDD2->garbageCollect();
  }

  class Slice {
  protected:
    std::uint64_t nextControlIdx = 0U;

    /**
     * @brief Determines how the current operation is decomposed for the summand
     * at index `controlIdx`.
     * @details nextControlIdx tracks the number of operations processed so far.
     * By comparing the shifted value to the bits of `controlIdx`, we can
     * determine how the current operation should be decomposed for the summand
     * at index `controlIdx`.
     * @return The next binary control decision.
     */
    bool getNextControl() {
      if (nextControlIdx >= 63U) {
        throw std::overflow_error(
            "The HSF checker supports at most 63 split operations.");
      }
      const auto decision =
          ((controlIdx >> nextControlIdx) & std::uint64_t{1}) != 0U;
      ++nextControlIdx;
      return decision;
    }

  public:
    qc::Qubit start;
    qc::Qubit end;
    std::uint64_t controlIdx;
    qc::Qubit nqubits;
    dd::MatrixDD matrix{};

    explicit Slice(std::unique_ptr<DDPackage>& dd, const qc::Qubit startQ,
                   const qc::Qubit endQ, const std::uint64_t controlQ)
        : start(startQ), end(endQ), controlIdx(controlQ),
          nqubits(end - start + 1), matrix(dd->makeIdent()) {
      dd->incRef(matrix);
    }

    /**
     * @brief Applies the decomposition of the current operation, based on the
     * summand index `controlIdx`, to the decision diagram of the specified
     * circuit slice.
     *
     * @param sliceDD Decision diagram for the lower or upper circuit part.
     * @param op
     * @return bool Returns true if the operation is a split operation, false
     * otherwise.
     */
    bool apply(std::unique_ptr<DDPackage>& sliceDD,
               const std::unique_ptr<qc::Operation>& op);
  };
};

} // namespace ec
