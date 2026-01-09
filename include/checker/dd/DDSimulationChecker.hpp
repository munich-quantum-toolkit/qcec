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

#include "DDEquivalenceChecker.hpp"
#include "EquivalenceCriterion.hpp"
#include "checker/dd/TaskManager.hpp"
#include "dd/Node.hpp"

#include <nlohmann/json_fwd.hpp>

namespace qc {
class QuantumComputation;
}

namespace ec {
class Configuration;
class StateGenerator;

class DDSimulationChecker final : public DDEquivalenceChecker<dd::VectorDD> {
public:
  DDSimulationChecker(const qc::QuantumComputation& circ1,
                      const qc::QuantumComputation& circ2,
                      Configuration config);

  void setRandomInitialState(StateGenerator& generator);

  /// Returns the initial state used for simulation
  [[nodiscard]] auto getInitialState() const -> const auto& {
    return initialState;
  }
  /// Returns the internal state of the first task manager
  [[nodiscard]] auto getInternalState1() const -> const auto& {
    return taskManager1.getInternalState();
  }

  /// Returns the internal state of the second task manager
  [[nodiscard]] auto getInternalState2() const -> const auto& {
    return taskManager2.getInternalState();
  }

  void json(nlohmann::basic_json<>& j) const noexcept override;

private:
  // the initial state used for simulation. defaults to the all-zero state
  // |0...0>
  dd::VectorDD initialState{};

  void initializeTask(TaskManager<dd::VectorDD>& taskManager) override;
  EquivalenceCriterion checkEquivalence() override;
};
} // namespace ec
