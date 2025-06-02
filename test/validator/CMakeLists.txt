# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeValidatorSubtypeTests
  ValidatorSubtypeTest.cpp
)

add_test(wasmedgeValidatorSubtypeTests wasmedgeValidatorSubtypeTests)

target_link_libraries(wasmedgeValidatorSubtypeTests
  PRIVATE
  wasmedgeTestSpec
  wasmedgeVM
)
