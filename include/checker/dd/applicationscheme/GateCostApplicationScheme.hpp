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

#include "ApplicationScheme.hpp"
#include "checker/dd/TaskManager.hpp"
#include "ir/QuantumComputation.hpp"
#include "ir/operations/OpType.hpp"

#include <cstddef>
#include <fstream>
#include <functional>
#include <istream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

template <> struct std::hash<std::pair<qc::OpType, std::size_t>> {
  std::size_t
  operator()(pair<qc::OpType, std::size_t> const& key) const noexcept {
    const std::size_t h1 = hash<decltype(key.first)>{}(key.first);
    const std::size_t h2 = hash<decltype(key.second)>{}(key.second);
    return h1 ^ (h2 << 1);
  }
}; // namespace std

namespace ec {
using GateCostLookupTableKeyType = std::pair<qc::OpType, std::size_t>;
using GateCostLookupTable =
    std::unordered_map<GateCostLookupTableKeyType, std::size_t>;
using CostFunction =
    std::function<std::size_t(const GateCostLookupTableKeyType&)>;

template <class DDType>
class GateCostApplicationScheme final : public ApplicationScheme<DDType> {
public:
  GateCostApplicationScheme(TaskManager<DDType>& tm1, TaskManager<DDType>& tm2,
                            const CostFunction& costFunction,
                            const bool singleQubitGateFusion)
      : ApplicationScheme<DDType>(tm1, tm2),
        singleQubitGateFusionEnabled(singleQubitGateFusion) {
    populateLookupTable(costFunction, tm1.getCircuit());
    populateLookupTable(costFunction, tm2.getCircuit());
  }

  GateCostApplicationScheme(TaskManager<DDType>& tm1, TaskManager<DDType>& tm2,
                            const std::string& filename,
                            const bool singleQubitGateFusion)
      : ApplicationScheme<DDType>(tm1, tm2),
        singleQubitGateFusionEnabled(singleQubitGateFusion) {
    populateLookupTable(filename);
  }

  std::pair<size_t, size_t> operator()() override {
    if (gateCostLookupTable.empty()) {
      return {1U, 1U};
    }

    const auto& op = (*this->taskManager1)();
    if (singleQubitGateFusionEnabled && op->getUsedQubits().size() == 1U) {
      // when single qubit gates are fused, any single-qubit gate should have a
      // single (compound) gate in the other circuit as a counterpart.
      return {1U, 1U};
    }

    const auto key =
        GateCostLookupTableKeyType{op->getType(), op->getNcontrols()};
    std::size_t cost = 1U;
    if (const auto it = gateCostLookupTable.find(key);
        it != gateCostLookupTable.end()) {
      cost = it->second;
    }
    return {1U, cost};
  }

private:
  GateCostLookupTable gateCostLookupTable;
  bool singleQubitGateFusionEnabled;

  template <class CostFun>
  void populateLookupTable(CostFun costFunction,
                           const qc::QuantumComputation* qc) {
    for (const auto& op : *qc) {
      const auto type = op->getType();
      const auto nControls = op->getNcontrols();
      const auto key = GateCostLookupTableKeyType{type, nControls};
      if (const auto it = gateCostLookupTable.find(key);
          it == gateCostLookupTable.end()) {
        const auto cost = costFunction(key);
        gateCostLookupTable.emplace(key, cost);
      }
    }
  }

  // read gate cost LUT from file
  // simple file format:
  // each line consists of
  // <identifier> <controls> <cost>
  void populateLookupTable(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs.good()) {
      throw std::invalid_argument("Error opening LUT file: " + filename);
    }
    populateLookupTable(ifs);
  }
  void populateLookupTable(std::istream& is) {
    qc::OpType opType = qc::OpType::None;
    std::size_t nControls = 0U;
    std::size_t cost = 1U;

    std::string line;
    while (std::getline(is, line)) {
      if (line.empty() || line[0] == '#') {
        continue;
      }
      std::istringstream iss(line);
      if (iss >> opType >> nControls >> cost) {
        gateCostLookupTable.emplace(std::pair{opType, nControls}, cost);
      }
    }
  }
};
} // namespace ec
