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
#include "checker/dd/DDSimulationChecker.hpp"
#include "checker/dd/applicationscheme/ApplicationScheme.hpp"
#include "checker/dd/applicationscheme/GateCostApplicationScheme.hpp"
#include "checker/dd/simulation/StateGenerator.hpp"
#include "dd/Node.hpp"
#include "ir/QuantumComputation.hpp"

#include <concepts>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <future>
#include <memory>
#include <mutex>
#include <nlohmann/json_fwd.hpp>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace ec {

class EquivalenceCheckingManager {
public:
  struct Results {
    double preprocessingTime{};
    double checkTime{};

    EquivalenceCriterion equivalence = EquivalenceCriterion::NoInformation;

    std::size_t startedSimulations = 0U;
    std::size_t performedSimulations = 0U;
    dd::VectorDD cexInput{};
    dd::VectorDD cexOutput1{};
    dd::VectorDD cexOutput2{};
    std::size_t performedInstantiations = 0U;

    nlohmann::json checkerResults = nlohmann::json::array();

    [[nodiscard]] bool consideredEquivalent() const {
      switch (equivalence) {
      case EquivalenceCriterion::Equivalent:
      case EquivalenceCriterion::ProbablyEquivalent:
      case EquivalenceCriterion::EquivalentUpToGlobalPhase:
      case EquivalenceCriterion::EquivalentUpToPhase:
        return true;
      default:
        return false;
      }
    }

    [[nodiscard]] nlohmann::json json() const;
    [[nodiscard]] std::string toString() const { return json().dump(2); }
    friend std::ostream& operator<<(std::ostream& os, const Results& res) {
      return os << res.toString();
    }
  };

  EquivalenceCheckingManager(const qc::QuantumComputation& circ1,
                             const qc::QuantumComputation& circ2,
                             Configuration config = Configuration{});

  void run();

  void reset() {
    stateGenerator.clear();
    results = Results();
    checkers.clear();
  }

  [[nodiscard]] EquivalenceCriterion equivalence() const {
    return results.equivalence;
  }

  /// Returns a mutable reference to the used configuration
  [[nodiscard]] auto getConfiguration() -> auto& { return configuration; }

  /// Returns an immutable reference to the results of the equivalence check
  [[nodiscard]] auto getResults() const -> const auto& { return results; }

  /**
   * @brief Get an immutable reference to the first circuit
   * @details This method allows introspection of the first circuit after the
   * EquivalenceCheckingManager has been constructed, which entails running the
   * configured optimizations.
   * @return The first circuit
   */
  [[nodiscard]] auto getFirstCircuit() const -> const auto& { return qc1; }

  /**
   * @brief Get an immutable reference to the second circuit
   * @details This method allows introspection of the second circuit after the
   * EquivalenceCheckingManager has been constructed, which entails running the
   * configured optimizations.
   * @return The second circuit
   */
  [[nodiscard]] auto getSecondCircuit() const -> const auto& { return qc2; }

  /// Disable all previously enabled checkers
  void disableAllCheckers() {
    configuration.execution.runConstructionChecker = false;
    configuration.execution.runZXChecker = false;
    configuration.execution.runSimulationChecker = false;
    configuration.execution.runAlternatingChecker = false;
  }

  /**
   * @brief Set the application scheme for all checkers that support schemes.
   * @param applicationScheme The application scheme to use
   */
  void setApplicationScheme(const ApplicationSchemeType applicationScheme) {
    configuration.application.constructionScheme = applicationScheme;
    configuration.application.simulationScheme = applicationScheme;
    configuration.application.alternatingScheme = applicationScheme;
  }

  /**
   * @brief Set the gate cost profile for all checkers that support schemes.
   * @details This also sets the application scheme to GateCost.
   * @param profileLocation The location of the gate cost profile.
   */
  void setGateCostProfile(const std::string_view profileLocation) {
    configuration.application.constructionScheme =
        ApplicationSchemeType::GateCost;
    configuration.application.simulationScheme =
        ApplicationSchemeType::GateCost;
    configuration.application.alternatingScheme =
        ApplicationSchemeType::GateCost;
    configuration.application.profile = profileLocation;
  }

