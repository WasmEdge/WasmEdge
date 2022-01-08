// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/signature/alg.h"
#include "host/wasi_crypto/signature/options.h"
#include "host/wasi_crypto/signature/publickey.h"
#include "host/wasi_crypto/signature/secretkey.h"
#include "host/wasi_crypto/signature/signature.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

class KeyPair {
public:
  virtual ~KeyPair() = default;

  virtual WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_keypair_encoding_e_t Encoding) = 0;

  virtual WasiCryptoExpect<std::unique_ptr<PublicKey>> publicKey() = 0;

  virtual WasiCryptoExpect<std::unique_ptr<SecretKey>> secretKey() = 0;

  virtual WasiCryptoExpect<std::unique_ptr<State>>
  asState(std::shared_ptr<Options> OptOptions) = 0;

  static WasiCryptoExpect<std::unique_ptr<KeyPair>> generate(SignatureAlgorithm Alg,
                                            std::optional<Options> Options);

  static WasiCryptoExpect<std::unique_ptr<KeyPair>>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_keypair_encoding_e_t Encoding);
};

} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
