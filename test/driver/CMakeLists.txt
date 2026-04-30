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

wasmedge_add_executable(wasmedgeDriverParseToolTests
  parseToolTest.cpp
)

add_test(NAME wasmedgeDriverParseToolTests
  COMMAND wasmedgeDriverParseToolTests
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(wasmedgeDriverParseToolTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedge_shared
)

wasmedge_add_executable(wasmedgeDriverValidateToolTests
  validateToolTest.cpp
)

add_test(NAME wasmedgeDriverValidateToolTests
  COMMAND wasmedgeDriverValidateToolTests
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(wasmedgeDriverValidateToolTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedge_shared
)

wasmedge_add_executable(wasmedgeDriverInstantiateToolTests
  instantiateToolTest.cpp
)

add_test(NAME wasmedgeDriverInstantiateToolTests
  COMMAND wasmedgeDriverInstantiateToolTests
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(wasmedgeDriverInstantiateToolTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedge_shared
)

wasmedge_add_executable(wasmedgeDriverCompileToolTests
  compileToolTest.cpp
)

add_test(NAME wasmedgeDriverCompileToolTests
  COMMAND wasmedgeDriverCompileToolTests
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(wasmedgeDriverCompileToolTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedge_shared
)
