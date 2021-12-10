// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/asymmetric_common/keypair.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<KeyPair>
WASICrypto::KeyPair::generate(__wasi_algorithm_type_e_t AlgType,
                              std::string_view AlgStr,
                              std::optional<Options> OptOptions) {

  switch (AlgType) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES: {
    auto OptSigOptions = optOptionsAs<SignatureOptions>(OptOptions);
    if (!OptSigOptions) {
      return WasiCryptoUnexpect(OptSigOptions);
    }

    auto Alg = tryFrom<SignatureAlgorithm>(AlgStr);
    if (!Alg) {
      return WasiCryptoUnexpect(Alg);
    }

    auto SigKp = SignatureKeyPair::generate(*Alg, *OptSigOptions);
    if (!SigKp) {
      return WasiCryptoUnexpect(SigKp);
    }

    return KeyPair{*SigKp};
  }
  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE: {
    auto OptKxOptions = optOptionsAs<KxOptions>(OptOptions);
    if (!OptKxOptions) {
      return WasiCryptoUnexpect(OptKxOptions);
    }

    auto Alg = tryFrom<KxAlgorithm>(AlgStr);
    if (!Alg) {
      return WasiCryptoUnexpect(Alg);
    }

    auto KxKp = KxKeyPair::generate(*Alg, *OptKxOptions);
    if (!KxKp) {
      return WasiCryptoUnexpect(KxKp);
    }

    return KeyPair{*KxKp};
  }

  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
}
WasiCryptoExpect<KeyPair>
KeyPair::import(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
                Span<const uint8_t> Encoded,
                __wasi_keypair_encoding_e_t Encoding) {
  switch (AlgType) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES: {
    auto Alg = tryFrom<SignatureAlgorithm>(AlgStr);
    if (!Alg) {
      return WasiCryptoUnexpect(Alg);
    }

    auto SigKp = SignatureKeyPair::import(*Alg, Encoded, Encoding);
    if (!SigKp) {
      return WasiCryptoUnexpect(SigKp);
    }

    return KeyPair{*SigKp};
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
}

WasiCryptoExpect<KeyPair> KeyPair::fromPkAndSk(PublicKey, SecretKey) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::vector<uint8_t>>
KeyPair::exportData(__wasi_keypair_encoding_e_t Encoding) {
  return std::visit(Overloaded{[Encoding](auto &Kp) {
                      return Kp.inner()->locked([Encoding](auto &Inner) {
                        return Inner->exportData(Encoding);
                      });
                    }},
                    Inner);
}

WasiCryptoExpect<PublicKey> KeyPair::publicKey() {
  return std::visit(Overloaded{[](auto &Kp) -> WasiCryptoExpect<PublicKey> {
                      auto Res = Kp.inner()->locked(
                          [](auto &Inner) { return Inner->publicKey(); });
                      if (!Res) {
                        return WasiCryptoUnexpect(Res);
                      }

                      return PublicKey{*Res};
                    }},
                    Inner);
}
WasiCryptoExpect<SecretKey> KeyPair::secretKey() {
  return std::visit(Overloaded{[](auto &Kp) -> WasiCryptoExpect<SecretKey> {
                      auto Res = Kp.inner()->locked(
                          [](auto &Inner) { return Inner->secretKey(); });
                      if (!Res) {
                        return WasiCryptoUnexpect(Res);
                      }

                      return SecretKey{*Res};
                    }},
                    Inner);
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
