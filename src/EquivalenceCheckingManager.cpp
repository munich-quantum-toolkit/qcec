/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "EquivalenceCheckingManager.hpp"

#include "EquivalenceCriterion.hpp"
#include "ThreadSafeQueue.hpp"
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
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
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
  for (auto physicalQubitIt = largerCircuitLayoutCopy.rbegin();
       physicalQubitIt != largerCircuitLayoutCopy.rend(); ++physicalQubitIt) {
    const auto physicalQubitIndex = physicalQubitIt->first;

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
          std::any_of(largerCircuit.outputPermutation.begin(),
                      largerCircuit.outputPermutation.end(),
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
      const auto it = std::find_if(smallerCircuit.initialLayout.begin(),
                                   smallerCircuit.initialLayout.end(),
                                   [logicalQubitIndex](const auto& pair) {
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
          std::any_of(largerCircuit.outputPermutation.begin(),
                      largerCircuit.outputPermutation.end(),
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
          std::any_of(smallerCircuit.outputPermutation.begin(),
                      smallerCircuit.outputPermutation.end(),
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
  done = false;

  results.equivalence = EquivalenceCriterion::NoInformation;

  const bool garbageQubitsPresent =
      qc1.getNgarbageQubits() > 0 || qc2.getNgarbageQubits() > 0;

  if (!configuration.anythingToExecute()) {
    std::clog << "Nothing to be executed. Check your configuration!\n";
    return;
  }

  if (qc1.empty() && qc2.empty()) {
    results.equivalence = EquivalenceCriterion::Equivalent;
    done = true;
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

  for (const auto& checker : checkers) {
    nlohmann::basic_json j{};
    checker->json(j);
    results.checkerResults.emplace_back(j);
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

void EquivalenceCheckingManager::checkSequential() {
  const auto start = std::chrono::steady_clock::now();

  // in case a timeout is configured, a separate thread is started that sets
  // the `done` flag after the timeout has passed
  std::thread timeoutThread{};
  if (configuration.execution.timeout > 0.) {
    timeoutThread = std::thread([this, timeout = std::chrono::duration<double>(
                                           configuration.execution.timeout)] {
      std::unique_lock doneLock(doneMutex);
      const auto finished =
          doneCond.wait_for(doneLock, timeout, [this] { return done; });
      // if the thread has already finished within the timeout, nothing
      // has to be done
      if (!finished) {
        setAndSignalDone();
      }
    });
  }

  if (configuration.execution.runSimulationChecker) {
    checkers.emplace_back(
        std::make_unique<DDSimulationChecker>(qc1, qc2, configuration));
    auto* const simulationChecker =
        dynamic_cast<DDSimulationChecker*>(checkers.back().get());
    while (!simulationsFinished() && !done) {
      // configure simulation based checker
      simulationChecker->setRandomInitialState(stateGenerator);

      // run the simulation
      ++results.startedSimulations;
      const auto result = simulationChecker->run();
      ++results.performedSimulations;

      // if the run completed but has not yielded any information this
      // indicates a timeout
      if (result == EquivalenceCriterion::NoInformation) {
        if (!done) {
          std::clog << "Simulation run returned without any information. "
                       "Something probably went wrong. Exiting!\n";
        }
        return;
      }

      // break if non-equivalence has been shown
      if (result == EquivalenceCriterion::NotEquivalent) {
        results.equivalence = EquivalenceCriterion::NotEquivalent;
        break;
      }

      // Otherwise, circuits are probably equivalent and execution can
      // continue
      results.equivalence = EquivalenceCriterion::ProbablyEquivalent;
    }

    // Circuits are non-equivalent
    if (results.equivalence == EquivalenceCriterion::NotEquivalent) {
      results.cexInput = simulationChecker->getInitialState();
      results.cexOutput1 = simulationChecker->getInternalState1();
      results.cexOutput2 = simulationChecker->getInternalState2();
      done = true;
      doneCond.notify_one();
    }

    // in case only simulations are performed and every single one is done,
    // everything is done
    if (configuration.onlySimulationCheckerConfigured() &&
        simulationsFinished()) {
      done = true;
      doneCond.notify_one();
    }
  }

  if (configuration.execution.runAlternatingChecker && !done) {
    checkers.emplace_back(
        std::make_unique<DDAlternatingChecker>(qc1, qc2, configuration));
    const auto& alternatingChecker = checkers.back();
    if (!done) {
      const auto result = alternatingChecker->run();

      // if the alternating check produces a result, this is final
      if (result != EquivalenceCriterion::NoInformation) {
        results.equivalence = result;

        // everything is done
        done = true;
        doneCond.notify_one();
      }
    }
  }

  if (configuration.execution.runConstructionChecker && !done) {
    checkers.emplace_back(
        std::make_unique<DDConstructionChecker>(qc1, qc2, configuration));
    const auto& constructionChecker = checkers.back();
    if (!done) {
      const auto result = constructionChecker->run();

      // if the construction check produces a result, this is final
      if (result != EquivalenceCriterion::NoInformation) {
        results.equivalence = result;

        // everything is done
        done = true;
        doneCond.notify_one();
      }
    }
  }

  if (configuration.execution.runZXChecker && !done) {
    if (ZXEquivalenceChecker::canHandle(qc1, qc2)) {
      checkers.emplace_back(
          std::make_unique<ZXEquivalenceChecker>(qc1, qc2, configuration));
      const auto& zxChecker = checkers.back();
      if (!done) {
        const auto result = zxChecker->run();

        // no matter the result, everything is done as this is the last check
        done = true;
        doneCond.notify_one();

        if (result == EquivalenceCriterion::Equivalent ||
            result == EquivalenceCriterion::EquivalentUpToGlobalPhase) {
          results.equivalence = result;
        } else if (result == EquivalenceCriterion::ProbablyNotEquivalent) {
          if (results.equivalence == EquivalenceCriterion::ProbablyEquivalent) {
            std::clog << "The ZX checker suggests that the circuits are not "
                         "equivalent, but the simulation checker suggests that "
                         "they are probably equivalent. Thus, no conclusion "
                         "can be drawn.\n";
            results.equivalence = EquivalenceCriterion::NoInformation;
          } else {
            results.equivalence = result;
          }
        } else {
          assert(result == EquivalenceCriterion::NoInformation);
          if (results.equivalence == EquivalenceCriterion::NoInformation) {
            // this can only happen if the ZX checker is the only checker
            assert(configuration.onlyZXCheckerConfigured());
            std::clog
                << "Only ZX checker specified, but it was not able to conclude "
                   "anything about the equivalence of the circuits!\n"
                << "This can happen since the ZX checker is not complete in "
                   "general.\n"
                << "Consider enabling other checkers to get more "
                   "information.\n";
          }
        }
      }
    } else if (configuration.onlyZXCheckerConfigured()) {
      std::clog
          << "Only ZX checker specified, but one of the circuits contains "
             "operations not supported by this checker! Exiting!\n";
      checkers.clear();
      results.equivalence = EquivalenceCriterion::NoInformation;
    }
  }

  const auto end = std::chrono::steady_clock::now();
  results.checkTime = std::chrono::duration<double>(end - start).count();

  // appropriately join the timeout thread, if it was launched
  if (timeoutThread.joinable()) {
    timeoutThread.join();
  }
}

void EquivalenceCheckingManager::checkParallel() {
  const auto start = std::chrono::steady_clock::now();

  std::chrono::time_point<std::chrono::steady_clock,
                          std::chrono::duration<double>>
      deadline{};
  if (configuration.execution.timeout > 0.) {
    deadline =
        start + std::chrono::duration<double>(configuration.execution.timeout);
  }

  if (const auto threadLimit = std::thread::hardware_concurrency();
      threadLimit != 0U && configuration.execution.nthreads > threadLimit) {
    std::clog
        << "Trying to use more threads than the underlying architecture claims "
           "to support. Over-subscription might impact performance!\n";
  }
  const auto maxThreads = configuration.execution.nthreads;

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

  const auto effectiveThreads = std::min(maxThreads, tasksToExecute);

  // reserve space for as many equivalence checkers as there will be
  // parallel threads
  checkers.resize(effectiveThreads);

  // create a thread safe queue which is used to check for available results
  ThreadSafeQueue<std::size_t> queue{};
  std::size_t id = 0U;

  // reserve space for the futures received from the async calls
  std::vector<std::future<void>> futures{};
  futures.reserve(effectiveThreads);

  if (configuration.execution.runAlternatingChecker) {
    // start a new thread that constructs and runs the alternating check
    futures.emplace_back(asyncRunChecker<DDAlternatingChecker>(id, queue));
    ++id;
  }

  if (configuration.execution.runConstructionChecker && !done) {
    // start a new thread that constructs and runs the construction check
    futures.emplace_back(asyncRunChecker<DDConstructionChecker>(id, queue));
    ++id;
  }

  if (configuration.execution.runZXChecker && !done) {
    // start a new thread that constructs and runs the ZX checker
    futures.emplace_back(asyncRunChecker<ZXEquivalenceChecker>(id, queue));
    ++id;
  }

  if (configuration.execution.runSimulationChecker) {
    const auto effectiveThreadsLeft = effectiveThreads - futures.size();
    const auto simulationsToStart =
        std::min(effectiveThreadsLeft, configuration.simulation.maxSims);
    // launch as many simulations as possible
    for (std::size_t i = 0; i < simulationsToStart && !done; ++i) {
      futures.emplace_back(asyncRunChecker<DDSimulationChecker>(id, queue));
      ++id;
      ++results.startedSimulations;
    }
  }

  // wait in a loop while no definitive result has been obtained
  while (!done) {
    std::shared_ptr<std::size_t> completedID{};
    if (configuration.execution.timeout > 0.) {
      completedID = queue.waitAndPopUntil(deadline);
    } else {
      completedID = queue.waitAndPop();
    }

    // in case no completed ID has been returned this indicates a timeout
    // and the computation should stop
    if (!completedID) {
      setAndSignalDone();
      break;
    }

    // otherwise, a checker has finished its execution
    // get the result of the future (which should be ready)
    // this makes sure exceptions are thrown if necessary
    futures.at(*completedID).get();

    // in case non-equivalence has been shown, the execution can be stopped
    const auto* const checker = checkers.at(*completedID).get();
    const auto result = checker->getEquivalence();

    if (result == EquivalenceCriterion::NoInformation) {
      if (dynamic_cast<const ZXEquivalenceChecker*>(checker) != nullptr) {
        if (configuration.onlyZXCheckerConfigured()) {
          std::clog
              << "Only ZX checker specified, but it was not able to conclude "
                 "anything about the equivalence of the circuits!\n"
              << "This can happen since the ZX checker is not complete in "
                 "general.\n"
              << "Consider enabling other checkers to get more "
                 "information.\n";
          setAndSignalDone();
          break;
        }
        continue;
      }
      std::clog << "Finished equivalence check provides no information. "
                   "Something probably went wrong. Exiting.\n";
      setAndSignalDone();
      results.equivalence = result;
      break;
    }

    if (result == EquivalenceCriterion::NotEquivalent) {
      setAndSignalDone();
      results.equivalence = result;

      // some special handling in case non-equivalence has been shown by a
      // simulation run
      if (const auto* const simulationChecker =
              dynamic_cast<const DDSimulationChecker*>(checker)) {
        ++results.performedSimulations;
        results.cexInput = simulationChecker->getInitialState();
        results.cexOutput1 = simulationChecker->getInternalState1();
        results.cexOutput2 = simulationChecker->getInternalState2();
      }
      break;
    }

    // the alternating and the construction checker provide definitive answers
    // once they finish
    if ((dynamic_cast<const DDAlternatingChecker*>(checker) != nullptr) ||
        (dynamic_cast<const DDConstructionChecker*>(checker) != nullptr)) {
      setAndSignalDone();
      results.equivalence = result;
      break;
    }

    if (dynamic_cast<const ZXEquivalenceChecker*>(checker) != nullptr) {
      if (result == EquivalenceCriterion::Equivalent ||
          result == EquivalenceCriterion::EquivalentUpToGlobalPhase) {
        setAndSignalDone();
        results.equivalence = result;
        break;
      }

      if (result == EquivalenceCriterion::ProbablyNotEquivalent) {
        if (results.equivalence == EquivalenceCriterion::ProbablyEquivalent) {
          if (simulationsFinished()) {
            std::clog << "The ZX checker suggests that the circuits are not "
                         "equivalent, but the simulation checker suggests that "
                         "they are probably equivalent. Thus, no conclusion "
                         "can be drawn.\n";
            setAndSignalDone();
            results.equivalence = EquivalenceCriterion::NoInformation;
            break;
          }
          results.equivalence = result;
          continue;
        }

        // update result if no information is known
        if (results.equivalence == EquivalenceCriterion::NoInformation) {
          results.equivalence = result;
          if (configuration.onlyZXCheckerConfigured()) {
            setAndSignalDone();
            break;
          }
          // Since the ZX checker is not complete, it cannot conclude
          // non-equivalence, but only suggest it. If the ZX checker is not the
          // only checker configured to run, the run continues uninterrupted.
          continue;
        }
      }
    }

    // at this point, the only option is that this is a simulation checker
    if (dynamic_cast<const DDSimulationChecker*>(checker) != nullptr) {
      ++results.performedSimulations;

      // if no information is known, the successful simulation suggests that
      // both circuits are likely to be equivalent.
      if (results.equivalence == EquivalenceCriterion::NoInformation) {
        results.equivalence = EquivalenceCriterion::ProbablyEquivalent;
      }

      if (simulationsFinished()) {
        if (configuration.onlySimulationCheckerConfigured()) {
          // if only simulations are performed and all of them are successful,
          // the circuits are most likely equivalent, and the procedure is done.
          setAndSignalDone();
          break;
        }

        if (results.equivalence ==
            EquivalenceCriterion::ProbablyNotEquivalent) {
          std::clog
              << "The ZX checker suggests that the circuits are not "
                 "equivalent, but the simulation checker suggests that they "
                 "are probably equivalent. Thus, no conclusion can be drawn.\n";
          setAndSignalDone();
          results.equivalence = EquivalenceCriterion::NoInformation;
          break;
        }
        // if all simulations finished and none of them showed non-equivalence,
        // the run continues uninterrupted.
        continue;
      }

      // it has to be checked, whether further simulations shall be
      // conducted
      if (results.startedSimulations < configuration.simulation.maxSims) {
        futures[*completedID] =
            asyncRunChecker<DDSimulationChecker>(*completedID, queue);
        ++results.startedSimulations;
      }
    }
  }

  const auto end = std::chrono::steady_clock::now();
  results.checkTime = std::chrono::duration<double>(end - start).count();

  // Futures are not explicitly waited for here, since the destructor of the
  // `std::future` object will block until the associated thread has finished.
  // If any thread is still stuck in a long-running operation, this might take a
  // while, but the program will terminate anyway. C++20 introduces
  // `std::jthread`, which allows to explicitly cancel a thread. This could be a
  // solution for the future to avoid this problem (and reduce the number of
  // `isDone` checks).
}

void EquivalenceCheckingManager::checkSymbolic() {
  const auto start = std::chrono::steady_clock::now();
  // in case a timeout is configured, a separate thread is started that
  // sets the `done` flag after the timeout has passed
  std::thread timeoutThread{};
  if (configuration.execution.timeout > 0.) {
    timeoutThread = std::thread([this, timeout = std::chrono::duration<double>(
                                           configuration.execution.timeout)] {
      std::unique_lock doneLock(doneMutex);
      auto finished =
          doneCond.wait_for(doneLock, timeout, [this] { return done; });
      // if the thread has already finished within the timeout,
      // nothing has to be done
      if (!finished) {
        setAndSignalDone();
      }
    });
  }

  if (!done) {
    if (zx::FunctionalityConstruction::transformableToZX(&qc1) &&
        zx::FunctionalityConstruction::transformableToZX(&qc2)) {
      checkers.emplace_back(
          std::make_unique<ZXEquivalenceChecker>(qc1, qc2, configuration));
      const auto& zxChecker = checkers.back();
      if (!done) {
        const auto result = zxChecker->run();
        results.equivalence = result;
        done = true;
        doneCond.notify_one();
      }
    } else {
      std::clog << "Checking symbolic circuits requires transformation "
                   "to ZX-diagram but one of the circuits contains "
                   "operations not supported by this checker! Exiting!"
                << '\n';
      checkers.clear();
      results.equivalence = EquivalenceCriterion::NoInformation;
      return;
    }
  }
  const auto end = std::chrono::steady_clock::now();
  results.checkTime = std::chrono::duration<double>(end - start).count();
  // appropriately join the timeout thread, if it was launched
  if (timeoutThread.joinable()) {
    timeoutThread.join();
  }
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
