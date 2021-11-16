# SPDX-License-Identifier: Apache-2.0

wasmedge_add_executable(wasmedgeSignatureTests
  signatureTest.cpp
)

add_test(wasmedgeSignatureTests wasmedgeSignatureTests)

target_link_libraries(wasmedgeSignatureTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeSignature
)