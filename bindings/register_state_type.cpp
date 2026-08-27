/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "checker/dd/simulation/StateType.hpp"

#include <nanobind/nanobind.h>

namespace ec {

namespace nb = nanobind;
using namespace nb::literals;

// NOLINTNEXTLINE(misc-use-internal-linkage)
void registerStateType(const nb::module_& m) {
  nb::enum_<StateType>(
      m, "StateType", nb::is_arithmetic(),
      R"pb(The type of states used in the simulation checker allows trading off efficiency versus performance.

- Classical stimuli (i.e., random *computational basis states*) already offer extremely high error detection rates in general and are comparatively fast to simulate, which makes them the default.
- Local quantum stimuli (i.e., random *single-qubit basis states*) are a little bit more computationally intensive, but provide even better error detection rates.
- Global quantum stimuli (i.e., random  *stabilizer states*) offer the highest available error detection rate, while at the same time incurring the highest computational effort.

For details, see :cite:p:`burgholzer2021randomStimuliGenerationQuantum`.)pb")

      .value(
          "computational_basis", StateType::ComputationalBasis,
          R"pb(Randomly choose computational basis states. Also referred to as "*classical*".)pb")

      .value("classical", StateType::ComputationalBasis,
             R"pb(Alias for :attr:`~StateType.computational_basis`.)pb")

      .value(
          "random_1q_basis", StateType::Random1QBasis,
          R"pb(Randomly choose a single-qubit basis state for each qubit from the six-tuple *(|0>, |1>, |+>, |->, |L>, |R>)*. Also referred to as *"local_random"*.)pb")

      .value("local_quantum", StateType::Random1QBasis,
             R"pb(Alias for :attr:`~StateType.random_1q_basis`.)pb")

      .value(
          "stabilizer", StateType::Stabilizer,
          R"pb(Randomly choose a stabilizer state by creating a random Clifford circuit. Also referred to as *"global_random"*.)pb")

      .value("global_quantum", StateType::Stabilizer,
             R"pb(Alias for :attr:`~StateType.stabilizer`.)pb");
}

} // namespace ec
