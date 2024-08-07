# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(spanTests
  span.cpp
)

add_test(spanTests spanTests)

target_link_libraries(spanTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)

target_include_directories(spanTests
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)
