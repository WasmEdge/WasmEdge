// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/asymmetric_common/publickey.h"
#include "host/wasi_crypto/signature/alg.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<PublicKey>
PublicKey::import(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
                  Span<uint8_t const> Encoded,
                  __wasi_publickey_encoding_e_t Encoding) {
  switch (AlgType) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES: {
    auto Alg = tryFrom<SignatureAlgorithm>(AlgStr);
    if (!Alg) {
      return WasiCryptoUnexpect(Alg);
    }

    auto Res = SignaturePublicKey::import(*Alg, Encoded, Encoding);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }

    return PublicKey{*Res};
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
}

WasiCryptoExpect<std::vector<uint8_t>>
PublicKey::exportData(__wasi_publickey_encoding_e_t Encoding) {
  return std::visit(Overloaded{[Encoding](auto &Pk) {
                      return Pk.inner()->locked([Encoding](auto &Inner) {
                        return Inner->exportData(Encoding);
                      });
                    }},
                    Inner);
}

WasiCryptoExpect<void> PublicKey::verify() {
  return std::visit(Overloaded{[](auto &Pk) {
                      return Pk.inner()->locked(
                          [](auto &Inner) { return Inner->verify(); });
                    }},
                    Inner);
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
