# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeValidatorRegressionTests
  ValidatorRegressionTest.cpp
)

add_test(wasmedgeValidatorRegressionTests wasmedgeValidatorRegressionTests)

target_link_libraries(wasmedgeValidatorRegressionTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)
