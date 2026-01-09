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
#include "dd/DDDefinitions.hpp"
#include "ir/Definitions.hpp"
#include "ir/QuantumComputation.hpp"
#include "ir/operations/Control.hpp"
#include "ir/operations/OpType.hpp"
#include "ir/operations/StandardOperation.hpp"
#include "qasm3/Importer.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace dd {

namespace {
const std::vector<std::vector<qc::OpType>> PRE_GENERATED_CIRCUITS_SIZE_1_1{
    {}, {}, {}, {}};

const std::vector<std::vector<qc::OpType>> PRE_GENERATED_CIRCUITS_SIZE_1_2{
    {qc::Z}, {qc::Tdg}, {qc::S}, {qc::Sdg}};

const std::vector<std::vector<qc::OpType>> PRE_GENERATED_CIRCUITS_SIZE_2_1{
    {}, {}, {}, {}, {qc::X}, {qc::X}};

const std::vector<std::vector<qc::OpType>> PRE_GENERATED_CIRCUITS_SIZE_2_2{
    {qc::Z}, {qc::Tdg}, {qc::S}, {qc::Sdg}, {qc::X, qc::Z}, {qc::Z, qc::X}};

void addPreGeneratedCircuits(qc::QuantumComputation& circuit1,
                             qc::QuantumComputation& circuit2,
                             const qc::Qubit groupBeginIndex,
                             const qc::Qubit groupSize) {
  const auto& circuits1 = groupSize == 1 ? PRE_GENERATED_CIRCUITS_SIZE_1_1
                                         : PRE_GENERATED_CIRCUITS_SIZE_2_1;
  const auto& circuits2 = groupSize == 1 ? PRE_GENERATED_CIRCUITS_SIZE_1_2
                                         : PRE_GENERATED_CIRCUITS_SIZE_2_2;
  const auto nrCircuits = circuits1.size();
  auto randomGenerator = circuit1.getGenerator();
  std::uniform_int_distribution<size_t> randomDistribution(0, nrCircuits - 1);

  const auto randomIndex = randomDistribution(randomGenerator);
  const auto x1 = circuits1[randomIndex];
  const auto x2 = circuits2[randomIndex];
  for (auto gateType : x1) {
    if (gateType == qc::X) { // add CNOT
      circuit1.emplace_back<qc::StandardOperation>(
          groupBeginIndex, groupBeginIndex + 1, gateType);
    } else {
      circuit1.emplace_back<qc::StandardOperation>(groupBeginIndex, gateType);
    }
  }
  for (auto gateType : x2) {
    if (gateType == qc::X) { // add CNOT
      circuit2.emplace_back<qc::StandardOperation>(
          groupBeginIndex, groupBeginIndex + 1, gateType);
    } else {
      circuit2.emplace_back<qc::StandardOperation>(groupBeginIndex, gateType);
    }
  }
}

void addDecomposedCcxGate(qc::QuantumComputation& circuit,
                          const qc::Controls& controls,
                          const qc::Qubit target) {
  const qc::Qubit control1 = controls.begin()->qubit;
  const qc::Qubit control2 = (++controls.begin())->qubit;
  circuit.h(target);
  circuit.cx(control1, target);
  circuit.tdg(target);
  circuit.cx(control2, target);
  circuit.t(target);
  circuit.cx(control1, target);
  circuit.tdg(target);
  circuit.cx(control2, target);
  circuit.t(target);
  circuit.t(control1);
  circuit.h(target);
  circuit.cx(control2, control1);
  circuit.t(control2);
  circuit.tdg(control1);
  circuit.cx(control2, control1);
}

void addStandardOperationToCircuit(qc::QuantumComputation& circuit,
                                   const qc::StandardOperation& op,
                                   const bool decomposeCcx) {
  if (op.getType() == qc::X && decomposeCcx && op.getControls().size() == 2) {
    // decompose toffoli gate
    addDecomposedCcxGate(circuit, op.getControls(), op.getTargets()[0]);
  } else {
    circuit.emplace_back<qc::StandardOperation>(op);
  }
}

std::vector<qc::Qubit>
fiveDifferentRandomNumbers(const qc::Qubit min, const qc::Qubit max,
                           std::mt19937_64& randomGenerator) {
  std::vector<qc::Qubit> numbers;

  for (qc::Qubit i = min; i < max; i++) {
    numbers.emplace_back(i);
  }
  std::shuffle(numbers.begin(), numbers.end(), randomGenerator);

  const int64_t lengthOutputVector{
      static_cast<int64_t>(std::min<size_t>(5UL, numbers.size()))};

  std::vector<qc::Qubit> outputVector(numbers.begin(),
                                      numbers.begin() + lengthOutputVector);
  return outputVector;
}

qc::StandardOperation convertToStandardOperation(
    const size_t nrQubits, const qc::OpType randomOpType,
    const qc::Qubit randomTarget1, const qc::Qubit randomTarget2,
    const fp randomParameter1, const fp randomParameter2,
    const fp randomParameter3, const qc::Controls& randomControls) {
  switch (randomOpType) {
    // two targets and zero parameters
  case qc::SWAP:
  case qc::iSWAP:
  case qc::iSWAPdg:
  case qc::Peres:
  case qc::Peresdg:
  case qc::DCX:
  case qc::ECR:
    if (nrQubits > 1) {
      return {randomControls, qc::Targets{randomTarget1, randomTarget2},
              randomOpType};
    }
    break;

    // two targets and one parameter
  case qc::RXX:
  case qc::RYY:
  case qc::RZZ:
  case qc::RZX:
    if (nrQubits > 1) {
      return {randomControls, qc::Targets{randomTarget1, randomTarget2},
              randomOpType, std::vector<fp>{randomParameter1}};
    }
    break;

    // two targets and two parameters
  case qc::XXminusYY:
  case qc::XXplusYY:
    if (nrQubits > 1) {
      return {randomControls, qc::Targets{randomTarget1, randomTarget2},
              randomOpType,
              std::vector<fp>{randomParameter1, randomParameter2}};
    }
    break;

    // one target and zero parameters
  case qc::I:
  case qc::H:
  case qc::X:
  case qc::Y:
  case qc::Z:
  case qc::S:
  case qc::Sdg:
  case qc::T:
  case qc::Tdg:
  case qc::V:
  case qc::Vdg:
  case qc::SX:
  case qc::SXdg:
    return {randomControls, randomTarget1, randomOpType};
    // one target and three parameters
  case qc::U:
    return {
        randomControls, randomTarget1, randomOpType,
        std::vector<fp>{randomParameter1, randomParameter2, randomParameter3}};
    // one target and two parameters
  case qc::U2:
    return {randomControls, randomTarget1, randomOpType,
            std::vector<fp>{randomParameter1, randomParameter2}};
    // one target and one parameter
  case qc::P:
  case qc::RX:
  case qc::RY:
  case qc::RZ:
    return {randomControls, randomTarget1, randomOpType,
            std::vector<fp>{randomParameter1}};
  default:
    return {randomTarget1, qc::I};
  }
  return {randomTarget1, qc::I};
}

qc::StandardOperation
makeRandomStandardOperation(const qc::Qubit nrQubits, const qc::Qubit min,
                            std::mt19937_64& randomGenerator) {
  const auto randomNumbers =
      fiveDifferentRandomNumbers(min, min + nrQubits, randomGenerator);

  // choose one of the non-compound operations, but not "None", and also
  // not GPhase or I or Barrier
  std::uniform_int_distribution<> randomDistrOpType(qc::H, qc::RZX);
  auto randomOpType =
      static_cast<qc::OpType>(randomDistrOpType(randomGenerator));
  const qc::Qubit randomTarget1 = randomNumbers[0];
  qc::Qubit randomTarget2{min};
  if (randomNumbers.size() > 1) {
    randomTarget2 = randomNumbers[1];
  };
  // choose random controls, but not more than available qubits
  std::uniform_int_distribution<size_t> randomDistrNrControls(0, 2);
  size_t nrControls = std::min(randomNumbers.size() - 3,
                               randomDistrNrControls(randomGenerator));
  if (randomNumbers.size() < 3) {
    nrControls = 0;
  }
  if (nrControls == 2) {
    // otherwise Toffoli gates are almost never generated
    randomOpType = qc::X;
  }
  qc::Controls randomControls{};
  for (size_t i = 0; i < nrControls; i++) {
    randomControls.emplace(randomNumbers[i + 2]);
  }
  std::uniform_real_distribution<fp> randomDistrParameters(0, 2 * PI);
  const fp randomParameter1 = randomDistrParameters(randomGenerator);
  const fp randomParameter2 = randomDistrParameters(randomGenerator);
  const fp randomParameter3 = randomDistrParameters(randomGenerator);
  return convertToStandardOperation(
      nrQubits, randomOpType, randomTarget1, randomTarget2, randomParameter1,
      randomParameter2, randomParameter3, randomControls);
}

/**
  Generate random benchmarks for partial equivalence checking. Returns pairs of
circuits which are partially equivalent, following the method described in the
paper "Partial Equivalence Checking of Quantum
Circuits" (https://arxiv.org/abs/2208.07564) in Section VI. B.
  @param n number of qubits of the resulting circuits
  @param d number of data qubits in the resulting circuits
  @param m number of measured qubits in the resulting circuit
  @return two circuits that are partially equivalent
**/
std::pair<qc::QuantumComputation, qc::QuantumComputation>
generatePartiallyEquivalentCircuits(const size_t n, const qc::Qubit d,
                                    const qc::Qubit m) {
  if (d > n || m > n) {
    throw std::runtime_error("The number of data or measured qubits can't be "
                             "bigger than the total number of qubits. n = " +
                             std::to_string(n) + "; d = " + std::to_string(d) +
                             "; m = " + std::to_string(m));
  }

  qc::QuantumComputation circuit1{n};
  qc::QuantumComputation circuit2{n};

  auto randomGenerator = circuit1.getGenerator();

  // 1) H gates
  for (qc::Qubit i = 0U; i < d; i++) {
    circuit1.h(i);
    circuit2.h(i);
  }

  circuit1.barrier();
  circuit2.barrier();

  // 2) Totally equivalent subcircuits
  // Generate a random subcircuit with d qubits and 3*d gates to apply
  // on both circuits, but all the Toffoli gates in circuit2 are decomposed
  for (qc::Qubit i = 0U; i < 3 * d; i++) {
    const auto op = makeRandomStandardOperation(d, 0, randomGenerator);
    addStandardOperationToCircuit(circuit1, op, false);
    addStandardOperationToCircuit(circuit2, op, true);
  }

  circuit1.barrier();
  circuit2.barrier();

  // 3) Partially equivalent subcircuits
  // Divide data qubits into groups of size 1 or 2. For each group, we apply
  // pre-generated subcircuits, which are pairwise partially equivalent.
  qc::Qubit groupBeginIndex = 0;
  std::uniform_int_distribution<qc::Qubit> randomDistrGroupSize(1, 2);
  while (groupBeginIndex < d) {
    qc::Qubit groupSize = 1;
    if (groupBeginIndex < d - 1) {
      groupSize = randomDistrGroupSize(randomGenerator);
    }

    addPreGeneratedCircuits(circuit1, circuit2, groupBeginIndex, groupSize);

    groupBeginIndex += groupSize;
  }

  circuit1.barrier();
  circuit2.barrier();

  // 4) Arbitrary gates
  // Arbitrary gates are added to data qubits that are not measured
  if (d > m) {
    const qc::Qubit notMQubits = d - m;
    for (qc::Qubit i = 0U; i < notMQubits; i++) {
      addStandardOperationToCircuit(
          circuit1, makeRandomStandardOperation(notMQubits, m, randomGenerator),
          false);
      addStandardOperationToCircuit(
          circuit2, makeRandomStandardOperation(notMQubits, m, randomGenerator),
          false);
    }
  }

  circuit1.barrier();
  circuit2.barrier();

  // 5) CNOT gates
  // For each ancilla qubit, add a CNOT that has the ancilla as control qubit,
  // and any data qubit as target. As ancilla qubits are initially set to 0 and
  // we haven't added any other gates to the ancilla qubits until now, these
  // CNOT gates do not affect the circuit.
  if (d > 0) {
    qc::Qubit currentDataQubit = 0;
    for (qc::Qubit currentAncillaQubit = d; currentAncillaQubit < n;
         currentAncillaQubit++) {
      auto nextDataQubit = (currentDataQubit + 1) % d;
      circuit1.cx(currentAncillaQubit, currentDataQubit);
      circuit2.cx(currentAncillaQubit, nextDataQubit);
      currentDataQubit = nextDataQubit;
    }
  }

  for (qc::Qubit i = d; i < n; i++) {
    circuit1.setLogicalQubitAncillary(i);
    circuit2.setLogicalQubitAncillary(i);
  }

  for (qc::Qubit i = m; i < n; i++) {
    circuit1.setLogicalQubitGarbage(i);
    circuit2.setLogicalQubitGarbage(i);
  }

  return std::make_pair(circuit1, circuit2);
}
} // namespace
} // namespace dd

