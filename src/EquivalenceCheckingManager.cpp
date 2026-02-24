/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "EquivalenceCheckingManager.hpp"

#include "EquivalenceCriterion.hpp"
#include "ProcessManager.hpp"
#include "checker/dd/DDAlternatingChecker.hpp"
#include "checker/dd/DDConstructionChecker.hpp"
#include "checker/dd/DDSimulationChecker.hpp"
#include "checker/dd/simulation/StateType.hpp"
#include "checker/zx/ZXChecker.hpp"
#include "circuit_optimizer/CircuitOptimizer.hpp"
#include "dd/ComplexNumbers.hpp"
#include "ir/Definitions.hpp"
#include "ir/Permutation.hpp"
#include "ir/QuantumComputation.hpp"
#include "zx/FunctionalityConstruction.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace ec {

namespace {
// Decrement logical qubit indices in the layout that exceed logicalQubitIndex
void decrementLogicalQubitsInLayoutAboveIndex(
    qc::Permutation& layout, const qc::Qubit logicalQubitIndex) {
  for (auto& [physical, logical] : layout) {
    if (logical > logicalQubitIndex) {
      --logical;
    }
  }
}
} // namespace

void EquivalenceCheckingManager::stripIdleQubits() {
  auto& largerCircuit = qc1.getNqubits() > qc2.getNqubits() ? qc1 : qc2;
  auto& smallerCircuit = qc1.getNqubits() > qc2.getNqubits() ? qc2 : qc1;
  auto qubitDifference =
      largerCircuit.getNqubits() - smallerCircuit.getNqubits();
  auto largerCircuitLayoutCopy = largerCircuit.initialLayout;

  // Iterate over the initialLayout of largerCircuit and remove an idle logical
  // qubit together with the physical qubit it is mapped to
  for (auto& physicalQubitIt :
       std::ranges::reverse_view(largerCircuitLayoutCopy)) {
    const auto physicalQubitIndex = physicalQubitIt.first;

    if (!largerCircuit.isIdleQubit(physicalQubitIndex)) {
      continue;
    }

    const auto logicalQubitIndex =
        largerCircuit.initialLayout.at(physicalQubitIndex);

    bool removedFromSmallerCircuit = false;

    // Remove idle logical qubit present exclusively in largerCircuit
    if (qubitDifference > 0 &&
        ((smallerCircuit.getNqubits() == 0) ||
         logicalQubitIndex > smallerCircuit.initialLayout.maxValue())) {
      const bool physicalUsedInOutputPermutation =
          largerCircuit.outputPermutation.find(physicalQubitIndex) !=
          largerCircuit.outputPermutation.end();
      const bool logicalUsedInOutputPermutation =
          std::ranges::any_of(largerCircuit.outputPermutation,
                              [logicalQubitIndex](const auto& pair) {
                                return pair.second == logicalQubitIndex;
                              });

      // a qubit can only be removed if it is not used in the output permutation
      // or if it is used in the output permutation and the logical qubit index
      // matches the logical qubit index in the output permutation for the
      // physical qubit index in question, which indicates that nothing has
      // happened to the qubit in the larger circuit.
      const bool safeToRemoveInLargerCircuit =
          (!physicalUsedInOutputPermutation &&
           !logicalUsedInOutputPermutation) ||
          (physicalUsedInOutputPermutation &&
           largerCircuit.outputPermutation.at(physicalQubitIndex) ==
               logicalQubitIndex);
      if (!safeToRemoveInLargerCircuit) {
        continue;
      }
      largerCircuit.removeQubit(logicalQubitIndex);
      --qubitDifference;

    } else {
      // Remove logical qubit that is idle in both circuits

      // find the corresponding logical qubit in the smaller circuit
      const auto it = std::ranges::find_if(
          smallerCircuit.initialLayout, [logicalQubitIndex](const auto& pair) {
            return pair.second == logicalQubitIndex;
          });
      // the logical qubit has to be present in the smaller circuit, otherwise
      // this would indicate a bug in the circuit IO initialization.
      assert(it != smallerCircuit.initialLayout.end());
      const auto physicalSmaller = it->first;

      // if the qubit is not idle in the second circuit, it cannot be removed
      // from either circuit.
      if (!smallerCircuit.isIdleQubit(physicalSmaller)) {
        continue;
      }

      const bool physicalLargerUsedInOutputPermutation =
          (largerCircuit.outputPermutation.find(physicalQubitIndex) !=
           largerCircuit.outputPermutation.end());

      const bool logicalLargerUsedInOutputPermutation =
          std::ranges::any_of(largerCircuit.outputPermutation,
                              [logicalQubitIndex](const auto& pair) {
                                return pair.second == logicalQubitIndex;
                              });

      const bool safeToRemoveInLargerCircuit =
          (!physicalLargerUsedInOutputPermutation &&
           !logicalLargerUsedInOutputPermutation) ||
          (physicalLargerUsedInOutputPermutation &&
           largerCircuit.outputPermutation.at(physicalQubitIndex) ==
               logicalQubitIndex);
      if (!safeToRemoveInLargerCircuit) {
        continue;
      }

      const bool physicalSmallerUsedInOutputPermutation =
          (smallerCircuit.outputPermutation.find(physicalSmaller) !=
           smallerCircuit.outputPermutation.end());

      const bool logicalSmallerUsedInOutputPermutation =
          std::ranges::any_of(smallerCircuit.outputPermutation,
                              [logicalQubitIndex](const auto& pair) {
                                return pair.second == logicalQubitIndex;
                              });

      const bool safeToRemoveInSmallerCircuit =
          (!physicalSmallerUsedInOutputPermutation &&
           !logicalSmallerUsedInOutputPermutation) ||
          (physicalSmallerUsedInOutputPermutation &&
           smallerCircuit.outputPermutation.at(physicalSmaller) ==
               logicalQubitIndex);
      if (!safeToRemoveInSmallerCircuit) {
        continue;
      }

      // only remove the qubit from both circuits if it is safe to do so in both
      // circuits (i.e., the qubit is not used in the output permutation or if
      // it is used, the logical qubit index matches the logical qubit index in
      // the output permutation for the physical qubit index in question).
      largerCircuit.removeQubit(logicalQubitIndex);
      smallerCircuit.removeQubit(logicalQubitIndex);
      removedFromSmallerCircuit = true;
    }
    decrementLogicalQubitsInLayoutAboveIndex(largerCircuit.initialLayout,
                                             logicalQubitIndex);
    decrementLogicalQubitsInLayoutAboveIndex(largerCircuit.outputPermutation,
                                             logicalQubitIndex);
    if (removedFromSmallerCircuit) {
      decrementLogicalQubitsInLayoutAboveIndex(smallerCircuit.initialLayout,
                                               logicalQubitIndex);
      decrementLogicalQubitsInLayoutAboveIndex(smallerCircuit.outputPermutation,
                                               logicalQubitIndex);
    }
  }
}

