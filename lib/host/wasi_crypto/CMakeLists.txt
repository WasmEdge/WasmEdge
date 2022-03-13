# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

find_package(OpenSSL REQUIRED)

wasmedge_add_library(wasmedgeHostModuleWasiCrypto
  module.cpp
  common/func.cpp
  symmetric/func.cpp
  asymmetric_common/func.cpp
  kx/func.cpp
  signatures/func.cpp
  common/ctx.cpp
  common/array_output.cpp
  symmetric/options.cpp
  kx/options.cpp
  signatures/options.cpp
  common/options.cpp
  symmetric/ctx.cpp
  symmetric/tag.cpp
  symmetric/key.cpp
  symmetric/state.cpp
  symmetric/factory.cpp
  utils/evp_wrapper.cpp
  symmetric/hash/sha2.cpp
  symmetric/mac/hmac.cpp
  symmetric/eae/hkdf.cpp
  symmetric/aeads/cipher.cpp
  asymmetric_common/ctx.cpp
  asymmetric_common/publickey.cpp
  asymmetric_common/secretkey.cpp
  asymmetric_common/keypair.cpp
  asymmetric_common/factory.cpp
  kx/ctx.cpp
  kx/factory.cpp
  kx/kx.cpp
  kx/dh/x25519.cpp
  signatures/ctx.cpp
)

target_include_directories(wasmedgeHostModuleWasiCrypto
  PUBLIC
  ${PROJECT_SOURCE_DIR}/thirdparty
)

target_link_libraries(wasmedgeHostModuleWasiCrypto
  PUBLIC
  wasmedgeSystem
  OpenSSL::Crypto
)
