# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeWasiCryptoTests
  WasiCryptoAeadsTest.cpp
  WasiCryptoAsymmetricTest.cpp
  WasiCryptoCommonTest.cpp
  WasiCryptoHashTest.cpp
  WasiCryptoKdfTest.cpp
  WasiCryptoKxTest.cpp
  WasiCryptoMacTest.cpp
  WasiCryptoNotImplementTest.cpp
  WasiCryptoSignaturesTest.cpp
  helper.cpp
)

add_dependencies(wasmedgeWasiCryptoTests
  wasmedgePluginWasiCrypto
)

target_compile_options(wasmedgeWasiCryptoTests
  PUBLIC
  -DOPENSSL_API_COMPAT=0x10100000L
)

target_include_directories(wasmedgeWasiCryptoTests
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:wasmedgePluginWasiCrypto,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgeWasiCryptoTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)
# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgeWasiCryptoTests
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgeWasiCryptoTests
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasmedgeWasiCryptoTests wasmedgeWasiCryptoTests)
