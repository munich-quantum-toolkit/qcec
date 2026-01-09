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
#include "checker/dd/TaskManager.hpp"
#include "dd/Node.hpp"

#include <nlohmann/json_fwd.hpp>

namespace qc {
class QuantumComputation;
}

namespace ec {
class Configuration;

class DDConstructionChecker final : public DDEquivalenceChecker<dd::MatrixDD> {
public:
  DDConstructionChecker(const qc::QuantumComputation& circ1,
                        const qc::QuantumComputation& circ2,
                        Configuration config);

  void json(nlohmann::json& j) const noexcept override;

private:
  void initializeTask(TaskManager<dd::MatrixDD>& taskManager) override;
};
} // namespace ec
