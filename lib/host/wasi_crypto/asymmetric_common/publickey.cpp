// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/asymmetric_common/publickey.h"
#include "host/wasi_crypto/key_exchange/alg.h"
#include "host/wasi_crypto/key_exchange/publickey.h"
#include "host/wasi_crypto/signature/alg.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Asymmetric {
WasiCryptoExpect<PublicKey>
publicKeyImport(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
                Span<uint8_t const> Encoded,
                __wasi_publickey_encoding_e_t Encoding) {

  switch (AlgType) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES: {
    auto Alg = tryFrom<SignatureAlgorithm>(AlgStr);
    if (!Alg) {
      return WasiCryptoUnexpect(Alg);
    }

    auto Res = Signatures::PublicKey::import(*Alg, Encoded, Encoding);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }

    return std::move(*Res);
  }
  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE: {
    auto Alg = tryFrom<KxAlgorithm>(AlgStr);
    if (!Alg) {
      return WasiCryptoUnexpect(Alg);
    }

    auto Res = Kx::PublicKey::import(*Alg, Encoded, Encoding);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }

    return std::move(*Res);
  }
  case __WASI_ALGORITHM_TYPE_SYMMETRIC:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  default:
    assumingUnreachable();
  }
}

WasiCryptoExpect<std::vector<uint8_t>>
publicKeyExportData(PublicKey PublicKey,
                    __wasi_publickey_encoding_e_t Encoding) {
  return std::visit({
          [Encoding](auto &Pk) -> WasiCryptoExpect<std::vector<uint8_t>> {
      return Pk->exportData(Encoding);
          },
      PublicKey);
}

WasiCryptoExpect<void> publicKeyVerify(PublicKey PublicKey) {
    return std::visit(
        [](auto &Pk) -> WasiCryptoExpect<void> { return Pk->verify(); },
        PublicKey);
}
} // namespace Asymmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
