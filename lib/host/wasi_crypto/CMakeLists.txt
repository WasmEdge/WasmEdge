# SPDX-License-Identifier: Apache-2.0

find_package(OpenSSL REQUIRED)

wasmedge_add_library(wasmedgeHostModuleWasiCrypto
  cryptomodule.cpp
  common/ctx.cpp
  common/array_output.cpp
  common/options.cpp
  common/func.cpp
  symmetric/aes_gcm.cpp
  symmetric/charcha_poly.cpp
  symmetric/hkdf.cpp
  symmetric/options.cpp
  symmetric/state.cpp
  symmetric/sha2.cpp
  symmetric/hmac_sha2.cpp
  symmetric/xoodyak.cpp
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
  wrapper/sha2.cpp
  wrapper/aes_gcm.cpp
  wrapper/hkdf.cpp
  wrapper/random.cpp
  wrapper/hmac_sha2.cpp
  wrapper/x25519.cpp
  wrapper/ecdsa.cpp
  wrapper/eddsa.cpp
  wrapper/rsa.cpp
  wrapper/chacha_poly.cpp
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


