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

#include <cstdint>
#include <iostream>
#include <string>

namespace ec {
enum class StateType : std::uint8_t {
  ComputationalBasis = 0,
  Random1QBasis = 1,
  Stabilizer = 2
};

inline std::string toString(const StateType& type) noexcept {
  switch (type) {
  case StateType::Random1QBasis:
    return "random_1Q_basis";
  case StateType::Stabilizer:
    return "stabilizer";
  default:
    return "computational_basis";
  }
}

inline StateType stateTypeFromString(const std::string& type) noexcept {
  if ((type == "computational_basis") || (type == "0") ||
      (type == "classical")) {
    return StateType::ComputationalBasis;
  }
  if ((type == "random_1Q_basis") || (type == "1") ||
      (type == "local_quantum")) {
    return StateType::Random1QBasis;
  }
  if ((type == "stabilizer") || (type == "2") || (type == "global_quantum")) {
    return StateType::Stabilizer;
  }
  std::cerr << "Unknown state type: " << type
            << ". Defaulting to computational basis states.\n";
  return StateType::ComputationalBasis;
}

inline std::istream& operator>>(std::istream& in, StateType& type) {
  std::string token;
  in >> token;

  if (token.empty()) {
    in.setstate(std::istream::failbit);
    return in;
  }

  type = stateTypeFromString(token);
  return in;
}

inline std::ostream& operator<<(std::ostream& out, const StateType& type) {
  out << toString(type);
  return out;
}
} // namespace ec
