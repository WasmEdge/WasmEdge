// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/aes_gcm.h"
#include "host/wasi_crypto/symmetric/charcha_poly.h"
#include "host/wasi_crypto/symmetric/hkdf.h"
#include "host/wasi_crypto/symmetric/hmac_sha2.h"
#include "host/wasi_crypto/symmetric/xoodyak.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
SymmetricKey::generate(std::string_view Alg,
                           std::shared_ptr<SymmetricOption> Option) {
  auto Builder = builder(Alg);
  if (!Builder) {
    return WasiCryptoUnexpect(Builder);
  }
  return (*Builder)->generate(Option);
}

WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
SymmetricKey::import(std::string_view Alg, Span<uint8_t const> Raw) {
  auto Builder = builder(Alg);
  if (!Builder) {
    return WasiCryptoUnexpect(Builder);
  }
  return (*Builder)->import(Raw);
}

WasiCryptoExpect<std::unique_ptr<SymmetricKeyBuilder>>
SymmetricKey::builder(std::string_view Alg) {
  auto EnumAlg = fromConstantString(Alg);
  if (!EnumAlg) {
    return WasiCryptoUnexpect(EnumAlg);
  }
  switch (*EnumAlg) {
  case SymmetricAlgorithm::HmacSha256:
  case SymmetricAlgorithm::HmacSha512:
    return std::make_unique<HmacSha2KeyBuilder>(*EnumAlg);
  case SymmetricAlgorithm::HkdfSha256Expand:
  case SymmetricAlgorithm::HkdfSha256Extract:
  case SymmetricAlgorithm::HkdfSha512Expand:
  case SymmetricAlgorithm::HkdfSha512Extract:
    return std::make_unique<HkdfSymmetricKeyBuilder>(*EnumAlg);
  case SymmetricAlgorithm::Aes128Gcm:
  case SymmetricAlgorithm::Aes256Gcm:
    return std::make_unique<AesGcmSymmetricKeyBuilder>(*EnumAlg);
  case SymmetricAlgorithm::ChaCha20Poly1305:
  case SymmetricAlgorithm::XChaCha20Poly1305:
    return std::make_unique<ChaChaPolySymmetricKeyBuilder>(*EnumAlg);
  case SymmetricAlgorithm::Xoodyak128:
  case SymmetricAlgorithm::Xoodyak160:
    return std::make_unique<XoodyakSymmetricKeyBuilder>(*EnumAlg);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
