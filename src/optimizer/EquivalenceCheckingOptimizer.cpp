/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "optimizer/EquivalenceCheckingOptimizer.hpp"

#include "ir/Definitions.hpp"
#include "ir/QuantumComputation.hpp"
#include "ir/operations/CompoundOperation.hpp"
#include "ir/operations/Control.hpp"
#include "ir/operations/IfElseOperation.hpp"
#include "ir/operations/NonUnitaryOperation.hpp"
#include "ir/operations/OpType.hpp"
#include "ir/operations/Operation.hpp"
#include "ir/operations/StandardOperation.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ec::detail {
using namespace qc;

namespace {
using DAG = std::vector<std::deque<std::unique_ptr<Operation>*>>;
using DAGReverseIterator =
    std::deque<std::unique_ptr<Operation>*>::reverse_iterator;
using DAGReverseIterators = std::vector<DAGReverseIterator>;

void addToDag(DAG& dag, std::unique_ptr<Operation>* op) {
  const auto usedQubits = (*op)->getUsedQubits();
  for (const auto q : usedQubits) {
    dag.at(q).push_back(op);
  }
}

void removeIdentities(QuantumComputation& qc) {
  auto it = qc.begin();
  while (it != qc.end()) {
    if ((*it)->getType() == I) {
      it = qc.erase(it);
    } else if ((*it)->isCompoundOperation()) {
      auto& compOp = dynamic_cast<CompoundOperation&>(**it);
      auto cit = compOp.cbegin();
      while (cit != compOp.cend()) {
        if ((*cit)->getType() == I) {
          cit = compOp.erase(cit);
        } else {
          ++cit;
        }
      }
      if (compOp.empty()) {
        it = qc.erase(it);
      } else {
        if (compOp.size() == 1) {
          // CompoundOperation has degraded to single Operation
          (*it) = std::move(*(compOp.begin()));
        }
        ++it;
      }
    } else {
      ++it;
    }
  }
}

DAG constructDAG(QuantumComputation& qc) {
  auto dag = DAG(qc.getHighestPhysicalQubitIndex() + 1);

  for (auto& op : qc) {
    addToDag(dag, &op);
  }
  return dag;
}

} // namespace

void singleQubitGateFusion(QuantumComputation& qc) {
  static const std::map<OpType, OpType> INVERSE_MAP = {
      {I, I},   {X, X},   {Y, Y},   {Z, Z},     {H, H},     {S, Sdg},
      {Sdg, S}, {T, Tdg}, {Tdg, T}, {SX, SXdg}, {SXdg, SX}, {Barrier, Barrier}};

  auto dag = DAG(qc.getHighestPhysicalQubitIndex() + 1U);

  for (auto& operation : qc) {
    if (!operation->isStandardOperation() || operation->isControlled() ||
        operation->getTargets().size() != 1U) {
      addToDag(dag, &operation);
      continue;
    }

    const auto target = operation->getTargets().at(0);
    if (dag.at(target).empty()) {
      addToDag(dag, &operation);
      continue;
    }

    auto* previous = dag.at(target).back();
    if (!(*previous)->isCompoundOperation() &&
        ((*previous)->isControlled() ||
         (*previous)->getTargets().size() != 1U)) {
      addToDag(dag, &operation);
      continue;
    }

    if ((*previous)->isCompoundOperation()) {
      auto* compound = dynamic_cast<CompoundOperation*>(previous->get());
      if (compound->getUsedQubits().size() > 1U) {
        addToDag(dag, &operation);
        continue;
      }

      if (compound->empty()) {
        compound->emplace_back(operation->clone());
        operation->setGate(I);
        continue;
      }

      const auto last = std::prev(compound->end());
      const auto inverse = INVERSE_MAP.find((*last)->getType());
      if (inverse != INVERSE_MAP.end() &&
          operation->getType() == inverse->second) {
        compound->pop_back();
      } else {
        compound->emplace_back<StandardOperation>(target, operation->getType(),
                                                  operation->getParameter());
      }
      operation->setGate(I);
      continue;
    }

    const auto inverse = INVERSE_MAP.find((*previous)->getType());
    if (inverse != INVERSE_MAP.end() &&
        operation->getType() == inverse->second) {
      (*previous)->setGate(I);
      operation->setGate(I);
      continue;
    }

    auto compound = std::make_unique<CompoundOperation>();
    compound->emplace_back<StandardOperation>((*previous)->getTargets().at(0),
                                              (*previous)->getType(),
                                              (*previous)->getParameter());
    compound->emplace_back<StandardOperation>(target, operation->getType(),
                                              operation->getParameter());
    operation->setGate(I);
    *previous = std::move(compound);
    dag.at(target).push_back(previous);
  }

  removeIdentities(qc);
}