void EquivalenceCheckingManager::setupAncillariesAndGarbage() {
  auto& largerCircuit =
      qc1.getNqubits() > qc2.getNqubits() ? this->qc1 : this->qc2;
  auto& smallerCircuit =
      qc1.getNqubits() > qc2.getNqubits() ? this->qc2 : this->qc1;
  const auto qubitDifference =
      largerCircuit.getNqubits() - smallerCircuit.getNqubits();

  if (qubitDifference == 0) {
    return;
  }

  std::vector<std::pair<qc::Qubit, std::optional<qc::Qubit>>> removed{};
  removed.reserve(qubitDifference);

  const auto nqubits = largerCircuit.getNqubits();
  std::vector<bool> garbage(nqubits);

  for (std::size_t i = 0; i < qubitDifference; ++i) {
    const auto logicalQubitIndex = largerCircuit.initialLayout.maxValue();
    garbage[logicalQubitIndex] =
        largerCircuit.logicalQubitIsGarbage(logicalQubitIndex);
    removed.push_back(largerCircuit.removeQubit(logicalQubitIndex));
  }

  // add appropriate ancillary register to smaller circuit
  smallerCircuit.addAncillaryRegister(qubitDifference, "anc_qcec");

  // reverse iterate over the removed qubits and add them back into the larger
  // circuit as ancillary
  const auto rend = removed.rend();
  for (auto it = removed.rbegin(); it != rend; ++it) {
    largerCircuit.addAncillaryQubit(it->first, it->second);
    // restore garbage
    if (garbage[largerCircuit.getNqubits() - 1]) {
      largerCircuit.setLogicalQubitGarbage(
          static_cast<qc::Qubit>(largerCircuit.getNqubits() - 1));
    }
    // also set the appropriate qubits in the smaller circuit as garbage
    smallerCircuit.setLogicalQubitGarbage(
        static_cast<qc::Qubit>(largerCircuit.getNqubits() - 1));
  }
}

