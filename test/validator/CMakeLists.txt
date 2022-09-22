# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

file(COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/testData
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}
)

wasmedge_add_executable(wasmedgeValidatorComponentTests
  nested_core_module_types.cpp
)

add_test(wasmedgeValidatorComponentTests wasmedgeValidatorComponentTests)

target_link_libraries(wasmedgeValidatorComponentTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeLoader
  wasmedgeValidator
)