  /**
   * @brief Set the gate cost function for all checkers that support schemes.
   * @details This also sets the application scheme to GateCost.
   * @param costFunction The cost function to use.
   */
  void setGateCostFunction(const CostFunction& costFunction) {
    configuration.application.constructionScheme =
        ApplicationSchemeType::GateCost;
    configuration.application.simulationScheme =
        ApplicationSchemeType::GateCost;
    configuration.application.alternatingScheme =
        ApplicationSchemeType::GateCost;
    configuration.application.costFunction = costFunction;
  }

protected:
  qc::QuantumComputation qc1;
  qc::QuantumComputation qc2;

  Configuration configuration{};

  StateGenerator stateGenerator;
  std::mutex stateGeneratorMutex;

  // Map from process ID to checker results
  std::map<std::size_t, std::unique_ptr<EquivalenceChecker>> checkers;

  Results results{};

  /// Strip away qubits with no operations applied to them and which do not
  /// occur in the output permutation if they are either idle in both circuits
  /// or idle in one and do not exist (logically) in the other circuit.
  void stripIdleQubits();

  /// Given that one circuit has more qubits than the other, the difference is
  /// assumed to arise from ancillary qubits. This function changes the
  /// additional qubits in the larger circuit to ancillary qubits. Furthermore
  /// it adds corresponding ancillaries in the smaller circuit
  void setupAncillariesAndGarbage();

  /// Run all configured optimization passes
  void runOptimizationPasses();

  /// Sequential Equivalence Check (TCAD'21)
  /// First, a couple of simulations with various stimuli are conducted.
  /// If any of those stimuli produce output states with a fidelity not close to
  /// 1, the non-equivalence has been shown and the check is finished. Given
  /// that a couple of simulations did not show any signs of non-equivalence,
  /// the circuits are probably equivalent. To assure this, the alternating
  /// decision diagram checker is invoked to determine the equivalence.
  void checkSequential();

  /// Helper method that runs sequential checks - can be called in a separate
  /// process for timeout
  EquivalenceCriterion runSequentialChecks();

  void checkSymbolic();

  /// Helper method that runs symbolic check - can be called in a separate
  /// process for timeout
  EquivalenceCriterion runSymbolicCheck();

  /// Helper method to execute a task with optional timeout using ProcessManager
  /// @param task The task to execute
  /// @param timeout The timeout duration (0 means no timeout)
  /// @return The equivalence criterion result
  EquivalenceCriterion
  executeWithOptionalTimeout(const std::function<EquivalenceCriterion()>& task,
                             std::chrono::duration<double> timeout);

  /// Parallel Equivalence Check
  /// The parallel flow makes use of the available processing power by
  /// orchestrating all configured checks in a parallel fashion
  void checkParallel();

  /**
   * @brief Create a task that runs an EquivalenceChecker
   * @tparam Checker Checker type (must derive from EquivalenceChecker)
   * @return Callable task suitable for execution in ProcessManager
   *
   * @details Captures circuit data by value for safety in forked processes.
   * StateGenerator is captured by reference as it's copy-on-write safe after
   * fork().
   */
  template <std::derived_from<EquivalenceChecker> Checker>
  auto makeCheckerTask() -> std::function<EquivalenceCriterion()> {
    // Capture circuits and config by value for process safety
    return [qc1 = this->qc1, qc2 = this->qc2, config = this->configuration,
            &stateGen = this->stateGenerator]() -> EquivalenceCriterion {
      auto checker = std::make_unique<Checker>(qc1, qc2, config);

      // Special handling for simulation checker
      if constexpr (std::is_same_v<Checker, DDSimulationChecker>) {
        // No dynamic_cast needed - we know the type at compile time
        checker->setRandomInitialState(stateGen);
      }

      return checker->run();
    };
  }

  [[nodiscard]] bool simulationsFinished() const {
    return results.performedSimulations == configuration.simulation.maxSims;
  }
};
} // namespace ec
