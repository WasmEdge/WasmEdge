# SPDX-License-Identifier: Apache-2.0

wasmedge_add_executable(wasmedgeSignatureTests
  signatureTest.cpp
)

file(COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/signatureTestData
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(wasmedgeSignatureTests wasmedgeSignatureTests)

target_link_libraries(wasmedgeSignatureTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeSignature
)