void EquivalenceCheckingManager::runOptimizationPasses() {
  if (qc1.empty() && qc2.empty()) {
    return;
  }

  const auto isDynamicCircuit1 = qc1.isDynamic();
  const auto isDynamicCircuit2 = qc2.isDynamic();
  if (isDynamicCircuit1 || isDynamicCircuit2) {
    if (configuration.optimizations.transformDynamicCircuit) {
      if (isDynamicCircuit1) {
        qc::CircuitOptimizer::eliminateResets(qc1);
        qc::CircuitOptimizer::deferMeasurements(qc1);
      }
      if (isDynamicCircuit2) {
        qc::CircuitOptimizer::eliminateResets(qc2);
        qc::CircuitOptimizer::deferMeasurements(qc2);
      }
    } else {
      throw std::runtime_error(
          "One of the circuits contains mid-circuit non-unitary primitives. "
          "To verify such circuits, the checker must be configured with "
          "`transformDynamicCircuit=true` (`transform_dynamic_circuits=True` "
          "in Python).");
    }
  }

  // first, make sure any potential SWAPs are reconstructed
  if (configuration.optimizations.reconstructSWAPs) {
    qc::CircuitOptimizer::swapReconstruction(qc1);
    qc::CircuitOptimizer::swapReconstruction(qc2);
  }

  // then, optionally backpropagate the output permutation
  if (configuration.optimizations.backpropagateOutputPermutation) {
    qc::CircuitOptimizer::backpropagateOutputPermutation(qc1);
    qc::CircuitOptimizer::backpropagateOutputPermutation(qc2);
  }

  // based on the above, all SWAPs should be reconstructed and accounted for,
  // so we can elide them.
  if (configuration.optimizations.elidePermutations) {
    qc::CircuitOptimizer::elidePermutations(qc1);
    qc::CircuitOptimizer::elidePermutations(qc2);
  }

  // fuse consecutive single qubit gates into compound operations (includes some
  // simple cancellation rules).
  if (configuration.optimizations.fuseSingleQubitGates) {
    qc::CircuitOptimizer::singleQubitGateFusion(qc1);
    qc::CircuitOptimizer::singleQubitGateFusion(qc2);
  }

  // optionally remove diagonal gates before measurements
  if (configuration.optimizations.removeDiagonalGatesBeforeMeasure) {
    qc::CircuitOptimizer::removeDiagonalGatesBeforeMeasure(qc1);
    qc::CircuitOptimizer::removeDiagonalGatesBeforeMeasure(qc2);
  }

  if (configuration.optimizations.reorderOperations) {
    qc1.reorderOperations();
    qc2.reorderOperations();
  }

  // remove final measurements from both circuits so that the underlying
  // functionality should be unitary
  qc::CircuitOptimizer::removeFinalMeasurements(qc1);
  qc::CircuitOptimizer::removeFinalMeasurements(qc2);
}

void EquivalenceCheckingManager::run() {
  results.equivalence = EquivalenceCriterion::NoInformation;

  const bool garbageQubitsPresent =
      qc1.getNgarbageQubits() > 0 || qc2.getNgarbageQubits() > 0;

  if (!configuration.anythingToExecute()) {
    std::clog << "Nothing to be executed. Check your configuration!\n";
    return;
  }

  if (qc1.empty() && qc2.empty()) {
    results.equivalence = EquivalenceCriterion::Equivalent;
    return;
  }

  if (qc1.isVariableFree() && qc2.isVariableFree()) {
    if (!configuration.execution.parallel ||
        configuration.execution.nthreads <= 1 ||
        configuration.onlySingleTask()) {
      checkSequential();
    } else {
      checkParallel();
    }
  } else {
    checkSymbolic();
  }

  for (const auto& checker : checkers | std::views::values) {
    if (!checker) {
      continue;
    }
    nlohmann::json j;
    checker->json(j);
    results.checkerResults.emplace_back(std::move(j));
  }

  if (!configuration.functionality.checkPartialEquivalence &&
      garbageQubitsPresent &&
      equivalence() == EquivalenceCriterion::NotEquivalent) {
    std::clog << "[QCEC] Warning: at least one of the circuits has garbage "
                 "qubits, but partial equivalence checking is turned off. In "
                 "order to take into account the garbage qubits, enable partial"
                 " equivalence checking. Consult the documentation for more"
                 "information.\n";
  }
}

