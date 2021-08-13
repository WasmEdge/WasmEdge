# SPDX-License-Identifier: Apache-2.0

wasmedge_add_executable(wasmedgeExternrefTests
  ExternrefTest.cpp
)

add_test(wasmedgeExternrefTests wasmedgeExternrefTests)

file(COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/externrefTestData
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(wasmedgeExternrefTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)
