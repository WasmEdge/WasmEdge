// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/asymmetric_common/keypair.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Asymmetric {

WasiCryptoExpect<KeyPair> keypairGenerate(__wasi_algorithm_type_e_t AlgType,
                                          std::string_view AlgStr,
                                          Common::Options OptOptions) {
  switch (AlgType) {

  case __WASI_ALGORITHM_TYPE_SIGNATURES:
    return std::visit(
        Overloaded{[AlgStr](std::shared_ptr<Signatures::Options> OptSigOptions)
                       -> WasiCryptoExpect<Asymmetric::KeyPair> {
                     auto Alg = tryFrom<SignatureAlgorithm>(AlgStr);
                     if (!Alg) {
                       return WasiCryptoUnexpect(Alg);
                     }

                     return Signatures::KeyPair::generate(*Alg, OptSigOptions);
                   },
                   [](auto &&) -> WasiCryptoExpect<Asymmetric::KeyPair> {
                     return WasiCryptoUnexpect(
                         __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
                   }},
        OptOptions);

  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE:
    return std::visit(
        Overloaded{[AlgStr](std::shared_ptr<Kx::Options> OptKxOptions)
                       -> WasiCryptoExpect<Asymmetric::KeyPair> {
                     auto Alg = tryFrom<KxAlgorithm>(AlgStr);
                     if (!Alg) {
                       return WasiCryptoUnexpect(Alg);
                     }

                     return Kx::KeyPair::generate(*Alg, OptKxOptions);
                   },
                   [](auto &&) -> WasiCryptoExpect<Asymmetric::KeyPair> {
                     return WasiCryptoUnexpect(
                         __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
                   }},
        OptOptions);

  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
}

WasiCryptoExpect<KeyPair> keyPairImport(__wasi_algorithm_type_e_t AlgType,
                                        std::string_view AlgStr,
                                        Span<const uint8_t> Encoded,
                                        __wasi_keypair_encoding_e_t Encoding) {
  switch (AlgType) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES: {
    auto Alg = tryFrom<SignatureAlgorithm>(AlgStr);
    if (!Alg) {
      return WasiCryptoUnexpect(Alg);
    }

    auto SigKp = Signatures::KeyPair::import(*Alg, Encoded, Encoding);
    if (!SigKp) {
      return WasiCryptoUnexpect(SigKp);
    }

    return std::move(*SigKp);
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
}

WasiCryptoExpect<KeyPair> keyPairFromPkAndSk(PublicKey, SecretKey) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::vector<uint8_t>>
keyPairExportData(KeyPair KeyPair, __wasi_keypair_encoding_e_t Encoding) {
  return std::visit(
      Overloaded{
          [Encoding](auto &&Kp) -> WasiCryptoExpect<std::vector<uint8_t>> {
            return Kp->exportData(Encoding);
          }},
      KeyPair);
}

WasiCryptoExpect<PublicKey> keyPairPublicKey(KeyPair KeyPair) {
  return std::visit(Overloaded{[](auto &&Kp) -> WasiCryptoExpect<PublicKey> {
                      return Kp->publicKey();
                    }},
                    KeyPair);
}

WasiCryptoExpect<SecretKey> keyPairSecretKey(KeyPair KeyPair) {
  return std::visit(Overloaded{[](auto &&Kp) -> WasiCryptoExpect<SecretKey> {
                      return Kp->secretKey();
                    }},
                    KeyPair);
}

} // namespace Asymmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