EquivalenceCheckingManager::EquivalenceCheckingManager(
    // Circuits not passed by value and moved because this would cause slicing.
    // NOLINTNEXTLINE(modernize-pass-by-value)
    const qc::QuantumComputation& circ1, const qc::QuantumComputation& circ2,
    Configuration config)
    : qc1(circ1), qc2(circ2), configuration(std::move(config)) {
  const auto start = std::chrono::steady_clock::now();

  // set numeric tolerance used throughout the check
  dd::ComplexNumbers::setTolerance(configuration.execution.numericalTolerance);

  if (qc1.isVariableFree() && qc2.isVariableFree()) {
    // run all configured optimization passes
    runOptimizationPasses();
  }

  // strip away qubits that are not acted upon
  stripIdleQubits();

  // given that one circuit has more qubits than the other, the difference is
  // assumed to arise from ancillary qubits. adjust both circuits accordingly
  setupAncillariesAndGarbage();

  if (this->qc1.getNqubitsWithoutAncillae() !=
      this->qc2.getNqubitsWithoutAncillae()) {
    std::clog << "[QCEC] Warning: circuits have different number of primary "
                 "inputs! Proceed with caution!\n";
  }

  if (configuration.execution.setAllAncillaeGarbage) {
    for (qc::Qubit q = 0; q < qc1.getNqubits(); ++q) {
      if (qc1.logicalQubitIsAncillary(q)) {
        qc1.setLogicalQubitGarbage(q);
      }
    }
    for (qc::Qubit q = 0; q < qc2.getNqubits(); ++q) {
      if (qc2.logicalQubitIsAncillary(q)) {
        qc2.setLogicalQubitGarbage(q);
      }
    }
  }

  // check whether the alternating checker is configured and can handle the
  // circuits
  if (configuration.execution.runAlternatingChecker &&
      !DDAlternatingChecker::canHandle(this->qc1, this->qc2)) {
    std::clog << "[QCEC] Warning: alternating checker cannot handle the "
                 "circuits. Falling back to construction checker.\n";
    this->configuration.execution.runAlternatingChecker = false;
    this->configuration.execution.runConstructionChecker = true;
  }

  // initialize the stimuli generator
  stateGenerator = StateGenerator(configuration.simulation.seed);

  // check whether the number of selected stimuli does exceed the maximum
  // number of unique computational basis states
  if (configuration.execution.runSimulationChecker &&
      configuration.simulation.stateType == StateType::ComputationalBasis) {
    const auto nq = this->qc1.getNqubitsWithoutAncillae();
    const std::size_t uniqueStates = 1ULL << nq;
    if (nq <= 63U && configuration.simulation.maxSims > uniqueStates) {
      this->configuration.simulation.maxSims = uniqueStates;
    }
  }

  const auto end = std::chrono::steady_clock::now();
  results.preprocessingTime =
      std::chrono::duration<double>(end - start).count();
}

namespace {
/// @brief Helper to get timeout duration from configuration
[[nodiscard]] auto getTimeoutDuration(double timeoutSeconds) {
  return timeoutSeconds > 0. ? std::chrono::duration<double>(timeoutSeconds)
                             : std::chrono::duration<double>::zero();
}
} // namespace

EquivalenceCriterion EquivalenceCheckingManager::executeWithOptionalTimeout(
    const std::function<EquivalenceCriterion()>& task,
    std::chrono::duration<double> timeout) {
  if (timeout.count() <= 0.) {
    return task(); // No timeout - execute directly
  }

  // Use ProcessManager for timeout support
  ProcessManager processManager;
  constexpr std::size_t checkerId = 0;

  if (!processManager.spawn(checkerId, task)) {
    std::cerr << "Failed to spawn process for check with timeout\n";
    return EquivalenceCriterion::NoInformation;
  }

  const auto result = processManager.waitForAny(timeout);
  if (!result) {
    processManager.terminateAll(); // Timeout occurred
    return EquivalenceCriterion::NoInformation;
  }

  return result->completed ? result->equivalence
                           : EquivalenceCriterion::NoInformation;
}

