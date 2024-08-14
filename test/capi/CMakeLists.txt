# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(capiTests
  spectest.cpp
)

add_test(capiTests capiTests)

target_link_libraries(capiTests
  PRIVATE
  wasmedgeCAPI
  ${GTEST_BOTH_LIBRARIES}
)

target_include_directories(capiTests
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)
