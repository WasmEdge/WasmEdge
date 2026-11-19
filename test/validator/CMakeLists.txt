# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeValidatorTests
  ValidatorRegressionTest.cpp
)

add_test(wasmedgeValidatorTests wasmedgeValidatorTests)

target_link_libraries(wasmedgeValidatorTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)