void EquivalenceCheckingManager::checkSequential() {
  const auto start = std::chrono::steady_clock::now();

  // Fast path: no timeout configured - run directly
  if (configuration.execution.timeout <= 0.) {
    results.equivalence = runSequentialChecks();
  } else {
    // Slow path: timeout configured - use ProcessManager
    results.equivalence = executeWithOptionalTimeout(
        [this] { return runSequentialChecks(); },
        getTimeoutDuration(configuration.execution.timeout));
  }

  const auto end = std::chrono::steady_clock::now();
  results.checkTime = std::chrono::duration<double>(end - start).count();
}

void EquivalenceCheckingManager::checkParallel() {
  const auto start = std::chrono::steady_clock::now();
  const auto timeout = getTimeoutDuration(configuration.execution.timeout);

  if (const auto threadLimit = std::thread::hardware_concurrency();
      threadLimit != 0U && configuration.execution.nthreads > threadLimit) {
    std::clog
        << "Trying to use more processes than the underlying architecture "
           "claims "
           "to support cores. Over-subscription might impact performance!\n";
  }
  const auto maxProcesses = configuration.execution.nthreads;

  std::size_t tasksToExecute = 0U;
  if (configuration.execution.runAlternatingChecker) {
    ++tasksToExecute;
  }
  if (configuration.execution.runConstructionChecker) {
    ++tasksToExecute;
  }
  if (configuration.execution.runSimulationChecker) {
    tasksToExecute += configuration.simulation.maxSims;
  }
  if (configuration.execution.runZXChecker) {
    if (zx::FunctionalityConstruction::transformableToZX(&qc1) &&
        zx::FunctionalityConstruction::transformableToZX(&qc2)) {
      ++tasksToExecute;
    } else {
      configuration.execution.runZXChecker = false;
    }
  }

  const auto effectiveProcesses = std::min(maxProcesses, tasksToExecute);

  ProcessManager processManager;
  std::size_t nextId = 0U;
  bool done = false;

  // Enum to track process types
  enum class ProcessType { Alternating, Construction, ZX, Simulation };
  std::map<std::size_t, ProcessType> processTypes;

  // Start alternating checker if configured
  if (configuration.execution.runAlternatingChecker) {
    processManager.spawn(nextId, makeCheckerTask<DDAlternatingChecker>());
    processTypes[nextId] = ProcessType::Alternating;
    ++nextId;
  }

  // Start construction checker if configured
  if (configuration.execution.runConstructionChecker) {
    processManager.spawn(nextId, makeCheckerTask<DDConstructionChecker>());
    processTypes[nextId] = ProcessType::Construction;
    ++nextId;
  }

  // Start ZX checker if configured
  if (configuration.execution.runZXChecker) {
    processManager.spawn(nextId, makeCheckerTask<ZXEquivalenceChecker>());
    processTypes[nextId] = ProcessType::ZX;
    ++nextId;
  }

  // Start simulation checkers
  if (configuration.execution.runSimulationChecker) {
    const auto effectiveProcessesLeft =
        effectiveProcesses - processManager.numRunningProcesses();
    const auto simulationsToStart =
        std::min(effectiveProcessesLeft, configuration.simulation.maxSims);

    for (std::size_t i = 0; i < simulationsToStart; ++i) {
      processManager.spawn(nextId, makeCheckerTask<DDSimulationChecker>());
      processTypes[nextId] = ProcessType::Simulation;
      ++nextId;
      ++results.startedSimulations;
    }
  }

  // Wait for processes to complete
  while (processManager.hasRunningProcesses() && !done) {
    auto result = processManager.waitForAny(timeout);

    // Timeout occurred
    if (!result.has_value()) {
      done = true;
      break;
    }

    const auto& procResult = *result;
    const auto procType = processTypes[procResult.id];
    const auto equivalence = procResult.equivalence;

    // Handle exceptions from child process
    if (procResult.hasException) {
      processManager.terminateAll();

      // Re-throw the exception in the parent process
      switch (static_cast<ExceptionType>(procResult.exceptionCode)) {
      case ExceptionType::InvalidArgument:
        throw std::invalid_argument("Exception in parallel checker");
      case ExceptionType::LogicError:
        throw std::logic_error("Exception in parallel checker");
      case ExceptionType::RuntimeError:
        throw std::runtime_error("Exception in parallel checker");
      default:
        throw std::runtime_error("Unknown exception in parallel checker");
      }
    }

    // Handle non-completion (process killed or failed)
    if (!procResult.completed) {
      std::clog << "Process did not complete successfully\n";
      continue;
    }

    // Handle no information result
    if (equivalence == EquivalenceCriterion::NoInformation) {
      if (procType == ProcessType::ZX) {
        if (configuration.onlyZXCheckerConfigured()) {
          std::clog
              << "Only ZX checker specified, but it was not able to conclude "
                 "anything about the equivalence of the circuits!\n"
              << "This can happen since the ZX checker is not complete in "
                 "general.\n"
              << "Consider enabling other checkers to get more "
                 "information.\n";
          done = true;
          break;
        }
        continue;
      }
      std::clog << "Finished equivalence check provides no information. "
                   "Something probably went wrong. Exiting.\n";
      results.equivalence = equivalence;
      done = true;
      break;
    }

    // Handle non-equivalence - this is definitive
    if (equivalence == EquivalenceCriterion::NotEquivalent) {
      results.equivalence = equivalence;

      // For simulation checkers, we need to re-run in parent to get
      // counter-example
      if (procType == ProcessType::Simulation) {
        ++results.performedSimulations;
        // Note: We lose counter-example data in process model
        // This is a limitation we document
      }
      done = true;
      break;
    }

    // Alternating and construction checkers provide definitive answers
    if (procType == ProcessType::Alternating ||
        procType == ProcessType::Construction) {
      results.equivalence = equivalence;
      done = true;
      break;
    }

    // Handle ZX checker results
    if (procType == ProcessType::ZX) {
      if (equivalence == EquivalenceCriterion::Equivalent ||
          equivalence == EquivalenceCriterion::EquivalentUpToGlobalPhase) {
        results.equivalence = equivalence;
        done = true;
        break;
      }

      if (equivalence == EquivalenceCriterion::ProbablyNotEquivalent) {
        if (results.equivalence == EquivalenceCriterion::ProbablyEquivalent) {
          if (simulationsFinished()) {
            std::clog << "The ZX checker suggests that the circuits are not "
                         "equivalent, but the simulation checker suggests that "
                         "they are probably equivalent. Thus, no conclusion "
                         "can be drawn.\n";
            results.equivalence = EquivalenceCriterion::NoInformation;
            done = true;
            break;
          }
          results.equivalence = equivalence;
          continue;
        }

        if (results.equivalence == EquivalenceCriterion::NoInformation) {
          results.equivalence = equivalence;
          if (configuration.onlyZXCheckerConfigured()) {
            done = true;
            break;
          }
          continue;
        }
      }
    }

    // Handle simulation results
    if (procType == ProcessType::Simulation) {
      ++results.performedSimulations;

      if (results.equivalence == EquivalenceCriterion::NoInformation) {
        results.equivalence = EquivalenceCriterion::ProbablyEquivalent;
      }

      if (simulationsFinished()) {
        if (configuration.onlySimulationCheckerConfigured()) {
          done = true;
          break;
        }

        if (results.equivalence ==
            EquivalenceCriterion::ProbablyNotEquivalent) {
          std::clog
              << "The ZX checker suggests that the circuits are not "
                 "equivalent, but the simulation checker suggests that they "
                 "are probably equivalent. Thus, no conclusion can be drawn.\n";
          results.equivalence = EquivalenceCriterion::NoInformation;
          done = true;
          break;
        }
        continue;
      }

      // Start another simulation if needed
      if (results.startedSimulations < configuration.simulation.maxSims &&
          processManager.numRunningProcesses() < effectiveProcesses) {
        processManager.spawn(nextId, makeCheckerTask<DDSimulationChecker>());
        processTypes[nextId] = ProcessType::Simulation;
        ++nextId;
        ++results.startedSimulations;
      }
    }
  }

  // Terminate all remaining processes
  processManager.terminateAll();

  const auto end = std::chrono::steady_clock::now();
  results.checkTime = std::chrono::duration<double>(end - start).count();
}

