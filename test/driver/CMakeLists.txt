# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

file(COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/driverTestData
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}
)

wasmedge_add_executable(wasmedgeDriverToolTests
  driverToolTest.cpp
)

add_test(NAME wasmedgeDriverToolTests
  COMMAND wasmedgeDriverToolTests
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(wasmedgeDriverToolTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedge_shared
)
