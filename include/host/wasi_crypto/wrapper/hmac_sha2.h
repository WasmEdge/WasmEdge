// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/symmetric/alg.h"
#include "host/wasi_crypto/wrapper/openssl.h"
#include "openssl/evp.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class HmacSha2 {
public:
  static WasiCryptoExpect<HmacSha2> make(SymmetricAlgorithm Alg);

  WasiCryptoExpect<void> setPKey(Span<uint8_t> Raw);

  WasiCryptoExpect<void> absorb(Span<const uint8_t> Data);

  WasiCryptoExpect<Span<uint8_t>> squeezeTag();

private:
  HmacSha2(OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx);

  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
