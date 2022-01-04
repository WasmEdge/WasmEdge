// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/aes_gcm.h"
#include "host/wasi_crypto/symmetric/charcha_poly.h"
#include "host/wasi_crypto/symmetric/hkdf.h"
#include "host/wasi_crypto/symmetric/hmac_sha2.h"
#include "host/wasi_crypto/symmetric/xoodyak.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<SymmetricKey>
SymmetricKey::generate(SymmetricAlgorithm Alg,
                       std::optional<SymmetricOptions> OptOption) {
  auto Builder = builder(Alg);
  if (!Builder) {
    return WasiCryptoUnexpect(Builder);
  }
  return (*Builder)->generate(OptOption);
}

WasiCryptoExpect<SymmetricKey> SymmetricKey::import(SymmetricAlgorithm Alg,
                                                    Span<uint8_t const> Raw) {
  auto Builder = builder(Alg);
  if (!Builder) {
    return WasiCryptoUnexpect(Builder);
  }
  return (*Builder)->import(Raw);
}

WasiCryptoExpect<SymmetricKey> SymmetricKey::from(SymmetricAlgorithm Alg,
                                                  std::vector<uint8_t> &&Data) {
  auto Builder = builder(Alg);
  if (!Builder) {
    return WasiCryptoUnexpect(Builder);
  }

  // TODO: a vector way
  return (*Builder)->import(Data);
}

WasiCryptoExpect<std::unique_ptr<SymmetricKey::Builder>>
SymmetricKey::builder(SymmetricAlgorithm Alg) {
  switch (Alg) {
  case SymmetricAlgorithm::HmacSha256:
  case SymmetricAlgorithm::HmacSha512:
    return std::make_unique<HmacSha2KeyBuilder>(Alg);
  case SymmetricAlgorithm::HkdfSha256Expand:
  case SymmetricAlgorithm::HkdfSha256Extract:
  case SymmetricAlgorithm::HkdfSha512Expand:
  case SymmetricAlgorithm::HkdfSha512Extract:
    return std::make_unique<HkdfSymmetricKeyBuilder>(Alg);
  case SymmetricAlgorithm::Aes128Gcm:
  case SymmetricAlgorithm::Aes256Gcm:
    return std::make_unique<AesGcmSymmetricKeyBuilder>(Alg);
  case SymmetricAlgorithm::ChaCha20Poly1305:
  case SymmetricAlgorithm::XChaCha20Poly1305:
    return std::make_unique<ChaChaPolySymmetricKeyBuilder>(Alg);
  case SymmetricAlgorithm::Xoodyak128:
  case SymmetricAlgorithm::Xoodyak160:
    return std::make_unique<XoodyakSymmetricKeyBuilder>(Alg);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
