// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/symmetric/alg.h"
#include "host/wasi_crypto/wrapper/openssl.h"
#include "openssl/evp.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class ChaChaPolyCtx {
public:
  inline static constexpr __wasi_size_t TagLen = 16;

  ChaChaPolyCtx(SymmetricAlgorithm /*Alg*/,
            OpenSSLUniquePtr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free> Ctx)
      : /*Alg(Alg),*/ Ctx(std::move(Ctx)) {}


  static WasiCryptoExpect<ChaChaPolyCtx> import(SymmetricAlgorithm Alg,
                                            Span<uint8_t const> Key,
                                            Span<uint8_t const> Nonce);

  WasiCryptoExpect<void> absorb(Span<const uint8_t> Data);

  WasiCryptoExpect<std::vector<uint8_t>>
  encryptDetached(Span<uint8_t> Out, Span<const uint8_t> Data);

  WasiCryptoExpect<__wasi_size_t> decryptDetached(Span<uint8_t> Out,
                                                  Span<uint8_t const> Data,
                                                  Span<uint8_t const> RawTag);

private:
  enum Mode { Unchanged = -1, Decrypt = 0, Encrypt = 1 };

//  SymmetricAlgorithm Alg;
  OpenSSLUniquePtr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free> Ctx;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
