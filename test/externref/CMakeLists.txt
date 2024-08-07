# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeExternrefTests
  ExternrefTest.cpp
)

add_test(wasmedgeExternrefTests wasmedgeExternrefTests)

file(COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/externrefTestData
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(wasmedgeExternrefTests
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(wasmedgeExternrefTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  std::filesystem
  wasmedge_shared
)
