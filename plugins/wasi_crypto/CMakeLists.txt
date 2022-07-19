# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

set(OPENSSL_USE_STATIC_LIBS ON)
find_package(OpenSSL REQUIRED)

wasmedge_add_library(wasmedgePluginWasiCrypto
  SHARED
  module.cpp
  ctx.cpp
  asymmetric_common/ctx.cpp
  asymmetric_common/func.cpp
  asymmetric_common/keypair.cpp
  asymmetric_common/publickey.cpp
  asymmetric_common/secretkey.cpp
  common/array_output.cpp
  common/ctx.cpp
  common/func.cpp
  common/options.cpp
  kx/ctx.cpp
  kx/dh/ecdsa.cpp
  kx/dh/x25519.cpp
  kx/func.cpp
  kx/kx.cpp
  kx/options.cpp
  signatures/ctx.cpp
  signatures/ecdsa.cpp
  signatures/eddsa.cpp
  signatures/func.cpp
  signatures/options.cpp
  signatures/rsa.cpp
  signatures/signatures.cpp
  signatures/signstate.cpp
  signatures/verificationstate.cpp
  symmetric/aeads.cpp
  symmetric/ctx.cpp
  symmetric/func.cpp
  symmetric/hash.cpp
  symmetric/kdf.cpp
  symmetric/key.cpp
  symmetric/mac.cpp
  symmetric/options.cpp
  symmetric/state.cpp
  symmetric/tag.cpp
  utils/evp_wrapper.cpp
  utils/hostfunction.cpp
)

target_compile_options(wasmedgePluginWasiCrypto
  PUBLIC
  -DWASMEDGE_PLUGIN
)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  target_link_options(wasmedgePluginWasiCrypto
    PUBLIC
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterC1EPKNS0_6Plugin16PluginDescriptorE
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterD1Ev
  )
endif()

target_include_directories(wasmedgePluginWasiCrypto
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/thirdparty
)

target_link_libraries(wasmedgePluginWasiCrypto
  PUBLIC
  OpenSSL::Crypto
  wasmedgeCommon
  wasmedgeSystem
)

install(TARGETS wasmedgePluginWasiCrypto DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
