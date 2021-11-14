# SPDX-License-Identifier: Apache-2.0

wasmedge_add_executable(wasiCryptoTests
  symmetric.cpp
  kx.cpp
  signature.cpp
)

add_test(wasiCryptoTests wasiCryptoTests)

target_link_libraries(wasiCryptoTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeHostModuleWasiCrypto
)