class PartialEquivalenceTest : public testing::Test {
  void SetUp() override {
    qc1 = qc::QuantumComputation(nqubits, nqubits);
    qc2 = qc::QuantumComputation(nqubits, nqubits);

    config.execution.runSimulationChecker = false;
    config.execution.runAlternatingChecker = false;
    config.execution.runConstructionChecker = false;
    config.execution.runZXChecker = false;
    config.execution.nthreads = 1;

    config.functionality.checkPartialEquivalence = true;
  }

protected:
  std::size_t nqubits = 3U;
  qc::QuantumComputation qc1;
  qc::QuantumComputation qc2;
  ec::Configuration config{};
};

TEST_F(PartialEquivalenceTest, AlternatingCheckerGarbage) {
  // these circuits have the same gates acting on the measured qubit
  // and random gates acting on the two garbage qubits
  qc1.cswap(1, 0, 2);
  qc1.cx(2, 0);
  qc1.h(0);
  qc1.tdg(1);
  qc1.s(1);
  qc1.z(2);

  qc2.cswap(1, 0, 2);
  qc2.cx(2, 0);
  qc2.h(0);
  qc2.h(2);
  qc2.rz(dd::PI_4, 1);
  qc2.ry(0.1, 1);
  qc2.cx(1, 2);

  qc1.setLogicalQubitGarbage(2);
  qc1.setLogicalQubitGarbage(1);
  qc2.setLogicalQubitGarbage(2);
  qc2.setLogicalQubitGarbage(1);

  config.execution.runAlternatingChecker = true;

  ec::EquivalenceCheckingManager ecm(qc1, qc2, config);
  ecm.run();
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());

  config.functionality.checkPartialEquivalence = false;
  ec::EquivalenceCheckingManager ecm2(qc1, qc2, config);
  ecm2.run();
  EXPECT_EQ(ecm2.equivalence(), ec::EquivalenceCriterion::NotEquivalent);
}

