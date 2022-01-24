// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/signature/keypair.h"
#include "host/wasi_crypto/signature/options.h"
#include "host/wasi_crypto/signature/publickey.h"
#include "host/wasi_crypto/signature/secretkey.h"
#include "host/wasi_crypto/signature/signature.h"

#include "host/wasi_crypto/evpwrapper.h"

#include <shared_mutex>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

class EddsaPublicKey final : public PublicKey {
public:
  inline static size_t PkSize = 32;

  EddsaPublicKey(EvpPkeyPtr Ctx) : Ctx(std::move(Ctx)) {}

  static WasiCryptoExpect<std::unique_ptr<EddsaPublicKey>>
  import(Span<uint8_t const> Encoded, __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_publickey_encoding_e_t Encoding) override;

  WasiCryptoExpect<std::unique_ptr<VerificationState>>
  openVerificationState() override;

private:
  EvpPkeyPtr Ctx;
};

class EddsaSecretKey final : public SecretKey {
public:
  inline static size_t SkSize = 32;

  EddsaSecretKey(EvpPkeyPtr Ctx) : Ctx(std::move(Ctx)) {}

  static WasiCryptoExpect<std::unique_ptr<SecretKey>>
  import(Span<uint8_t const> Encoded, __wasi_secretkey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_secretkey_encoding_e_t Encoding) override;

private:
  EvpPkeyPtr Ctx;
};

class EddsaKeyPair final : public KeyPair {
public:
  inline static size_t KpSize = 64;

  EddsaKeyPair(EvpPkeyPtr Ctx) : Ctx(std::move(Ctx)) {}

  static WasiCryptoExpect<std::unique_ptr<KeyPair>>
  generate(std::shared_ptr<Signatures::Options> Options);

  static WasiCryptoExpect<std::unique_ptr<KeyPair>>
  import(Span<uint8_t const> Encoded, __wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_keypair_encoding_e_t Encoding) override;

  WasiCryptoExpect<std::unique_ptr<PublicKey>> publicKey() override;

  WasiCryptoExpect<std::unique_ptr<SecretKey>> secretKey() override;

  WasiCryptoExpect<std::unique_ptr<SignState>> openSignState() override;

private:
  EvpPkeyPtr Ctx;
};

class EddsaSignature final : public Signature {
public:
  inline static size_t SigSize = 64;

  EddsaSignature(std::vector<uint8_t> &&Sign)
      : Signature(SignatureAlgorithm::Ed25519, std::move(Sign)) {}

  static WasiCryptoExpect<std::unique_ptr<Signature>>
  import(Span<uint8_t const> Encoded, __wasi_signature_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_signature_encoding_e_t Encoding) override;
};

class EddsaSignState final : public SignState {
public:
  EddsaSignState(EvpMdCtxPtr Ctx) : Ctx(std::move(Ctx)) {}

  WasiCryptoExpect<void> update(Span<uint8_t const> Input) override;

  WasiCryptoExpect<std::unique_ptr<Signature>> sign() override;

private:
  std::shared_mutex Mutex;
  std::vector<uint8_t> Cache;
  EvpMdCtxPtr Ctx;
};

class EddsaVerificationState final : public VerificationState {
public:
  EddsaVerificationState(EvpMdCtxPtr Ctx) : Ctx(std::move(Ctx)) {}

  WasiCryptoExpect<void> update(Span<const uint8_t> Input) override;

  WasiCryptoExpect<void> verify(std::shared_ptr<Signature> Sig) override;

private:
  std::shared_mutex Mutex;
  std::vector<uint8_t> Cache;
  EvpMdCtxPtr Ctx;
};

} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
