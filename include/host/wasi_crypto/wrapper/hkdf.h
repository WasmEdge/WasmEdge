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
  static WasiCryptoExpect<HkdfCtx> make(SymmetricAlgorithm Alg);

  WasiCryptoExpect<void> setKey(Span<uint8_t> Key);

  WasiCryptoExpect<void> absorb(Span<const uint8_t> Data);

  WasiCryptoExpect<Span<uint8_t const>> squeezeKey();

  WasiCryptoExpect<void> squeeze(Span<uint8_t> Out);

private:
  HkdfCtx(SymmetricAlgorithm Alg,
       OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx);

  SymmetricAlgorithm Alg;
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