void swapReconstruction(QuantumComputation& qc) {
  auto dag = DAG(qc.getHighestPhysicalQubitIndex() + 1);

  for (auto& it : qc) {
    if (!it->isStandardOperation()) {
      addToDag(dag, &it);
      continue;
    }

    // Operation is not a CNOT
    if (it->getType() != X || it->getNcontrols() != 1 ||
        it->getControls().begin()->type != Control::Type::Pos) {
      addToDag(dag, &it);
      continue;
    }

    const Qubit control = it->getControls().begin()->qubit;
    const Qubit target = it->getTargets().at(0);

    // first operation
    if (dag.at(control).empty() || dag.at(target).empty()) {
      addToDag(dag, &it);
      continue;
    }

    auto* opC = dag.at(control).back();
    auto* opT = dag.at(target).back();

    // previous operation is not a CNOT
    if ((*opC)->getType() != X || (*opC)->getNcontrols() != 1 ||
        (*opC)->getControls().begin()->type != Control::Type::Pos ||
        (*opT)->getType() != X || (*opT)->getNcontrols() != 1 ||
        (*opT)->getControls().begin()->type != Control::Type::Pos) {
      addToDag(dag, &it);
      continue;
    }

    const auto opControl = (*opC)->getControls().begin()->qubit;
    const auto opCtarget = (*opC)->getTargets().at(0);
    const auto opTcontrol = (*opT)->getControls().begin()->qubit;
    const auto opTtarget = (*opT)->getTargets().at(0);

    // operation at control and target qubit are not the same
    if (opControl != opTcontrol || opCtarget != opTtarget) {
      addToDag(dag, &it);
      continue;
    }

    if (control == opControl && target == opCtarget) {
      // elimination
      dag.at(control).pop_back();
      dag.at(target).pop_back();
      (*opC)->setGate(I);
      (*opC)->clearControls();
      it->setGate(I);
      it->clearControls();
    } else if (control == opCtarget && target == opControl) {
      dag.at(control).pop_back();
      dag.at(target).pop_back();

      // replace with SWAP + CNOT
      (*opC)->setGate(SWAP);
      if (target > control) {
        (*opC)->setTargets({control, target});
      } else {
        (*opC)->setTargets({target, control});
      }
      (*opC)->clearControls();
      addToDag(dag, opC);

      it->setTargets({control});
      it->setControls({Control{target}});
      addToDag(dag, &it);
    } else {
      addToDag(dag, &it);
    }
  }

  removeIdentities(qc);
}