void EquivalenceCheckingManager::checkSymbolic() {
  const auto start = std::chrono::steady_clock::now();

  // Fast path: no timeout configured - run directly
  if (configuration.execution.timeout <= 0.) {
    results.equivalence = runSymbolicCheck();
  } else {
    // Slow path: timeout configured - use ProcessManager
    results.equivalence = executeWithOptionalTimeout(
        [this] { return runSymbolicCheck(); },
        getTimeoutDuration(configuration.execution.timeout));
  }

  const auto end = std::chrono::steady_clock::now();
  results.checkTime = std::chrono::duration<double>(end - start).count();
}

EquivalenceCriterion EquivalenceCheckingManager::runSequentialChecks() {
  auto localEquivalence = EquivalenceCriterion::NoInformation;
  std::size_t checkerIdCounter = 0;

  if (configuration.execution.runSimulationChecker) {
    auto simulationChecker =
        std::make_unique<DDSimulationChecker>(qc1, qc2, configuration);
    auto* const simChecker = simulationChecker.get();

    while (results.performedSimulations < configuration.simulation.maxSims) {
      // configure simulation based checker
      simChecker->setRandomInitialState(stateGenerator);

      // run the simulation
      ++results.startedSimulations;
      const auto result = simChecker->run();
      ++results.performedSimulations;

      // if the run completed but has not yielded any information this
      // indicates something went wrong
      if (result == EquivalenceCriterion::NoInformation) {
        return EquivalenceCriterion::NoInformation;
      }

      // break if non-equivalence has been shown
      if (result == EquivalenceCriterion::NotEquivalent) {
        // Note: In process-based execution, counter-example data is lost
        checkers[checkerIdCounter++] = std::move(simulationChecker);
        return EquivalenceCriterion::NotEquivalent;
      }

      // Otherwise, circuits are probably equivalent and execution can continue
      localEquivalence = EquivalenceCriterion::ProbablyEquivalent;
    }

    checkers[checkerIdCounter++] = std::move(simulationChecker);

    // in case only simulations are performed and every single one is done
    if (configuration.onlySimulationCheckerConfigured()) {
      return localEquivalence;
    }
  }

  if (configuration.execution.runAlternatingChecker) {
    auto alternatingChecker =
        std::make_unique<DDAlternatingChecker>(qc1, qc2, configuration);
    const auto result = alternatingChecker->run();
    checkers[checkerIdCounter++] = std::move(alternatingChecker);

    // if the alternating check produces a result, this is final
    if (result != EquivalenceCriterion::NoInformation) {
      return result;
    }
  }

  if (configuration.execution.runConstructionChecker) {
    auto constructionChecker =
        std::make_unique<DDConstructionChecker>(qc1, qc2, configuration);
    const auto result = constructionChecker->run();
    checkers[checkerIdCounter++] = std::move(constructionChecker);

    // if the construction check produces a result, this is final
    if (result != EquivalenceCriterion::NoInformation) {
      return result;
    }
  }

  if (configuration.execution.runZXChecker) {
    if (ZXEquivalenceChecker::canHandle(qc1, qc2)) {
      auto zxChecker =
          std::make_unique<ZXEquivalenceChecker>(qc1, qc2, configuration);
      const auto result = zxChecker->run();
      checkers[checkerIdCounter++] = std::move(zxChecker);

      if (result == EquivalenceCriterion::Equivalent ||
          result == EquivalenceCriterion::EquivalentUpToGlobalPhase) {
        return result;
      } else if (result == EquivalenceCriterion::ProbablyNotEquivalent) {
        if (localEquivalence == EquivalenceCriterion::ProbablyEquivalent) {
          return EquivalenceCriterion::NoInformation;
        }
        return result;
      } else if (result == EquivalenceCriterion::NoInformation &&
                 configuration.onlyZXCheckerConfigured()) {
        return EquivalenceCriterion::NoInformation;
      }
    } else if (configuration.onlyZXCheckerConfigured()) {
      return EquivalenceCriterion::NoInformation;
    }
  }

  return localEquivalence;
}

EquivalenceCriterion EquivalenceCheckingManager::runSymbolicCheck() {
  if (zx::FunctionalityConstruction::transformableToZX(&qc1) &&
      zx::FunctionalityConstruction::transformableToZX(&qc2)) {
    auto zxChecker =
        std::make_unique<ZXEquivalenceChecker>(qc1, qc2, configuration);
    const auto result = zxChecker->run();
    checkers[0] = std::move(zxChecker);
    return result;
  }

  return EquivalenceCriterion::NoInformation;
}

nlohmann::json EquivalenceCheckingManager::Results::json() const {
  nlohmann::json res{};
  res["preprocessing_time"] = preprocessingTime;
  res["check_time"] = checkTime;
  res["equivalence"] = ec::toString(equivalence);

  if (startedSimulations > 0) {
    auto& sim = res["simulations"];
    sim["started"] = startedSimulations;
    sim["performed"] = performedSimulations;
  }
  auto& par = res["parameterized"];
  par["performed_instantiations"] = performedInstantiations;

  res["checkers"] = checkerResults;

  return res;
}
} // namespace ec
