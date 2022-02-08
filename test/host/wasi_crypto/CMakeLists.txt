# SPDX-License-Identifier: Apache-2.0

wasmedge_add_executable(wasiCryptoTests
  x25519.cpp
  eddsa.cpp
  ecdsa.cpp
  helper.cpp
  sha2.cpp
  hmac.cpp
  hkdf.cpp
  aes_gcm.cpp
  chacha_poly.cpp
  common.cpp
  rsa.cpp
)

add_test(wasiCryptoTests wasiCryptoTests)

target_link_libraries(wasiCryptoTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeHostModuleWasiCrypto
)