namespace {
bool removeDiagonalGate(DAG& dag, DAGReverseIterators& dagIterators, Qubit idx,
                        DAGReverseIterator& it, Operation* op);

void removeDiagonalGatesBeforeMeasureRecursive(
    DAG& dag, DAGReverseIterators& dagIterators, Qubit idx,
    const Operation* until) {
  // qubit is finished -> consider next qubit
  if (dagIterators.at(idx) == dag.at(idx).rend()) {
    if (idx < static_cast<Qubit>(dag.size() - 1)) {
      removeDiagonalGatesBeforeMeasureRecursive(dag, dagIterators, idx + 1,
                                                nullptr);
    }
    return;
  }
  // check if desired operation was reached
  if (until != nullptr) {
    if ((*dagIterators.at(idx))->get() == until) {
      return;
    }
  }

  auto& it = dagIterators.at(idx);
  while (it != dag.at(idx).rend()) {
    // check if desired operation was reached
    if (until != nullptr) {
      if ((*dagIterators.at(idx))->get() == until) {
        break;
      }
    }
    auto* op = (*it)->get();
    if (op->isStandardOperation()) {
      // try removing gate and upon success increase all corresponding iterators
      auto onlyDiagonalGates =
          removeDiagonalGate(dag, dagIterators, idx, it, op);
      if (onlyDiagonalGates) {
        for (const auto& control : op->getControls()) {
          ++(dagIterators.at(control.qubit));
        }
        for (const auto& target : op->getTargets()) {
          ++(dagIterators.at(target));
        }
      }

    } else if (op->isCompoundOperation()) {
      // iterate over all gates of compound operation and upon success increase
      // all corresponding iterators
      auto* compOp = dynamic_cast<CompoundOperation*>(op);
      bool onlyDiagonalGates = true;
      auto cit = compOp->rbegin();
      while (cit != compOp->rend()) {
        auto* cop = cit->get();
        onlyDiagonalGates = removeDiagonalGate(dag, dagIterators, idx, it, cop);
        if (!onlyDiagonalGates) {
          break;
        }
        ++cit;
      }
      if (onlyDiagonalGates) {
        for (size_t q = 0; q < dag.size(); ++q) {
          if (compOp->actsOn(static_cast<Qubit>(q))) {
            ++(dagIterators.at(q));
          }
        }
      }
    } else if (op->isNonUnitaryOperation()) {
      // non-unitary operation is not diagonal
      it = dag.at(idx).rend();
    } else {
      throw std::runtime_error("Unexpected operation encountered");
    }
  }

  // qubit is finished -> consider next qubit
  if (dagIterators.at(idx) == dag.at(idx).rend() &&
      idx < static_cast<Qubit>(dag.size() - 1)) {
    removeDiagonalGatesBeforeMeasureRecursive(dag, dagIterators, idx + 1,
                                              nullptr);
  }
}

bool removeDiagonalGate(DAG& dag, DAGReverseIterators& dagIterators, Qubit idx,
                        DAGReverseIterator& it, Operation* op) {
  // not a diagonal gate
  if (!op->isDiagonalGate()) {
    it = dag.at(idx).rend();
    return false;
  }

  if (op->getNcontrols() != 0) {
    // need to check all controls and targets
    bool onlyDiagonalGates = true;
    for (const auto& control : op->getControls()) {
      auto controlQubit = control.qubit;
      if (controlQubit == idx) {
        continue;
      }
      if (control.type == Control::Type::Neg) {
        dagIterators.at(controlQubit) = dag.at(controlQubit).rend();
        onlyDiagonalGates = false;
        break;
      }
      if (dagIterators.at(controlQubit) == dag.at(controlQubit).rend()) {
        onlyDiagonalGates = false;
        break;
      }
      // recursive call at control with this operation as goal
      removeDiagonalGatesBeforeMeasureRecursive(dag, dagIterators, controlQubit,
                                                (*it)->get());
      // check if iteration of control qubit was successful
      if (*dagIterators.at(controlQubit) != *it) {
        onlyDiagonalGates = false;
        break;
      }
    }
    for (const auto& target : op->getTargets()) {
      if (target == idx) {
        continue;
      }
      if (dagIterators.at(target) == dag.at(target).rend()) {
        onlyDiagonalGates = false;
        break;
      }
      // recursive call at target with this operation as goal
      removeDiagonalGatesBeforeMeasureRecursive(dag, dagIterators, target,
                                                (*it)->get());
      // check if iteration of target qubit was successful
      if (*dagIterators.at(target) != *it) {
        onlyDiagonalGates = false;
        break;
      }
    }
    if (!onlyDiagonalGates) {
      // end qubit
      dagIterators.at(idx) = dag.at(idx).rend();
    } else {
      // set operation to identity so that it can be collected by the
      // removeIdentities pass
      op->setGate(I);
    }
    return onlyDiagonalGates;
  }
  // set operation to identity so that it can be collected by the
  // removeIdentities pass
  op->setGate(I);
  return true;
}
} // namespace

void removeDiagonalGatesBeforeMeasure(QuantumComputation& qc) {
  auto dag = constructDAG(qc);

  // initialize iterators
  DAGReverseIterators dagIterators{dag.size()};
  for (size_t q = 0; q < dag.size(); ++q) {
    if (dag.at(q).empty() || dag.at(q).back()->get()->getType() != Measure) {
      // qubit is not measured and thus does not have to be considered
      dagIterators.at(q) = dag.at(q).rend();
    } else {
      // point to operation before measurement
      dagIterators.at(q) = ++(dag.at(q).rbegin());
    }
  }
  // iterate over DAG in depth-first fashion
  removeDiagonalGatesBeforeMeasureRecursive(dag, dagIterators, 0, nullptr);

  // remove resulting identities from circuit
  removeIdentities(qc);
}

