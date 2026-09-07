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
#include "dd/Package_fwd.hpp"
#include "ir/Definitions.hpp"
#include "ir/QuantumComputation.hpp"
#include "ir/operations/Operation.hpp"
#include "nlohmann/json_fwd.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace ec {
/**
 * @brief Check approximate equivalence with the hybrid
 * Schrödinger--Feynman method.
 *
 * The checker divides each circuit into a lower and an upper qubit slice. It
 * decomposes controlled gates that cross the cut into tensor-product summands.
 * The checker uses the trace identities
 *
 * tr[L ⊗ U] = tr[L] ⋅ tr[U]
 *
 * and
 *
 * tr[A + B] = tr[A] + tr[B],
 *
 * to evaluate each slice and summand independently. This method supports
 * parallel trace computation for large, shallow circuits.
 *
 * @note The checker supports at most 63 cross-cut controlled operations in
 * total across both circuits.
 */
class DDHybridSchrodingerFeynmanChecker final : public EquivalenceChecker {
public:
  DDHybridSchrodingerFeynmanChecker(const qc::QuantumComputation& circ1,
                                    const qc::QuantumComputation& circ2,
                                    ec::Configuration config);

  EquivalenceCriterion run() override;

  void json(nlohmann::json& j) const noexcept override;

  /**
   * @brief Validate that the HSF checker can handle a pair of circuits.
   *
   * @param qc1 First circuit.
   * @param qc2 Second circuit.
   * @return The total number of cross-cut decisions across both circuits.
   * @throws std::invalid_argument If the circuits violate an HSF input
   * requirement.
   * @throws std::overflow_error If the circuits require more decisions than
   * the implementation can represent.
   */
  [[nodiscard]] static std::size_t validate(const qc::QuantumComputation& qc1,
                                            const qc::QuantumComputation& qc2);

  /**
   * @brief Check whether the HSF checker can handle the given circuits.
   *
   * The function returns `false` if a circuit does not meet the input
   * requirements or if the circuits require more than 63 decisions in total.
   *
   * @param qc1
   * @param qc2
   * @return `true` if both circuits can be handled by the HSF checker,
   * otherwise `false`.
   */
  [[nodiscard]] static bool canHandle(const qc::QuantumComputation& qc1,
                                      const qc::QuantumComputation& qc2);

private:
  static constexpr std::size_t MAX_DECISIONS = 63U;

  class Slice;
  using DDPackage = dd::Package;

  qc::Qubit splitQubit{};
  std::size_t nDecisions{};
  qc::QuantumComputation invertedQc2;
  double globalPhaseDifference{};

  EquivalenceCriterion checkEquivalence();

  [[nodiscard]] static std::size_t
  countDecisions(const qc::QuantumComputation& circuit);

  [[nodiscard]] std::optional<dd::ComplexValue>
  simulateSlicing(DDPackage& sliceDD, std::uint64_t i);

  static void applyLowerUpper(DDPackage& sliceDD, const qc::Operation& op,
                              Slice& lower, Slice& upper);
};

} // namespace ec
