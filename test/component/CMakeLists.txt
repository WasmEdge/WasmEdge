# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

file(COPY
  data
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}
)

wasmedge_add_executable(componentTests
  spectest.cpp
)

add_test(componentTests componentTests)

target_link_libraries(componentTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedge_shared
)

target_include_directories(componentTests
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)
