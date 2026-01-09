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

#include "checker/dd/TaskManager.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>

namespace ec {
// A list of application schemes that implement the below interface
enum class ApplicationSchemeType : std::uint8_t {
  Sequential = 0,
  OneToOne = 1,
  Lookahead = 2,
  GateCost = 3,
  Proportional = 4
};

inline std::string
toString(const ApplicationSchemeType& applicationScheme) noexcept {
  switch (applicationScheme) {
  case ApplicationSchemeType::Sequential:
    return "sequential";
  case ApplicationSchemeType::OneToOne:
    return "one_to_one";
  case ApplicationSchemeType::Lookahead:
    return "lookahead";
  case ApplicationSchemeType::GateCost:
    return "gate_cost";
  default:
    return "proportional";
  }
}

inline ApplicationSchemeType
applicationSchemeFromString(const std::string& applicationScheme) noexcept {
  if ((applicationScheme == "sequential") || (applicationScheme == "0") ||
      (applicationScheme == "reference")) {
    return ApplicationSchemeType::Sequential;
  }
  if ((applicationScheme == "one_to_one") || (applicationScheme == "1") ||
      (applicationScheme == "naive")) {
    return ApplicationSchemeType::OneToOne;
  }
  if ((applicationScheme == "lookahead") || (applicationScheme == "2")) {
    return ApplicationSchemeType::Lookahead;
  }
  if ((applicationScheme == "gate_cost") || (applicationScheme == "3") ||
      (applicationScheme == "compilation_flow")) {
    return ApplicationSchemeType::GateCost;
  }
  if ((applicationScheme == "proportional") || (applicationScheme == "4")) {
    return ApplicationSchemeType::Proportional;
  }
  std::cerr << "Unknown application scheme: " << applicationScheme
            << ". Defaulting to proportional!\n";
  return ApplicationSchemeType::Proportional;
}

inline std::istream& operator>>(std::istream& in,
                                ApplicationSchemeType& applicationScheme) {
  std::string token;
  in >> token;

  if (token.empty()) {
    in.setstate(std::istream::failbit);
    return in;
  }

  applicationScheme = applicationSchemeFromString(token);
  return in;
}

inline std::ostream&
operator<<(std::ostream& out, const ApplicationSchemeType& applicationScheme) {
  out << toString(applicationScheme);
  return out;
}

// Interface for describing an application scheme
// Given the current state of the check (tracked by two task managers), an
// application scheme describes how to proceed with the check, i.e., how many
// operations to apply from either circuit.
template <class DDType> class ApplicationScheme {
protected:
  using TM = TaskManager<DDType>;

public:
  ApplicationScheme(TM& tm1, TM& tm2) noexcept
      : taskManager1(&tm1), taskManager2(&tm2) {};

  virtual ~ApplicationScheme() = default;

  // get how many gates from either circuit shall be applied next
  virtual std::pair<std::size_t, std::size_t> operator()() = 0;

protected:
  TM* taskManager1;
  TM* taskManager2;
};

} // namespace ec
