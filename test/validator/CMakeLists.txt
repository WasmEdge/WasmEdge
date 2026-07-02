# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeValidatorRegressionTests
  ValidatorRegressionTest.cpp
)

add_test(wasmedgeValidatorRegressionTests wasmedgeValidatorRegressionTests)

target_link_libraries(wasmedgeValidatorRegressionTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)