namespace {
void changeTargets(Targets& targets,
                   const std::map<Qubit, Qubit>& replacementMap) {
  for (auto& target : targets) {
    auto newTargetIt = replacementMap.find(target);
    if (newTargetIt != replacementMap.end()) {
      target = newTargetIt->second;
    }
  }
}

void changeControls(Controls& controls,
                    const std::map<Qubit, Qubit>& replacementMap) {
  if (controls.empty() || replacementMap.empty()) {
    return;
  }

  // iterate over the replacement map and see if any control matches
  for (const auto& [from, to] : replacementMap) {
    auto controlIt = controls.find(from);
    if (controlIt != controls.end()) {
      const auto controlType = controlIt->type;
      controls.erase(controlIt);
      controls.insert(Control{to, controlType});
    }
  }
}

void addConditionControl(Controls& controls, const Control condition) {
  if (const auto existing = controls.find(condition.qubit);
      existing != controls.end() && existing->type != condition.type) {
    throw std::runtime_error(
        "A classically controlled operation cannot use the measured qubit "
        "with the opposite quantum-control polarity.");
  }
  controls.emplace(condition);
}

[[noreturn]] void throwMeasuredQubitTargeted() {
  throw std::runtime_error(
      "Deferring a measurement past an operation targeting the measured "
      "qubit is not supported. Eliminate resets before deferring "
      "measurements and do not reuse measured qubits without a reset.");
}

void changeQubits(Operation& operation,
                  const std::map<Qubit, Qubit>& replacementMap) {
  if (auto* compound = dynamic_cast<CompoundOperation*>(&operation)) {
    changeControls(compound->getControls(), replacementMap);
    for (auto& nestedOperation : *compound) {
      changeQubits(*nestedOperation, replacementMap);
    }
    return;
  }

  if (auto* ifElse = dynamic_cast<IfElseOperation*>(&operation)) {
    changeQubits(*ifElse->getThenOp(), replacementMap);
    if (auto* elseOperation = ifElse->getElseOp()) {
      changeQubits(*elseOperation, replacementMap);
    }
    return;
  }

  changeTargets(operation.getTargets(), replacementMap);
  changeControls(operation.getControls(), replacementMap);
}

bool targetsQubit(const Operation& operation, const Qubit qubit) {
  if (const auto* compound =
          dynamic_cast<const CompoundOperation*>(&operation)) {
    return std::ranges::any_of(*compound, [&](const auto& nestedOperation) {
      return targetsQubit(*nestedOperation, qubit);
    });
  }

  if (const auto* ifElse = dynamic_cast<const IfElseOperation*>(&operation)) {
    return targetsQubit(*ifElse->getThenOp(), qubit) ||
           (ifElse->getElseOp() != nullptr &&
            targetsQubit(*ifElse->getElseOp(), qubit));
  }

  return std::ranges::find(operation.getTargets(), qubit) !=
         operation.getTargets().end();
}

bool containsReset(const Operation& operation) {
  if (operation.getType() == Reset) {
    return true;
  }
  if (const auto* compound =
          dynamic_cast<const CompoundOperation*>(&operation)) {
    return std::ranges::any_of(*compound, [](const auto& nestedOperation) {
      return containsReset(*nestedOperation);
    });
  }
  if (const auto* ifElse = dynamic_cast<const IfElseOperation*>(&operation)) {
    return containsReset(*ifElse->getThenOp()) ||
           (ifElse->getElseOp() != nullptr &&
            containsReset(*ifElse->getElseOp()));
  }
  return false;
}

bool containsConditionalReset(const Operation& operation) {
  if (const auto* ifElse = dynamic_cast<const IfElseOperation*>(&operation)) {
    return containsReset(*ifElse->getThenOp()) ||
           (ifElse->getElseOp() != nullptr &&
            containsReset(*ifElse->getElseOp()));
  }
  if (const auto* compound =
          dynamic_cast<const CompoundOperation*>(&operation)) {
    return std::ranges::any_of(*compound, [](const auto& nestedOperation) {
      return containsConditionalReset(*nestedOperation);
    });
  }
  return false;
}

void validateMeasurementMapping(const Operation& operation,
                                std::unordered_map<Qubit, Bit>& measuredQubits,
                                std::unordered_map<Bit, Qubit>& writtenBits) {
  if (const auto* measurement =
          dynamic_cast<const NonUnitaryOperation*>(&operation);
      measurement != nullptr && operation.getType() == Measure) {
    const auto& targets = measurement->getTargets();
    const auto& classics = measurement->getClassics();
    if (targets.size() != classics.size()) {
      throw std::runtime_error(
          "Measurement targets and classical bits must have equal sizes.");
    }
    for (std::size_t i = 0; i < targets.size(); ++i) {
      const auto [qubitIt, qubitInserted] =
          measuredQubits.try_emplace(targets[i], classics[i]);
      const auto [bitIt, bitInserted] =
          writtenBits.try_emplace(classics[i], targets[i]);
      if ((!qubitInserted && qubitIt->second != classics[i]) ||
          (!bitInserted && bitIt->second != targets[i])) {
        throw std::runtime_error(
            "Deferring measurements requires a one-to-one mapping between "
            "measured qubits and classical bits.");
      }
    }
    return;
  }
  if (const auto* compound =
          dynamic_cast<const CompoundOperation*>(&operation)) {
    for (const auto& nestedOperation : *compound) {
      validateMeasurementMapping(*nestedOperation, measuredQubits, writtenBits);
    }
    return;
  }
  if (const auto* ifElse = dynamic_cast<const IfElseOperation*>(&operation)) {
    if (ifElse->getThenOp() != nullptr) {
      validateMeasurementMapping(*ifElse->getThenOp(), measuredQubits,
                                 writtenBits);
    }
    if (ifElse->getElseOp() != nullptr) {
      validateMeasurementMapping(*ifElse->getElseOp(), measuredQubits,
                                 writtenBits);
    }
  }
}

Qubit addResetReplacement(QuantumComputation& qc) {
  const auto logical = static_cast<Qubit>(qc.getNqubits());
  const auto physical = qc.getHighestPhysicalQubitIndex() + 1U;
  qc.addQubit(logical, physical, logical);
  return physical;
}

template <class Container>
void eliminateResetsImpl(Container& operations, QuantumComputation& qc,
                         std::map<Qubit, Qubit>& replacementMap) {
  auto it = operations.begin();
  while (it != operations.end()) {
    if ((*it)->getType() == Reset) {
      for (const auto target : (*it)->getTargets()) {
        replacementMap.insert_or_assign(target, addResetReplacement(qc));
      }
      it = operations.erase(it);
      continue;
    }

    if (auto* compound = dynamic_cast<CompoundOperation*>(it->get())) {
      changeControls(compound->getControls(), replacementMap);
      eliminateResetsImpl(*compound, qc, replacementMap);
    } else if (!replacementMap.empty()) {
      changeQubits(**it, replacementMap);
    }
    ++it;
  }
}
} // namespace

