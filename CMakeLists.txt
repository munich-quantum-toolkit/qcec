# Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

# set required cmake version
cmake_minimum_required(VERSION 3.24...4.0)

project(
  mqt-qcec
  LANGUAGES C CXX
  DESCRIPTION "MQT QCEC - A tool for Quantum Circuit Equivalence Checking")

option(BUILD_MQT_QCEC_BINDINGS "Build the MQT QCEC Python bindings" OFF)
if(BUILD_MQT_QCEC_BINDINGS)
  # ensure that the BINDINGS option is set
  set(BINDINGS
      ON
      CACHE INTERNAL "Enable settings related to Python bindings")
  # Some common settings for finding Python
  set(Python_FIND_VIRTUALENV
      FIRST
      CACHE STRING "Give precedence to virtualenvs when searching for Python")
  set(Python_FIND_FRAMEWORK
      LAST
      CACHE STRING "Prefer Brew/Conda to Apple framework Python")
  set(Python_ARTIFACTS_INTERACTIVE
      ON
      CACHE BOOL "Prevent multiple searches for Python and instead cache the results.")

  if(DISABLE_GIL)
    message(STATUS "Disabling Python GIL")
    add_compile_definitions(Py_GIL_DISABLED)
  endif()

  # top-level call to find Python
  find_package(
    Python 3.9 REQUIRED
    COMPONENTS Interpreter Development.Module
    OPTIONAL_COMPONENTS Development.SABIModule)
endif()

# check if this is the master project or used via add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(MQT_QCEC_MASTER_PROJECT ON)
else()
  set(MQT_QCEC_MASTER_PROJECT OFF)
endif()

option(BUILD_MQT_QCEC_TESTS "Also build tests for the MQT QCEC project" ${MQT_QCEC_MASTER_PROJECT})

include(cmake/ExternalDependencies.cmake)

# set the include directory for the build tree
set(MQT_QCEC_INCLUDE_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

# set prefix for all MQT QCEC targets
set(MQT_QCEC_TARGET_NAME mqt-qcec)

# add main library code
add_subdirectory(src)

if(BUILD_MQT_QCEC_BINDINGS)
  add_subdirectory(bindings)
endif()

# add test code
if(BUILD_MQT_QCEC_TESTS)
  enable_testing()
  include(GoogleTest)
  add_subdirectory(test)
endif()

if(MQT_QCEC_MASTER_PROJECT)
  if(NOT TARGET mqt-qcec-uninstall)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
                   ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake IMMEDIATE @ONLY)
    add_custom_target(mqt-qcec-uninstall COMMAND ${CMAKE_COMMAND} -P
                                                 ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
  endif()
else()
  set(mqt-qcec_FOUND
      TRUE
      CACHE INTERNAL "True if mqt-qcec is found on the system")
endif()
