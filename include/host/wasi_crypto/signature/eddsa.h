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

class EddsaSignaturePublicKey: public SignaturePublicKey::Base{
public:
  static WasiCryptoExpect<std::unique_ptr<EddsaSignaturePublicKey>>
  import(SignatureAlgorithm Alg, Span<uint8_t const > Encoded,
         __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_publickey_encoding_e_t Encoding);

  SignatureAlgorithm Alg;
};

class EddsaSignatureSecretKey {
public:
};

class EddsaSignatureKeyPair {
public:
  static WasiCryptoExpect<EddsaSignatureKeyPair>
  fromRaw(SignatureAlgorithm Alg, Span<uint8_t const> Raw);

  static WasiCryptoExpect<EddsaSignatureKeyPair> fromPcks8();

  static WasiCryptoExpect<EddsaSignatureKeyPair>
  fromPem(SignatureAlgorithm Alg, Span<uint8_t const> Pem);

  WasiCryptoExpect<std::vector<uint8_t>> asRaw();

  static WasiCryptoExpect<EddsaSignatureKeyPair>
  generate(SignatureAlgorithm Alg, std::optional<SignatureOptions> Options);

  static WasiCryptoExpect<EddsaSignatureKeyPair>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<EddsaSignaturePublicKey> publicKey();
};

class EddsaSignature : public Signature::Base {
public:
  std::vector<uint8_t> asRaw() override;

  static WasiCryptoExpect<std::unique_ptr<EddsaSignature>>
  fromRaw(Span<uint8_t const> Raw);
};

class EddsaSignatureState : public SignatureState::Base {
public:
  EddsaSignatureState(EddsaSignatureKeyPair Kp);
  WasiCryptoExpect<void> update(Span<uint8_t> Input) override;
  WasiCryptoExpect<void> sign() override;
};

class EddsaSignatureVerificationState
    : public SignatureVerificationState::Base {
public:
  EddsaSignatureVerificationState(EddsaSignaturePublicKey Pk);
  WasiCryptoExpect<void> update(Span<const uint8_t> Input) override;
  WasiCryptoExpect<void> verify(Signature &Sig) override;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
