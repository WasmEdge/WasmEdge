// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/aeads/aes_gcm.h"
#include "host/wasi_crypto/symmetric/aeads/charcha_poly.h"
#include "host/wasi_crypto/symmetric/extract_and_expand/hkdf.h"
#include "host/wasi_crypto/symmetric/mac/hmac_sha2.h"
#include "host/wasi_crypto/symmetric/session/xoodyak.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

WasiCryptoExpect<std::unique_ptr<Key>>
Key::generate(SymmetricAlgorithm Alg, std::shared_ptr<Option> OptOption) {
  auto Builder = builder(Alg);
  if (!Builder) {
    return WasiCryptoUnexpect(Builder);
  }
  return (*Builder)->generate(OptOption);
}

WasiCryptoExpect<std::unique_ptr<Key>> Key::import(SymmetricAlgorithm Alg,
                                                   Span<uint8_t const> Raw) {
  auto Builder = builder(Alg);
  if (!Builder) {
    return WasiCryptoUnexpect(Builder);
  }
  return (*Builder)->import(Raw);
}

WasiCryptoExpect<std::unique_ptr<Key::Builder>>
Key::builder(SymmetricAlgorithm Alg) {
  switch (Alg) {
  case SymmetricAlgorithm::HmacSha256:
    return std::make_unique<HmacSha256::KeyBuilder>(Alg);
  case SymmetricAlgorithm::HmacSha512:
    return std::make_unique<HmacSha512::KeyBuilder>(Alg);
  case SymmetricAlgorithm::HkdfSha256Expand:
    return std::make_unique<Hkdf256Expand::KeyBuilder>(Alg);
  case SymmetricAlgorithm::HkdfSha256Extract:
    return std::make_unique<Hkdf256Extract::KeyBuilder>(Alg);
  case SymmetricAlgorithm::HkdfSha512Expand:
    return std::make_unique<Hkdf512Expand::KeyBuilder>(Alg);
  case SymmetricAlgorithm::HkdfSha512Extract:
    return std::make_unique<Hkdf512Extract::KeyBuilder>(Alg);
  case SymmetricAlgorithm::Aes128Gcm:
    return std::make_unique<AesGcm128::KeyBuilder>(Alg);
  case SymmetricAlgorithm::Aes256Gcm:
    return std::make_unique<AesGcm256::KeyBuilder>(Alg);
  case SymmetricAlgorithm::ChaCha20Poly1305:
    return std::make_unique<ChaChaPoly1305::KeyBuilder>(Alg);
  case SymmetricAlgorithm::XChaCha20Poly1305:
  case SymmetricAlgorithm::Xoodyak128:
  case SymmetricAlgorithm::Xoodyak160:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
}

///// require: T have a key builder class
// template <typename T> std::unique_ptr<T> makeKeyBuilder(typename T::Alg Alg)
// {
//   static const std::map<SymmetricAlgorithm, typename T::builder> Inner{
//       {SymmetricAlgorithm::HmacSha256, }
//   };
// }

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