TEST_F(PartialEquivalenceTest, AlternatingCheckerGarbageNotEquivalent) {
  qc1.cswap(1, 0, 2);
  qc1.cx(2, 0);
  qc1.h(0);
  qc1.tdg(1);
  qc1.s(1);
  qc1.z(2);

  qc2.cswap(1, 0, 2);
  qc2.cx(2, 0);
  qc2.h(0);
  qc2.h(2);
  qc2.rz(dd::PI_4, 1);
  qc2.ry(0.1, 1);
  qc2.cx(1, 2);

  config.execution.runAlternatingChecker = true;

  ec::EquivalenceCheckingManager ecm(qc1, qc2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::NotEquivalent);
}

TEST_F(PartialEquivalenceTest, AlternatingCheckerGarbage2) {
  // measured qubit: 1
  qc1.setLogicalQubitGarbage(2);
  qc1.setLogicalQubitGarbage(0);

  qc1.h(1);
  qc1.tdg(0);
  qc1.s(0);
  qc1.z(2);

  // measured qubit: 1
  qc1.setLogicalQubitGarbage(0);
  qc1.setLogicalQubitGarbage(2);
  qc2.setLogicalQubitGarbage(0);
  qc2.setLogicalQubitGarbage(2);

  qc2.h(1);
  qc2.h(0);
  qc2.rz(dd::PI_4, 2);
  qc2.ry(0.1, 2);
  qc2.cx(2, 0);

  config.execution.runAlternatingChecker = true;

  ec::EquivalenceCheckingManager ecm(qc1, qc2, config);
  ecm.run();
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());

  config.functionality.checkPartialEquivalence = false;
  ec::EquivalenceCheckingManager ecm2(qc1, qc2, config);
  ecm2.run();
  EXPECT_EQ(ecm2.equivalence(), ec::EquivalenceCriterion::NotEquivalent);
}

