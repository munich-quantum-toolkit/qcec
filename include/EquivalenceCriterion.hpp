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
enum class EquivalenceCriterion : std::uint8_t {
  NotEquivalent = 0,
  Equivalent = 1,
  NoInformation = 2,
  ProbablyEquivalent = 3,
  EquivalentUpToGlobalPhase = 4,
  EquivalentUpToPhase = 5,
  ProbablyNotEquivalent = 6
};

inline std::string toString(const EquivalenceCriterion& criterion) noexcept {
  switch (criterion) {
  case EquivalenceCriterion::NotEquivalent:
    return "not_equivalent";
  case EquivalenceCriterion::Equivalent:
    return "equivalent";
  case EquivalenceCriterion::ProbablyEquivalent:
    return "probably_equivalent";
  case EquivalenceCriterion::EquivalentUpToGlobalPhase:
    return "equivalent_up_to_global_phase";
  case EquivalenceCriterion::EquivalentUpToPhase:
    return "equivalent_up_to_phase";
  case EquivalenceCriterion::ProbablyNotEquivalent:
    return "probably_not_equivalent";
  default:
    return "no_information";
  }
}

inline EquivalenceCriterion fromString(const std::string& criterion) noexcept {
  if ((criterion == "not_equivalent") || (criterion == "0")) {
    return EquivalenceCriterion::NotEquivalent;
  }
  if ((criterion == "equivalent") || (criterion == "1")) {
    return EquivalenceCriterion::Equivalent;
  }
  if ((criterion == "probably_equivalent") || (criterion == "2")) {
    return EquivalenceCriterion::ProbablyEquivalent;
  }
  if ((criterion == "equivalent_up_to_global_phase") || (criterion == "3")) {
    return EquivalenceCriterion::EquivalentUpToGlobalPhase;
  }
  if ((criterion == "equivalent_up_to_phase") || (criterion == "4")) {
    return EquivalenceCriterion::EquivalentUpToPhase;
  }
  if ((criterion == "no_information") || (criterion == "5")) {
    return EquivalenceCriterion::NoInformation;
  }
  if ((criterion == "probably_not_equivalent") || (criterion == "6")) {
    return EquivalenceCriterion::ProbablyNotEquivalent;
  }
  std::cerr << "Unknown equivalence criterion: " << criterion
            << ". Defaulting to `no_information`.\n";
  return EquivalenceCriterion::NoInformation;
}

inline std::istream& operator>>(std::istream& in,
                                EquivalenceCriterion& criterion) {
  std::string token;
  in >> token;

  if (token.empty()) {
    in.setstate(std::istream::failbit);
    return in;
  }

  criterion = fromString(token);
  return in;
}

inline std::ostream& operator<<(std::ostream& out,
                                const EquivalenceCriterion& criterion) {
  out << toString(criterion);
  return out;
}
} // namespace ec
