# Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

# collect all test files
file(GLOB_RECURSE TEST_FILES "*.cpp")

# add test executable
package_add_test(mqt-qcec-test MQT::QCEC ${TEST_FILES})

# link additional test dependencies
target_link_libraries(mqt-qcec-test PRIVATE Boost::multiprecision MQT::CoreAlgorithms MQT::CoreQASM)
target_include_directories(mqt-qcec-test SYSTEM PRIVATE ${MQT_QCEC_BOOST_INCLUDE_DIRS})
