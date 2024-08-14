# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

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

add_dependencies(wasiCryptoTests
  wasmedgePluginWasiCrypto
)

target_compile_options(wasiCryptoTests
  PUBLIC
  -DOPENSSL_API_COMPAT=0x10100000L
)

target_include_directories(wasiCryptoTests
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:wasmedgePluginWasiCrypto,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasiCryptoTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)
# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasiCryptoTests
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasiCryptoTests
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasiCryptoTests wasiCryptoTests)