void eliminateResets(QuantumComputation& qc) {
  //      ┌───┐┌─┐     ┌───┐┌─┐            ┌───┐┌─┐ ░
  // q_0: ┤ H ├┤M├─|0>─┤ H ├┤M├       q_0: ┤ H ├┤M├─░─────────
  //      └───┘└╥┘     └───┘└╥┘   -->      └───┘└╥┘ ░ ┌───┐┌─┐
  // c: 2/══════╩════════════╩═       q_1: ──────╫──░─┤ H ├┤M├
  //            0            1                   ║  ░ └───┘└╥┘
  //                                  c: 2/══════╩══════════╩═
  //                                             0          1
  if (std::ranges::any_of(qc, [](const auto& operation) {
        return containsConditionalReset(*operation);
      })) {
    throw std::runtime_error(
        "Eliminating resets inside classically controlled operations is not "
        "supported.");
  }
  if (qc.getNancillae() != 0U &&
      std::ranges::any_of(qc, [](const auto& operation) {
        return containsReset(*operation);
      })) {
    throw std::runtime_error(
        "Eliminating resets in circuits that already contain ancillary "
        "qubits is not supported.");
  }
  auto replacementMap = std::map<Qubit, Qubit>();
  eliminateResetsImpl(qc, qc, replacementMap);
}

