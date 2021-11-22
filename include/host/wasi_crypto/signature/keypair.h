// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class SignatureKeyPair {
public:
  static WasiCryptoExpect<SignatureKeyPair>
  generate(SignatureAlgorithm Alg, std::optional<SignatureOptions> Options);

  static WasiCryptoExpect<SignatureKeyPair>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<SignaturePublicKey> publicKey();

  WasiCryptoExpect<SignatureSecretKey> secretKey();
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
