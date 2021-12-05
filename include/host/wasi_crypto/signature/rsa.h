// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/signature/alg.h"
#include "host/wasi_crypto/signature/options.h"
#include "host/wasi_crypto/signature/signature.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class RsaSignaturePublicKey {
public:
  static WasiCryptoExpect<RsaSignaturePublicKey>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_publickey_encoding_e_t Encoding);

  SignatureAlgorithm Alg;
};

class RsaSignatureSecretKey {

};

class RsaSignatureKeyPair {
public:
  static WasiCryptoExpect<RsaSignatureKeyPair> fromRaw(SignatureAlgorithm Alg,
                                                       Span<uint8_t const> Raw);

  static WasiCryptoExpect<RsaSignatureKeyPair> fromPcks8();

  static WasiCryptoExpect<RsaSignatureKeyPair> fromPem(SignatureAlgorithm Alg,
                                                       Span<uint8_t const> Pem);

  WasiCryptoExpect<std::vector<uint8_t>> asRaw();

  static WasiCryptoExpect<RsaSignatureKeyPair>
  generate(SignatureAlgorithm Alg, std::optional<SignatureOptions> Options);

  static WasiCryptoExpect<RsaSignatureKeyPair>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<RsaSignaturePublicKey> publicKey();
};

class RsaSignature : public Signature::Base {};

class RsaSignatureState : public SignatureState::Base {
public:
  RsaSignatureState(RsaSignatureKeyPair Kp);
  WasiCryptoExpect<void> update(Span<uint8_t> Input) override;
  WasiCryptoExpect<void> sign() override;
};

class RsaSignatureVerificationState : public SignatureVerificationState::Base {
public:
  RsaSignatureVerificationState(RsaSignaturePublicKey Pk);
  WasiCryptoExpect<void> update(Span<const uint8_t> Input) override;
  WasiCryptoExpect<void> verify(Signature &Sig) override;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
