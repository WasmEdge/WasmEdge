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

add_test(wasiCryptoTests wasiCryptoTests)

target_link_libraries(wasiCryptoTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeHostModuleWasiCrypto
)
