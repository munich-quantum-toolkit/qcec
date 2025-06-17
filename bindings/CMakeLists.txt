# Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

if(APPLE)
  set(BASEPOINT @loader_path)
else()
  set(BASEPOINT $ORIGIN)
endif()
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
list(
  APPEND
  CMAKE_INSTALL_RPATH
  ${BASEPOINT}
  ${BASEPOINT}/${CMAKE_INSTALL_LIBDIR}
  ${BASEPOINT}/../core/${CMAKE_INSTALL_LIBDIR}
  ${BASEPOINT}/../core/lib
  ${BASEPOINT}/../core/lib64
  ${BASEPOINT}/../../core/${CMAKE_INSTALL_LIBDIR}
  ${BASEPOINT}/../../core/lib
  ${BASEPOINT}/../../core/lib64)

add_mqt_python_binding(
  QCEC
  ${MQT_QCEC_TARGET_NAME}-application_scheme-bindings
  application_scheme.cpp
  MODULE_NAME
  application_scheme
  INSTALL_DIR
  .
  LINK_LIBS
  MQT::QCEC
  pybind11_json
  MQT::ProjectOptions
  MQT::ProjectWarnings)

add_mqt_python_binding(
  QCEC
  ${MQT_QCEC_TARGET_NAME}-configuration-bindings
  configuration.cpp
  MODULE_NAME
  configuration
  INSTALL_DIR
  .
  LINK_LIBS
  MQT::QCEC
  pybind11_json
  MQT::ProjectOptions
  MQT::ProjectWarnings)

add_mqt_python_binding(
  QCEC
  ${MQT_QCEC_TARGET_NAME}-equivalence_checking_manager-bindings
  equivalence_checking_manager.cpp
  MODULE_NAME
  equivalence_checking_manager
  INSTALL_DIR
  .
  LINK_LIBS
  MQT::QCEC
  pybind11_json
  MQT::ProjectOptions
  MQT::ProjectWarnings)

add_mqt_python_binding(
  QCEC
  ${MQT_QCEC_TARGET_NAME}-equivalence_criterion-bindings
  equivalence_criterion.cpp
  MODULE_NAME
  equivalence_criterion
  INSTALL_DIR
  .
  LINK_LIBS
  MQT::QCEC
  pybind11_json
  MQT::ProjectOptions
  MQT::ProjectWarnings)

add_mqt_python_binding(
  QCEC
  ${MQT_QCEC_TARGET_NAME}-state_type-bindings
  state_type.cpp
  MODULE_NAME
  state_type
  INSTALL_DIR
  .
  INSTALL_DIR
  LINK_LIBS
  MQT::QCEC
  pybind11_json
  MQT::ProjectOptions
  MQT::ProjectWarnings)

# install the Python stub files in editable mode for better IDE support
if(SKBUILD_STATE STREQUAL "editable")
  file(GLOB PYI_FILES ${PROJECT_SOURCE_DIR}/python/mqt/qcec/*.pyi)
  install(
    FILES ${PYI_FILES}
    DESTINATION .
    COMPONENT ${MQT_QCEC_TARGET_NAME}_Python)
endif()