TEST_F(PartialEquivalenceTest, AlternatingCheckerGarbageAndAncillary) {
  qc1.h(0);
  qc1.tdg(1);
  qc1.s(1);
  // ancillary qubits are initialized to zero, therefore this gate doesn't
  // change the functionality of the circuit
  qc1.cx(2, 0);

  qc::QuantumComputation qc3(nqubits - 1);
  qc3.setLogicalQubitGarbage(1);

  qc3.h(0);
  qc3.rz(dd::PI_4, 1);
  qc3.ry(0.1, 1);

  qc1.setLogicalQubitGarbage(2);
  qc1.setLogicalQubitGarbage(1);
  qc1.setLogicalQubitAncillary(2);

  config.execution.runAlternatingChecker = true;
  ec::EquivalenceCheckingManager ecm(qc1, qc3, config);
  ecm.run();
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());

  config.functionality.checkPartialEquivalence = false;
  ec::EquivalenceCheckingManager ecm2(qc1, qc3, config);
  ecm2.run();
  EXPECT_EQ(ecm2.equivalence(), ec::EquivalenceCriterion::NotEquivalent);
}

TEST_F(PartialEquivalenceTest, AlternatingCheckerGarbageNotEquivalent2) {
  // example from the paper https://arxiv.org/abs/2208.07564
  qc1.cswap(1, 0, 2);
  qc1.h(0);
  qc1.z(2);
  qc1.cswap(1, 0, 2);

  qc2.x(1);
  qc2.ch(1, 0);

  qc1.setLogicalQubitGarbage(2);
  qc1.setLogicalQubitGarbage(1);
  qc2.setLogicalQubitGarbage(2);
  qc2.setLogicalQubitGarbage(1);

  // partially equivalent circuits
  config.execution.runAlternatingChecker = true;
  ec::EquivalenceCheckingManager ecm(qc1, qc2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);

  // these two circuits are only partially equivalent,
  // therefore the equivalence checker returns NotEquivalent when we disable
  // partial equality checking
  config.functionality.checkPartialEquivalence = false;
  ec::EquivalenceCheckingManager ecm2(qc1, qc2, config);
  ecm2.run();
  EXPECT_EQ(ecm2.equivalence(), ec::EquivalenceCriterion::NotEquivalent);
}

