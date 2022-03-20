// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC
#include "host/wasi_crypto/signatures/factory.h"
#include "host/wasi_crypto/signatures/ecdsa.h"
#include "host/wasi_crypto/utils/error.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

FactoryVariant makeFactory(Algorithm Alg) noexcept {
  switch (Alg) {
  case Algorithm::ECDSA_P256_SHA256:
    return EcdsaP256{};
  case Algorithm::ECDSA_K256_SHA256:
    return EcdsaK256{};
  case Algorithm::Ed25519:
    return Eddsa{};
  case Algorithm::RSA_PKCS1_2048_SHA256:
    return RSA_PKCS1_2048_SHA256{};
  case Algorithm::RSA_PKCS1_2048_SHA384:
    return RSA_PKCS1_2048_SHA384{};
  case Algorithm::RSA_PKCS1_2048_SHA512:
    return RSA_PKCS1_2048_SHA512{};
  case Algorithm::RSA_PKCS1_3072_SHA384:
    return RSA_PKCS1_3072_SHA384{};
  case Algorithm::RSA_PKCS1_3072_SHA512:
    return RSA_PKCS1_3072_SHA512{};
  case Algorithm::RSA_PKCS1_4096_SHA512:
    return RSA_PKCS1_4096_SHA512{};
  case Algorithm::RSA_PSS_2048_SHA256:
    return RSA_PSS_2048_SHA256{};
  case Algorithm::RSA_PSS_2048_SHA384:
    return RSA_PSS_2048_SHA384{};
  case Algorithm::RSA_PSS_2048_SHA512:
    return RSA_PSS_2048_SHA512{};
  case Algorithm::RSA_PSS_3072_SHA384:
    return RSA_PSS_3072_SHA384{};
  case Algorithm::RSA_PSS_3072_SHA512:
    return RSA_PSS_3072_SHA512{};
  case Algorithm::RSA_PSS_4096_SHA512:
    return RSA_PSS_4096_SHA512{};

  default:
    assumingUnreachable();
  }
}

WasiCryptoExpect<SigVariant>
sigImport(Algorithm Alg, Span<const uint8_t> Encoded,
          __wasi_signature_encoding_e_t Encoding) noexcept {
  return std::visit(
      [=](auto Factory) noexcept {
        using FactoryType = std::decay_t<decltype(Factory)>;
        return FactoryType::Signature::import(Encoded, Encoding)
            .map([](auto &&Sig) noexcept { return SigVariant{Sig}; });
      },
      makeFactory(Alg));
}

} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge