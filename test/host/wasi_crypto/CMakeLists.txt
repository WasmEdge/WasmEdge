# SPDX-License-Identifier: Apache-2.0

wasmedge_add_executable(wasiCryptoTests
  symmetric/symmetric.cpp
  asymmetric/kx.cpp
  asymmetric/eddsa.cpp
  asymmetric/rsa.cpp
  asymmetric/ecdsa.cpp
)

add_test(wasiCryptoTests wasiCryptoTests)

target_link_libraries(wasiCryptoTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeHostModuleWasiCrypto
)
