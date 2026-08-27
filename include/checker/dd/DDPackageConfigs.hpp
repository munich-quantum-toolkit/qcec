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

#include "dd/DDpackageConfig.hpp"

#include <cstddef>

namespace ec {
struct SimulationDDPackageConfig : public dd::DDPackageConfig {
  // simulation requires more resources for vectors.
  static constexpr std::size_t UT_VEC_NBUCKET = 65'536U;
  static constexpr std::size_t CT_VEC_ADD_NBUCKET = 65'536U;
  static constexpr std::size_t CT_MAT_VEC_MULT_NBUCKET = 65'536U;
  static constexpr std::size_t CT_VEC_INNER_PROD_NBUCKET = 32'768U;

  // simulation only needs matrices for representing operations. Hence, very
  // little is needed here.
  static constexpr std::size_t UT_MAT_NBUCKET = 128U;
  static constexpr std::size_t UT_MAT_INITIAL_ALLOCATION_SIZE = 32U;

  // simulation needs no matrix addition, conjugate transposition, matrix-matrix
  // multiplication, or kronecker products.
  static constexpr std::size_t CT_MAT_ADD_NBUCKET = 1U;
  static constexpr std::size_t CT_MAT_CONJ_TRANS_NBUCKET = 1U;
  static constexpr std::size_t CT_MAT_MAT_MULT_NBUCKET = 1U;
  static constexpr std::size_t CT_VEC_KRON_NBUCKET = 1U;
  static constexpr std::size_t CT_MAT_KRON_NBUCKET = 1U;
};

struct ConstructionDDPackageConfig : public dd::DDPackageConfig {
  // construction requires more resources for matrices.
  static constexpr std::size_t UT_MAT_NBUCKET = 65'536U;
  static constexpr std::size_t CT_MAT_ADD_NBUCKET = 65'536U;
  static constexpr std::size_t CT_MAT_MAT_MULT_NBUCKET = 65'536U;
  static constexpr std::size_t CT_MAT_CONJ_TRANS_NBUCKET = 32'768U;

  // construction does not need any vector nodes
  static constexpr std::size_t UT_VEC_NBUCKET = 1U;
  static constexpr std::size_t UT_VEC_INITIAL_ALLOCATION_SIZE = 1U;

  // construction needs no vector addition, matrix-vector multiplication,
  // kronecker products, or inner products.
  static constexpr std::size_t CT_VEC_ADD_NBUCKET = 1U;
  static constexpr std::size_t CT_MAT_VEC_MULT_NBUCKET = 1U;
  static constexpr std::size_t CT_VEC_KRON_NBUCKET = 1U;
  static constexpr std::size_t CT_MAT_KRON_NBUCKET = 1U;
  static constexpr std::size_t CT_VEC_INNER_PROD_NBUCKET = 1U;
};

struct AlternatingDDPackageConfig : public dd::DDPackageConfig {
  // The alternating checker requires more resources for matrices.
  static constexpr std::size_t UT_MAT_NBUCKET = 65'536U;
  static constexpr std::size_t CT_MAT_ADD_NBUCKET = 65'536U;
  static constexpr std::size_t CT_MAT_MAT_MULT_NBUCKET = 65'536U;

  // The alternating checker does not need any vector nodes
  static constexpr std::size_t UT_VEC_NBUCKET = 1U;
  static constexpr std::size_t UT_VEC_INITIAL_ALLOCATION_SIZE = 1U;

  // The alternating needs no vector addition, matrix-vector multiplication,
  // kronecker products, or inner products.
  static constexpr std::size_t CT_VEC_ADD_NBUCKET = 1U;
  static constexpr std::size_t CT_MAT_VEC_MULT_NBUCKET = 1U;
  static constexpr std::size_t CT_VEC_KRON_NBUCKET = 1U;
  static constexpr std::size_t CT_MAT_KRON_NBUCKET = 1U;
  static constexpr std::size_t CT_VEC_INNER_PROD_NBUCKET = 1U;
};
} // namespace ec
