# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_executable(wasiCryptoTests 
  helper.cpp
  common.cpp
  hash.cpp
  mac.cpp
  kdf.cpp
  aeads.cpp
  asymmetric.cpp
  kx.cpp
  signatures.cpp
  notimplement.cpp
)

add_test(wasiCryptoTests wasiCryptoTests)

target_link_libraries(wasiCryptoTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeHostModuleWasiCrypto
)