TEST_F(PartialEquivalenceTest, AlternatingCheckerGarbageNotEquivalent3) {
  qc1.cswap(1, 0, 2);
  qc1.h(0);
  qc1.z(2);
  qc1.cswap(1, 0, 2);

  qc2.x(1);
  qc2.ch(1, 0);

  config.execution.runAlternatingChecker = true;
  ec::EquivalenceCheckingManager ecm(qc1, qc2, config);
  ecm.run();

  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::NotEquivalent);
}

TEST_F(PartialEquivalenceTest, TrivialEquivalence) {
  auto qc = qc::QuantumComputation(2);

  qc.h(1);
  qc.h(0);
  qc.cx(0, 1);
  qc.swap(1, 0);

  config.execution.runConstructionChecker = true;
  ec::EquivalenceCheckingManager ecm(qc, qc, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);

  qc.setLogicalQubitAncillary(1);

  ec::EquivalenceCheckingManager ecm2(qc, qc, config);
  ecm2.run();
  EXPECT_EQ(ecm2.equivalence(), ec::EquivalenceCriterion::Equivalent);

  qc.setLogicalQubitGarbage(1);

  ec::EquivalenceCheckingManager ecm3(qc, qc, config);
  ecm3.run();
  EXPECT_EQ(ecm3.equivalence(), ec::EquivalenceCriterion::Equivalent);

  qc.setLogicalQubitGarbage(0);

  ec::EquivalenceCheckingManager ecm4(qc, qc, config);
  ecm4.run();
  EXPECT_EQ(ecm4.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST_F(PartialEquivalenceTest, BasicPartialEquivalenceChecking) {
  // only the second qubit has differing gates in the two circuits,
  // therefore they should be equivalent if we only measure the first qubit
  qc1.h(1);
  qc1.x(1);

  qc1.measure(0, 0);
  qc2.measure(0, 0);

  qc1.setLogicalQubitGarbage(1);
  qc2.setLogicalQubitGarbage(1);
  qc1.setLogicalQubitGarbage(2);
  qc2.setLogicalQubitGarbage(2);

  config.execution.runConstructionChecker = true;
  ec::EquivalenceCheckingManager ecm(qc1, qc2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST_F(PartialEquivalenceTest, ConstructionCheckerNotEquivalent) {
  // the first qubit has differing gates in the two circuits,
  // therefore they should not be equivalent if we only measure the first qubit
  qc1.h(0);
  qc1.x(0);

  qc1.setLogicalQubitGarbage(1);
  qc2.setLogicalQubitGarbage(1);

  config.execution.runConstructionChecker = true;
  ec::EquivalenceCheckingManager ecm(qc1, qc2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::NotEquivalent);
}

TEST_F(PartialEquivalenceTest, ConstructionCheckerGarbageNotEquivalent) {
  // example from the paper https://arxiv.org/abs/2208.07564
  qc1.cswap(1, 0, 2);
  qc1.h(0);
  qc1.z(2);
  qc1.cswap(1, 0, 2);

  qc2.x(1);
  qc2.ch(1, 0);
  qc2.h(2);

  qc1.setLogicalQubitGarbage(2);
  qc1.setLogicalQubitGarbage(1);
  qc2.setLogicalQubitGarbage(2);
  qc2.setLogicalQubitGarbage(1);

  // partially equivalent circuits
  config.execution.runConstructionChecker = true;
  ec::EquivalenceCheckingManager ecm(qc1, qc2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);

  // these two circuits are only partially equivalent,
  // therefore the equivalence checker returns NotEquivalent when we disable
  // partial equality checking
  config.functionality.checkPartialEquivalence = false;
  ec::EquivalenceCheckingManager ecm2(qc1, qc2, config);
  ecm2.run();
  EXPECT_EQ(ecm2.equivalence(), ec::EquivalenceCriterion::NotEquivalent);
}

TEST_F(PartialEquivalenceTest, ConstructionCheckerNotEquivalent2) {
  // check that garbage reduction still recognizes non equivalent circuits
  qc1.cswap(1, 0, 2);
  qc1.h(0);
  qc1.z(2);
  qc1.cswap(1, 0, 2);

  qc2.x(1);
  qc2.ch(1, 0);
  qc2.h(2);

  qc1.setLogicalQubitGarbage(2);
  qc2.setLogicalQubitGarbage(2);
  // partially equivalent circuits
  config.execution.runConstructionChecker = true;
  ec::EquivalenceCheckingManager ecm(qc1, qc2, config);
  ecm.run();

  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::NotEquivalent);
}

TEST_F(PartialEquivalenceTest, SimulationCheckerGarbageNotEquivalent) {
  // example from the paper https://arxiv.org/abs/2208.07564
  qc1.cswap(1, 0, 2);
  qc1.h(0);
  qc1.z(2);
  qc1.cswap(1, 0, 2);

  qc2.x(1);
  qc2.ch(1, 0);
  qc2.h(2);

  qc1.setLogicalQubitGarbage(2);
  qc1.setLogicalQubitGarbage(1);
  qc2.setLogicalQubitGarbage(2);
  qc2.setLogicalQubitGarbage(1);

  // partially equivalent circuits
  config.execution.runSimulationChecker = true;
  ec::EquivalenceCheckingManager ecm(qc1, qc2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::ProbablyEquivalent);

  // these two circuits are only partially equivalent,
  // therefore the equivalence checker returns NotEquivalent when we disable
  // partial equality checking
  config.functionality.checkPartialEquivalence = false;
  ec::EquivalenceCheckingManager ecm2(qc1, qc2, config);
  ecm2.run();
  EXPECT_EQ(ecm2.equivalence(), ec::EquivalenceCriterion::NotEquivalent);
}

TEST_F(PartialEquivalenceTest, ConstructionCheckerDifferentNumberOfQubits) {
  qc1.cswap(1, 0, 2);
  qc1.h(0);
  qc1.z(2);
  qc1.cswap(1, 0, 2);

  auto qc3 = qc::QuantumComputation(2);

  qc3.x(1);
  qc3.ch(1, 0);

  qc1.setLogicalQubitGarbage(2);
  qc1.setLogicalQubitGarbage(1);
  qc3.setLogicalQubitGarbage(1);

  config.execution.runConstructionChecker = true;
  ec::EquivalenceCheckingManager ecm(qc1, qc3, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST_F(PartialEquivalenceTest, MQTBenchGrover3Qubits) {
  const qc::QuantumComputation c1 = qasm3::Importer::importf(
      "./circuits/partialEquivalenceTest/"
      "grover-noancilla_nativegates_ibm_qiskit_opt0_3.qasm");
  const qc::QuantumComputation c2 = qasm3::Importer::importf(
      "./circuits/partialEquivalenceTest/grover-noancilla_indep_qiskit_3.qasm");

  c1.print(std::cout);
  c2.print(std::cout);
  // 3 measured qubits and 3 data qubits, full equivalence
  // construction checker
  config.execution.runConstructionChecker = true;
  ec::EquivalenceCheckingManager ecm(c1, c2, config);
  ecm.run();
  EXPECT_TRUE(ecm.getResults().consideredEquivalent());

  c1.print(std::cout);
  c2.print(std::cout);
  // alternating checker
  config.execution.runConstructionChecker = false;
  config.execution.runAlternatingChecker = true;
  ec::EquivalenceCheckingManager ecm2(c1, c2, config);
  ecm2.run();
  EXPECT_EQ(ecm2.equivalence(),
            ec::EquivalenceCriterion::EquivalentUpToGlobalPhase);
}

TEST_F(PartialEquivalenceTest, MQTBenchGrover7Qubits) {
  const qc::QuantumComputation c1 = qasm3::Importer::importf(
      "./circuits/partialEquivalenceTest/"
      "grover-noancilla_nativegates_ibm_qiskit_opt0_7.qasm");
  const qc::QuantumComputation c2 = qasm3::Importer::importf(
      "./circuits/partialEquivalenceTest/"
      "grover-noancilla_nativegates_ibm_qiskit_opt1_7.qasm");

  // 7 measured qubits and 7 data qubits, full equivalence
  // construction checker
  config.execution.runConstructionChecker = true;
  ec::EquivalenceCheckingManager ecm(c1, c2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);

  // alternating checker
  config.execution.runConstructionChecker = false;
  config.execution.runAlternatingChecker = true;
  ec::EquivalenceCheckingManager ecm2(c1, c2, config);
  ecm2.run();
  EXPECT_EQ(ecm2.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST_F(PartialEquivalenceTest, SliQECGrover22Qubits) {
  const qc::QuantumComputation c1 = qasm3::Importer::importf(
      "./circuits/partialEquivalenceTest/Grover_1.qasm"); // 11 qubits, 11 data
                                                          // qubits
  qc::QuantumComputation c2 = qasm3::Importer::importf(
      "./circuits/partialEquivalenceTest/Grover_2.qasm"); // 12 qubits, 11 data
                                                          // qubits

  // 11 measured qubits and 11 data qubits
  c2.setLogicalQubitAncillary(11);
  c2.setLogicalQubitGarbage(11);

  // construction checker
  config.execution.runConstructionChecker = true;
  ec::EquivalenceCheckingManager ecm(c1, c2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST_F(PartialEquivalenceTest, SliQECAdd19Qubits) {
  // full equivalence, 19 qubits
  // but this test uses algorithm for partial equivalence, not the "zero
  // ancillae" version
  qc::QuantumComputation c1 = qasm3::Importer::importf(
      "./circuits/partialEquivalenceTest/add6_196_1.qasm");
  qc::QuantumComputation c2 = qasm3::Importer::importf(
      "./circuits/partialEquivalenceTest/add6_196_2.qasm");

  // just for benchmarking reasons, we only measure 8 qubits
  c1.setLogicalQubitsAncillary(8, 18);
  c2.setLogicalQubitsAncillary(8, 18);
  c1.setLogicalQubitsGarbage(8, 18);
  c2.setLogicalQubitsGarbage(8, 18);

  config.execution.runConstructionChecker = true;
  ec::EquivalenceCheckingManager ecm(c1, c2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST_F(PartialEquivalenceTest, ExamplePaperDifferentQubitOrderAndNumber) {
  qc::QuantumComputation c1{4, 1};
  c1.cswap(1, 2, 0);
  c1.h(2);
  c1.z(0);
  c1.cswap(1, 2, 0);

  qc::QuantumComputation c2{3, 1};
  c2.x(1);
  c2.ch(1, 2);

  c1.setLogicalQubitsGarbage(0, 1);
  c1.setLogicalQubitGarbage(3);
  c1.setLogicalQubitAncillary(3);

  c2.setLogicalQubitsGarbage(0, 1);

  config.execution.runConstructionChecker = true;
  ec::EquivalenceCheckingManager ecm(qc1, qc2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST_F(PartialEquivalenceTest, AlternatingCheckerSliQEC19Qubits) {
  config.execution.runAlternatingChecker = true;

  // full equivalence, 10 qubits
  const qc::QuantumComputation c1 = qasm3::Importer::importf(
      "./circuits/partialEquivalenceTest/entanglement_1.qasm");
  const qc::QuantumComputation c2 = qasm3::Importer::importf(
      "./circuits/partialEquivalenceTest/entanglement_2.qasm");

  ec::EquivalenceCheckingManager ecm(c1, c2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);

  // full equivalence, 19 qubits
  const qc::QuantumComputation c3 = qasm3::Importer::importf(
      "./circuits/partialEquivalenceTest/add6_196_1.qasm");
  const qc::QuantumComputation c4 = qasm3::Importer::importf(
      "./circuits/partialEquivalenceTest/add6_196_2.qasm");

  ec::EquivalenceCheckingManager ecm2(c1, c2, config);
  ecm2.run();
  EXPECT_EQ(ecm2.equivalence(), ec::EquivalenceCriterion::Equivalent);

  // full equivalence, 10 qubits
  const qc::QuantumComputation c5 =
      qasm3::Importer::importf("./circuits/partialEquivalenceTest/bv_1.qasm");
  const qc::QuantumComputation c6 =
      qasm3::Importer::importf("./circuits/partialEquivalenceTest/bv_2.qasm");

  // calls zeroAncillaePartialEquivalenceCheck
  ec::EquivalenceCheckingManager ecm3(c1, c2, config);
  ecm3.run();
  EXPECT_EQ(ecm3.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST_F(PartialEquivalenceTest, AlternatingCheckerSliQECRandomCircuit) {
  config.execution.runAlternatingChecker = true;
  // full equivalence, 10 qubits
  const qc::QuantumComputation c1 = qasm3::Importer::importf(
      "./circuits/partialEquivalenceTest/random_1.qasm");
  const qc::QuantumComputation c2 = qasm3::Importer::importf(
      "./circuits/partialEquivalenceTest/random_2.qasm");

  ec::EquivalenceCheckingManager ecm(c1, c2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

TEST_F(PartialEquivalenceTest, ConstructionCheckerSliQECPeriodFinding8Qubits) {
  config.execution.runConstructionChecker = true;
  // 8 qubits, 3 data qubits
  qc::QuantumComputation c1 = qasm3::Importer::importf(
      "./circuits/partialEquivalenceTest/period_finding_1.qasm");
  // 8 qubits, 3 data qubits
  qc::QuantumComputation c2 = qasm3::Importer::importf(
      "./circuits/partialEquivalenceTest/period_finding_2.qasm");

  // 3 measured qubits and 3 data qubits

  c2.setLogicalQubitsAncillary(3, 7);
  c2.setLogicalQubitsGarbage(3, 7);

  c1.setLogicalQubitsAncillary(3, 7);
  c1.setLogicalQubitsGarbage(3, 7);

  ec::EquivalenceCheckingManager ecm(c1, c2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);
}

namespace {
void partialEquivalencCheckingBenchmarks(const qc::Qubit minN,
                                         const qc::Qubit maxN,
                                         const size_t reps,
                                         const bool addAncilla,
                                         const ec::Configuration& config) {
  std::mt19937 gen(55);

  for (qc::Qubit n = minN; n < maxN; n++) {
    double totalTime{0};
    std::size_t totalGates{0};
    for (size_t k = 0; k < reps; k++) {
      qc::Qubit d = n;
      if (addAncilla) {
        std::uniform_int_distribution<qc::Qubit> nrDataQubits(1, n);
        d = nrDataQubits(gen);
      }
      std::uniform_int_distribution<qc::Qubit> nrDataQubits(1, d);
      const qc::Qubit m = nrDataQubits(gen);

      const auto [c1, c2] = dd::generatePartiallyEquivalentCircuits(n, d, m);

      ec::EquivalenceCheckingManager ecm(c1, c2, config);
      ecm.run();
      EXPECT_TRUE(ecm.getResults().consideredEquivalent());

      const auto duration = ecm.getResults().checkTime;

      totalTime += duration;
      totalGates += c2.size();
    }
    std::cout << "\nnumber of qubits = " << n << "; number of reps = " << reps
              << "; average time = "
              << (totalTime / static_cast<double>(reps) /*/ 1000000.*/)
              << " seconds; average number of gates = "
              << (static_cast<double>(totalGates) / static_cast<double>(reps))
              << "\n";
  }
}
} // namespace

TEST_F(PartialEquivalenceTest, Benchmark) {
  config.execution.runConstructionChecker = true;
  const size_t minN = 2;
  const size_t maxN = 8;
  const size_t reps = 10;
  std::cout << "Partial equivalence check\n";
  partialEquivalencCheckingBenchmarks(minN, maxN, reps, true, config);
}

TEST_F(PartialEquivalenceTest, ZeroAncillaBenchmark) {
  config.execution.runAlternatingChecker = true;
  const size_t minN = 3;
  const size_t maxN = 14;
  const size_t reps = 10;
  std::cout << "Zero-ancilla partial equivalence check\n";
  partialEquivalencCheckingBenchmarks(minN, maxN, reps, false, config);
}

TEST_F(PartialEquivalenceTest, ConstructionCheckerPartiallyEquivalent) {
  // the first qubit has differing gates in the two circuits,
  // therefore they should not be equivalent if we only measure the first qubit
  qc2.h(0);
  qc2.h(0);
  qc2.h(1);
  qc2.h(1);
  qc1.tdg(0);
  qc1.tdg(1);

  qc2.h(0);
  qc2.h(0);
  qc2.h(1);
  qc2.h(1);

  config.execution.runConstructionChecker = true;
  ec::EquivalenceCheckingManager ecm(qc1, qc2, config);
  ecm.run();
  EXPECT_EQ(ecm.equivalence(), ec::EquivalenceCriterion::Equivalent);

  config.functionality.checkPartialEquivalence = false;
  ec::EquivalenceCheckingManager ecm2(qc1, qc2, config);
  ecm2.run();
  EXPECT_EQ(ecm2.equivalence(), ec::EquivalenceCriterion::NotEquivalent);
}
