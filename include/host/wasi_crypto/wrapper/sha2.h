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

class Sha2 {
public:
  static WasiCryptoExpect<Sha2> make(SymmetricAlgorithm Alg);

  WasiCryptoExpect<void> absorb(Span<uint8_t const> Data);

  WasiCryptoExpect<void> squeeze(Span<uint8_t> Out);

private:
  Sha2(OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx);

  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
