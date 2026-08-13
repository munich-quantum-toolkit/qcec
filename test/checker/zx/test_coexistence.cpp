/*
 * Copyright (c) 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "checker/zx/ZXDiagram.hpp"
#include "zx/ZXDiagram.hpp"

#include <gtest/gtest.h>

TEST(ZXCoexistenceTest, CoreAndQCECDiagramsUseDistinctSymbols) {
  const ::zx::ZXDiagram coreDiagram(1);
  const ec::zx::ZXDiagram qcecDiagram(1);

  EXPECT_EQ(coreDiagram.getNQubits(), 1);
  EXPECT_EQ(qcecDiagram.getNQubits(), 1);
}
