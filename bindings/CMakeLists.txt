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

file(GLOB_RECURSE QCEC_SOURCES **.cpp)

add_mqt_python_binding(
  QCEC
  ${MQT_QCEC_TARGET_NAME}-bindings
  ${QCEC_SOURCES}
  MODULE_NAME
  pyqcec
  INSTALL_DIR
  .
  LINK_LIBS
  MQT::QCEC
  pybind11_json)

# install the Python stub files in editable mode for better IDE support
if(SKBUILD_STATE STREQUAL "editable")
  install(
    FILES ${PROJECT_SOURCE_DIR}/python/mqt/qcec/pyqcec.pyi
    DESTINATION .
    COMPONENT ${MQT_QCEC_TARGET_NAME}_Python)
endif()
