# SPDX-License-Identifier: Apache-2.0

find_package(OpenSSL REQUIRED)

wasmedge_add_library(wasmedgeHostModuleWasiCrypto
  cryptomodule.cpp
  common/ctx.cpp
  common/array_output.cpp
  common/options.cpp
  common/func.cpp
  symmetric/aeads/aeads_state.cpp
  symmetric/aeads/aes_gcm.cpp
  symmetric/aeads/charcha_poly.cpp
  symmetric/extract_and_expand/hkdf.cpp
  symmetric/session/xoodyak.cpp
  symmetric/hash/sha2.cpp
  symmetric/mac/hmac_sha2.cpp
  symmetric/options.cpp
  symmetric/state.cpp
  symmetric/ctx.cpp
  symmetric/key.cpp
  symmetric/func.cpp
  symmetric/tag.cpp
  asymmetric_common/ctx.cpp
  asymmetric_common/func.cpp
  asymmetric_common/publickey.cpp
  asymmetric_common/secretkey.cpp
  asymmetric_common/keypair.cpp
  signature/options.cpp
  signature/func.cpp
  signature/ctx.cpp
  signature/publickey.cpp
  signature/secretkey.cpp
  signature/keypair.cpp
  signature/signature.cpp
  signature/rsa.cpp
  signature/ecdsa.cpp
  signature/eddsa.cpp
  key_exchange/options.cpp
  key_exchange/ctx.cpp
  key_exchange/func.cpp
  key_exchange/secretkey.cpp
  key_exchange/keypair.cpp
  key_exchange/publickey.cpp
  key_exchange/dh/x25519.cpp
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


