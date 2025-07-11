/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#pragma once

#include "Configuration.hpp"
#include "EquivalenceCriterion.hpp"
#include "TaskManager.hpp"
#include "applicationscheme/ApplicationScheme.hpp"
#include "checker/EquivalenceChecker.hpp"
#include "dd/DDpackageConfig.hpp"
#include "dd/Package.hpp"
#include "ir/QuantumComputation.hpp"

#include <cstddef>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <utility>

namespace ec {
template <class DDType> class DDEquivalenceChecker : public EquivalenceChecker {
public:
  DDEquivalenceChecker(
      const qc::QuantumComputation& circ1, const qc::QuantumComputation& circ2,
      Configuration config,
      const dd::DDPackageConfig& packageConfig = dd::DDPackageConfig{})
      : EquivalenceChecker(circ1, circ2, std::move(config)),
        dd(std::make_unique<dd::Package>(nqubits, packageConfig)),
        taskManager1(TaskManager<DDType>(circ1, *dd)),
        taskManager2(TaskManager<DDType>(circ2, *dd)) {}

  EquivalenceCriterion run() override;

protected:
  std::unique_ptr<dd::Package> dd;

  TaskManager<DDType> taskManager1;
  TaskManager<DDType> taskManager2;

  std::unique_ptr<ApplicationScheme<DDType>> applicationScheme;

  void initializeApplicationScheme(ApplicationSchemeType scheme);

  // at some point this routine should probably make its way into the DD package
  // in some form
  EquivalenceCriterion equals(const DDType& e, const DDType& f);

  virtual void initializeTask(TaskManager<DDType>& taskManager);
  virtual void initialize();
  virtual void execute();
  virtual void finish();
  virtual void postprocessTask(TaskManager<DDType>& task);
  virtual void postprocess();
  virtual EquivalenceCriterion checkEquivalence();
};

} // namespace ec