void deferMeasurements(QuantumComputation& qc) {
  //      ┌───┐┌─┐                         ┌───┐     ┌─┐
  // q_0: ┤ H ├┤M├───────             q_0: ┤ H ├──■──┤M├
  //      └───┘└╥┘ ┌───┐                   └───┘┌─┴─┐└╥┘
  // q_1: ──────╫──┤ X ├─     -->     q_1: ─────┤ X ├─╫─
  //            ║  └─╥─┘                        └───┘ ║
  //            ║ ┌──╨──┐             c: 2/═══════════╩═
  // c: 2/══════╩═╡ = 1 ╞                             0
  //            0 └─────┘
  auto measuredQubits = std::unordered_map<Qubit, Bit>();
  auto writtenBits = std::unordered_map<Bit, Qubit>();
  for (const auto& operation : qc) {
    validateMeasurementMapping(*operation, measuredQubits, writtenBits);
  }

  if (std::ranges::any_of(qc, [](const auto& operation) {
        return operation->isCompoundOperation() &&
               operation->isNonUnitaryOperation();
      })) {
    qc.flattenOperations();
  }

  // Replacing an if-else operation can add at most one operation. Reserving
  // twice the current size therefore keeps all unaffected iterators valid.
  qc.reserve(qc.size() * 2U);

  std::unordered_map<Qubit, std::size_t> qubitsToAddMeasurements{};
  std::size_t operationIndex = 0;
  while (operationIndex < qc.size()) {
    if (const auto* measurement =
            dynamic_cast<NonUnitaryOperation*>(qc.at(operationIndex).get());
        measurement != nullptr && measurement->getType() == Measure) {
      const auto targets = measurement->getTargets();
      const auto classics = measurement->getClassics();

      if (targets.size() != 1 || classics.size() != 1) {
        throw std::runtime_error(
            "Deferring measurements with anything other than one target and "
            "one classical bit is not supported. Try decomposing your "
            "measurements.");
      }

      // if this is the last operation, nothing has to be done
      if (operationIndex + 1U == qc.size()) {
        break;
      }

      const auto measurementQubit = targets[0];
      const auto measurementBit = classics[0];

      // remember q->c for adding measurements later
      qubitsToAddMeasurements[measurementQubit] = measurementBit;

      // remove the measurement from the vector of operations
      auto opIt =
          qc.erase(qc.begin() + static_cast<std::ptrdiff_t>(operationIndex));

      // starting from the next operation after the measurement (if there is
      // any)
      auto currentInsertionPoint = opIt;
      bool measuredQubitTargeted = false;

      // iterate over all subsequent operations
      while (opIt != qc.end()) {
        const auto* operation = opIt->get();
        if (operation->isUnitary()) {
          if (targetsQubit(*operation, measurementQubit)) {
            measuredQubitTargeted = true;
          } else if (!measuredQubitTargeted) {
            ++currentInsertionPoint;
          }
          ++opIt;
          continue;
        }

        if (operation->getType() == Reset) {
          throw std::runtime_error(
              "Reset encountered in deferMeasurements routine. Please use the "
              "eliminateResets method before deferring measurements.");
        }

        if (const auto* measurement2 =
                dynamic_cast<NonUnitaryOperation*>(opIt->get());
            measurement2 != nullptr && operation->getType() == Measure) {
          const auto& targets2 = measurement2->getTargets();
          const auto& classics2 = measurement2->getClassics();
          // An identical later measurement is a breakpoint.
          if (targets == targets2 && classics == classics2) {
            if (measuredQubitTargeted) {
              throwMeasuredQubitTargeted();
            }
            const auto insertionIndex = static_cast<std::size_t>(
                std::distance(qc.begin(), currentInsertionPoint));
            qc.insert(currentInsertionPoint,
                      std::make_unique<NonUnitaryOperation>(targets, classics));
            qubitsToAddMeasurements.erase(measurementQubit);
            if (insertionIndex == operationIndex) {
              ++operationIndex;
            }
            break;
          }

          if (!measuredQubitTargeted) {
            ++currentInsertionPoint;
          }
          ++opIt;
          continue;
        }

        if (auto* ifElse = dynamic_cast<IfElseOperation*>(opIt->get());
            ifElse != nullptr) {
          // determine control bit
          std::uint64_t expectedValue = 0U;
          Bit cBit = 0;
          if (const auto& controlRegister = ifElse->getControlRegister();
              controlRegister.has_value()) {
            assert(!ifElse->getControlBit().has_value());
            expectedValue = ifElse->getExpectedValueRegister();
            if (controlRegister->getSize() != 1) {
              throw std::runtime_error(
                  "If-else operations controlled by more than one classical "
                  "bit are currently not supported. Try decomposing the "
                  "operation into individual contributions.");
            }
            cBit = controlRegister->getStartIndex();
          }
          if (const auto& controlBit = ifElse->getControlBit();
              controlBit.has_value()) {
            assert(!ifElse->getControlRegister().has_value());
            expectedValue = ifElse->getExpectedValueBit() ? 1U : 0U;
            cBit = controlBit.value();
          }

          // continue if the control bit is not the bit being measured
          if (cBit != measurementBit) {
            if (targetsQubit(*operation, measurementQubit)) {
              measuredQubitTargeted = true;
            } else if (!measuredQubitTargeted) {
              ++currentInsertionPoint;
            }
            ++opIt;
            continue;
          }

          if (ifElse->getComparisonKind() != Eq || expectedValue > 1U) {
            throw std::runtime_error(
                "Deferring measurements only supports equality comparisons "
                "of one classical bit against zero or one.");
          }

          if (measuredQubitTargeted) {
            throwMeasuredQubitTargeted();
          }

          // determine the appropriate control to add
          const auto controlQubit = measurementQubit;
          const auto thenControlType =
              (expectedValue == 1U) ? Control::Type::Pos : Control::Type::Neg;
          const auto elseControlType =
              (expectedValue == 1U) ? Control::Type::Neg : Control::Type::Pos;

          // modify the then-operation
          auto* thenOp = ifElse->getThenOp();
          const auto* standardThenOp = dynamic_cast<StandardOperation*>(thenOp);
          if (standardThenOp == nullptr) {
            std::stringstream ss{};
            ss << "The then-operation of the if-else operation is not a "
                  "StandardOperation.\n";
            thenOp->print(ss, qc.getNqubits());
            throw std::runtime_error(ss.str());
          }

          const auto thenTargets = standardThenOp->getTargets();
          for (const auto& thenTarget : thenTargets) {
            if (thenTarget == measurementQubit) {
              throw std::runtime_error(
                  "Implicit reset operation in circuit detected. Measuring a "
                  "qubit and then targeting the same qubit with an if-else "
                  "operation is currently not supported.");
            }
          }
          auto thenControls = standardThenOp->getControls();
          addConditionControl(thenControls,
                              Control{controlQubit, thenControlType});
          const auto thenType = standardThenOp->getType();
          const auto thenParameters = standardThenOp->getParameter();

          // modify the else-operation
          auto* elseOp = ifElse->getElseOp();
          Controls elseControls;
          Targets elseTargets;
          OpType elseType = None;
          std::vector<fp> elseParameters;
          if (elseOp != nullptr) {
            const auto* standardElseOp =
                dynamic_cast<StandardOperation*>(elseOp);
            if (standardElseOp == nullptr) {
              std::stringstream ss{};
              ss << "The else-operation of the if-else operation is not a "
                    "StandardOperation.\n";
              thenOp->print(ss, qc.getNqubits());
              throw std::runtime_error(ss.str());
            }

            elseTargets = standardElseOp->getTargets();
            for (const auto& elseTarget : elseTargets) {
              if (elseTarget == measurementQubit) {
                throw std::runtime_error(
                    "Implicit reset operation in circuit detected. Measuring a "
                    "qubit and then targeting the same qubit with an if-else "
                    "operation is currently not supported.");
              }
            }
            elseControls = standardElseOp->getControls();
            addConditionControl(elseControls,
                                Control{controlQubit, elseControlType});
            elseType = standardElseOp->getType();
            elseParameters = standardElseOp->getParameter();
          }

          // Remove the if-else operation and preserve the insertion point if
          // it precedes the erased operation.
          const auto insertionPointInvalidated =
              (currentInsertionPoint >= opIt);

          opIt = qc.erase(opIt);

          if (insertionPointInvalidated) {
            currentInsertionPoint = opIt;
          }

          // insert the new operations
          currentInsertionPoint = qc.insert(
              currentInsertionPoint,
              std::make_unique<StandardOperation>(thenControls, thenTargets,
                                                  thenType, thenParameters));
          if (elseOp != nullptr) {
            ++currentInsertionPoint;
            currentInsertionPoint = qc.insert(
                currentInsertionPoint,
                std::make_unique<StandardOperation>(elseControls, elseTargets,
                                                    elseType, elseParameters));
          }

          // advance just after the currently inserted operation
          ++currentInsertionPoint;
          // the inner loop also has to restart from here due to the
          // invalidation of the iterators
          opIt = currentInsertionPoint;
          continue;
        }

        throw std::runtime_error(
            "Unsupported non-unitary operation encountered while deferring "
            "measurements.");
      }
      if (measuredQubitTargeted) {
        throwMeasuredQubitTargeted();
      }
      continue;
    }
    ++operationIndex;
  }
  if (!qubitsToAddMeasurements.empty()) {
    qc.outputPermutation.clear();
    for (const auto& [qubit, clbit] : qubitsToAddMeasurements) {
      qc.measure(qubit, clbit);
    }
    qc.initializeIOMapping();
  }

  if (!qc.empty() && qc.isDynamic()) {
    throw std::runtime_error(
        "Measurement deferral left unsupported dynamic operations in the "
        "circuit.");
  }
}

