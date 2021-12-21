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

class Sha2Ctx {
public:
  Sha2Ctx(OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx)
      : Ctx(std::move(Ctx)) {}

  static WasiCryptoExpect<Sha2Ctx> import(SymmetricAlgorithm Alg);

  WasiCryptoExpect<void> absorb(Span<uint8_t const> Data);

  WasiCryptoExpect<void> squeeze(Span<uint8_t> Out);

private:
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
