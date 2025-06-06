# Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

if(NOT TARGET MQT::QCEC)
  # collect headers and source files
  file(GLOB_RECURSE QCEC_HEADERS ${MQT_QCEC_INCLUDE_BUILD_DIR}/*.hpp)
  file(GLOB_RECURSE QCEC_SOURCES **.cpp)

  # add QCEC Package library
  add_library(${PROJECT_NAME} ${QCEC_HEADERS} ${QCEC_SOURCES})

  # set include directories
  target_include_directories(${PROJECT_NAME}
                             PUBLIC $<BUILD_INTERFACE:${MQT_QCEC_INCLUDE_BUILD_DIR}>)

  # link to the MQT::Core libraries
  target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC MQT::CoreDD MQT::CoreZX
    PRIVATE MQT::CoreCircuitOptimizer MQT::CoreAlgorithms MQT::ProjectWarnings MQT::ProjectOptions)

  # add MQT alias
  add_library(MQT::QCEC ALIAS ${PROJECT_NAME})
endif()
