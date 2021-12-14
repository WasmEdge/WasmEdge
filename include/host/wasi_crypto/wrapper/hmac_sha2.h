// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/symmetric/alg.h"
#include "host/wasi_crypto/wrapper/openssl.h"
#include "openssl/evp.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class HmacSha2Ctx {
public:
  HmacSha2Ctx(OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx)
      : Ctx(std::move(Ctx)) {}

  static WasiCryptoExpect<HmacSha2Ctx> import(SymmetricAlgorithm Alg,
                                           Span<uint8_t const> Raw);

  WasiCryptoExpect<void> absorb(Span<const uint8_t> Data);

  WasiCryptoExpect<std::vector<uint8_t>> squeezeTag();

private:
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
