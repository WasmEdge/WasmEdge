# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_executable(wasiCryptoTests 
  aeads.cpp
  asymmetric.cpp
  common.cpp
  hash.cpp
  helper.cpp
  kdf.cpp
  kx.cpp
  mac.cpp
  notimplement.cpp
  signatures.cpp
)

target_link_libraries(wasiCryptoTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgePlugin
  wasmedgePluginWasiCrypto
)

add_test(wasiCryptoTests wasiCryptoTests)
