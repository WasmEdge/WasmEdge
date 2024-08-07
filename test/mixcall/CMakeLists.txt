# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeMixcallTests
  mixcallTest.cpp
)

add_test(wasmedgeMixcallTests wasmedgeMixcallTests)

file(COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/mixcallTestData
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(wasmedgeMixcallTests
  PRIVATE
  std::filesystem
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeLLVM
  wasmedgeVM
)
