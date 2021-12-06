// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/signature/alg.h"
#include "host/wasi_crypto/signature/options.h"
#include "host/wasi_crypto/signature/publickey.h"
#include "host/wasi_crypto/signature/secretkey.h"
#include "host/wasi_crypto/signature/signature.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class EcdsaSignaturePublicKey : public SignaturePublicKey::Base {
public:
  static WasiCryptoExpect<std::unique_ptr<EcdsaSignaturePublicKey>>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_publickey_encoding_e_t Encoding);

  SignatureAlgorithm Alg;
};

class EcdsaSignatureSecretKey : public SignatureSecretKey::Base {
public:
};

class EcdsaSignatureKeyPair : public SignatureKeyPair::Base {
public:
  static WasiCryptoExpect<EcdsaSignatureKeyPair>
  fromRaw(SignatureAlgorithm Alg, Span<uint8_t const> Raw);

  static WasiCryptoExpect<EcdsaSignatureKeyPair> fromPcks8();

  static WasiCryptoExpect<EcdsaSignatureKeyPair>
  fromPem(SignatureAlgorithm Alg, Span<uint8_t const> Pem);

  WasiCryptoExpect<std::vector<uint8_t>> asRaw();

  static WasiCryptoExpect<EcdsaSignatureKeyPair>
  generate(SignatureAlgorithm Alg, std::optional<SignatureOptions> Options);

  static WasiCryptoExpect<EcdsaSignatureKeyPair>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<SignaturePublicKey> publicKey();
};

class EcdsaSignature : public Signature::Base {
public:

  static WasiCryptoExpect<std::unique_ptr<EcdsaSignature>>
  fromRaw(Span<uint8_t const> Raw) {
    return WasmEdge::Host::WASICrypto::WasiCryptoExpect<
        std::unique_ptr<EcdsaSignature>>();
  }
};

class EcdsaSignatureState : public SignatureState::Base {
public:
  EcdsaSignatureState(EcdsaSignatureKeyPair Kp);

  WasiCryptoExpect<void> update(Span<uint8_t const> Input) override;
  WasiCryptoExpect<Signature> sign() override;
};

class EcdsaSignatureVerificationState
    : public SignatureVerificationState::Base {
public:
  EcdsaSignatureVerificationState(EcdsaSignaturePublicKey Pk);

  WasiCryptoExpect<void> update(Span<const uint8_t> Input) override;
  WasiCryptoExpect<void> verify(Signature &Sig) override;
};
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
