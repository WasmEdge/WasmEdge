# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

find_package(OpenSSL REQUIRED)

wasmedge_add_library(wasmedgeHostModuleWasiCrypto
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
  module.cpp
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

target_include_directories(wasmedgeHostModuleWasiCrypto
  PUBLIC
  ${PROJECT_SOURCE_DIR}/thirdparty
)

target_link_libraries(wasmedgeHostModuleWasiCrypto
  PUBLIC
  OpenSSL::Crypto
  wasmedgeSystem
)
