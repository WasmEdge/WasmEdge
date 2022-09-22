# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC
file(COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/testData
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}
)

wasmedge_add_executable(wasmedgeExecutorComponentTests
  initTest.cpp
)

add_test(wasmedgeExecutorComponentTests wasmedgeExecutorComponentTests)

target_link_libraries(wasmedgeExecutorComponentTests
  PRIVATE
  std::filesystem
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeLoader
  wasmedgeVM
)