namespace {
using ConstReverseIterator = QuantumComputation::const_reverse_iterator;
void backpropagateOutputPermutationImpl(
    const ConstReverseIterator& rbegin, const ConstReverseIterator& rend,
    Permutation& permutation, std::unordered_set<Qubit>& missingLogicalQubits) {
  for (auto it = rbegin; it != rend; ++it) {
    if ((*it)->isCompoundOperation()) {
      auto& op = dynamic_cast<CompoundOperation&>(**it);
      backpropagateOutputPermutationImpl(op.crbegin(), op.crend(), permutation,
                                         missingLogicalQubits);
      continue;
    }

    if ((*it)->getType() == SWAP && !(*it)->isControlled() &&
        (*it)->getTargets().size() == 2U) {
      const auto& targets = (*it)->getTargets();
      // four cases
      // 1. both targets are in the permutation
      // 2. only the first target is in the permutation
      // 3. only the second target is in the permutation
      // 4. neither target is in the permutation

      const auto it0 = permutation.find(targets[0]);
      const auto it1 = permutation.find(targets[1]);

      if (it0 != permutation.end() && it1 != permutation.end()) {
        // case 1: swap the entries
        std::swap(it0->second, it1->second);
        continue;
      }

      if (it0 != permutation.end()) {
        // case 2: swap the value assign the other target from the list of
        // missing logical qubits. Give preference to choosing the same logical
        // qubit as the missing physical qubit
        permutation[targets[1]] = it0->second;

        if (missingLogicalQubits.contains(targets[0])) {
          missingLogicalQubits.erase(targets[0]);
          it0->second = targets[0];
        } else {
          it0->second = *missingLogicalQubits.begin();
          missingLogicalQubits.erase(missingLogicalQubits.begin());
        }
        continue;
      }

      if (it1 != permutation.end()) {
        // case 3: swap the value assign the other target from the list of
        // missing logical qubits. Give preference to choosing the same logical
        // qubit as the missing physical qubit
        permutation[targets[0]] = it1->second;

        if (missingLogicalQubits.contains(targets[1])) {
          missingLogicalQubits.erase(targets[1]);
          it1->second = targets[1];
        } else {
          it1->second = *missingLogicalQubits.begin();
          missingLogicalQubits.erase(missingLogicalQubits.begin());
        }
        continue;
      }

      // case 4: nothing to do
    }
  }
}
} // namespace

