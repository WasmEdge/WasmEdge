// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/symmetric/alg.h"
#include "host/wasi_crypto/wrapper/openssl.h"

#include "openssl/evp.h"
#include "openssl/kdf.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class HkdfCtx {
public:
  HkdfCtx(SymmetricAlgorithm Alg,
          OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx)
      : Alg(Alg), Ctx(std::move(Ctx)) {}

  static WasiCryptoExpect<HkdfCtx> import(SymmetricAlgorithm Alg,
                                          Span<const uint8_t> Key);

  WasiCryptoExpect<void> absorb(Span<const uint8_t> Data);

  // Extract
  WasiCryptoExpect<std::vector<uint8_t>> squeezeKey();

  // Expand
  WasiCryptoExpect<void> squeeze(Span<uint8_t> Out);

private:
  SymmetricAlgorithm Alg;
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