void backpropagateOutputPermutation(QuantumComputation& qc) {
  auto permutation = qc.outputPermutation;

  // Collect all logical qubits missing from the output permutation
  std::unordered_set<Qubit> logicalQubits{};
  for (const auto& [physical, logical] : permutation) {
    logicalQubits.insert(logical);
  }
  std::unordered_set<Qubit> missingLogicalQubits{};
  for (Qubit i = 0; i < qc.getNqubits(); ++i) {
    if (!logicalQubits.contains(i)) {
      missingLogicalQubits.emplace(i);
    }
  }

  backpropagateOutputPermutationImpl(qc.crbegin(), qc.crend(), permutation,
                                     missingLogicalQubits);

  // `permutation` now holds a potentially incomplete initial layout
  // check whether the initial layout is complete and return if it is
  if (permutation.size() == qc.getNqubits()) {
    qc.initialLayout = permutation;
    return;
  }

  // Otherwise, fill the initial layout with the missing logical qubits.
  // Give preference to choosing the same logical qubit as the missing physical
  // qubit (i.e., an identity mapping) to avoid unnecessary permutations.
  for (Qubit i = 0; i < qc.getNqubits(); ++i) {
    if (permutation.find(i) == permutation.end()) {
      if (missingLogicalQubits.contains(i)) {
        permutation.emplace(i, i);
        missingLogicalQubits.erase(i);
      } else {
        permutation.emplace(i, *missingLogicalQubits.begin());
        missingLogicalQubits.erase(missingLogicalQubits.begin());
      }
    }
  }
  assert(missingLogicalQubits.empty());
  qc.initialLayout = permutation;
}

namespace {
template <class Container>
void elidePermutationsImpl(Container& container, Permutation& permutation) {
  for (auto it = container.begin(); it != container.end();) {
    auto& op = *it;
    if (auto* compOp = dynamic_cast<CompoundOperation*>(op.get())) {
      elidePermutationsImpl(*compOp, permutation);
      if (compOp->empty()) {
        it = container.erase(it);
        continue;
      }
      if (compOp->isConvertibleToSingleOperation()) {
        *it = compOp->collapseToSingleOperation();
      } else {
        // also update the tracked controls in the compound operation
        compOp->getControls() = permutation.apply(compOp->getControls());
      }
      ++it;
      continue;
    }

    if (op->getType() == SWAP && !op->isControlled()) {
      const auto& targets = op->getTargets();
      assert(targets.size() == 2U);
      assert(permutation.find(targets[0]) != permutation.end());
      assert(permutation.find(targets[1]) != permutation.end());
      auto& target0 = permutation[targets[0]];
      auto& target1 = permutation[targets[1]];
      std::swap(target0, target1);
      it = container.erase(it);
      continue;
    }

    op->apply(permutation);
    ++it;
  }
}
} // namespace

void elidePermutations(QuantumComputation& qc) {
  if (qc.empty()) {
    return;
  }

  auto permutation = qc.initialLayout;
  elidePermutationsImpl(qc, permutation);

  // adjust the initial layout
  Permutation initialLayout{};
  for (auto& [physical, logical] : qc.initialLayout) {
    initialLayout[logical] = logical;
  }
  qc.initialLayout = initialLayout;

  // adjust the output permutation
  Permutation outputPermutation{};
  for (auto& [physical, logical] : qc.outputPermutation) {
    assert(permutation.find(physical) != permutation.end());
    outputPermutation[permutation[physical]] = logical;
  }
  qc.outputPermutation = outputPermutation;
}

} // namespace ec::detail
