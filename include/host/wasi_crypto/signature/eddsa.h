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

class EddsaPublicKey : public PublicKey {
public:
  EddsaPublicKey(EVP_PKEY *Ctx) : Ctx(std::move(Ctx)) {}

  static WasiCryptoExpect<std::unique_ptr<EddsaPublicKey>>
  import(Span<uint8_t const> Encoded, __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_publickey_encoding_e_t Encoding) override;

  WasiCryptoExpect<std::unique_ptr<VerificationState>>
  openVerificationState() override;

private:
  EvpPkeyPtr Ctx;
};

class EddsaSecretKey : public SecretKey {
public:
  EddsaSecretKey(EVP_PKEY *Ctx) : Ctx(Ctx) {}

  static WasiCryptoExpect<std::unique_ptr<SecretKey>>
  import(Span<uint8_t const> Encoded, __wasi_secretkey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_secretkey_encoding_e_t Encoding) override;

private:
  EvpPkeyPtr Ctx;
};

class EddsaKeyPair : public KeyPair {
public:
  EddsaKeyPair(EVP_PKEY *Ctx) : Ctx(Ctx) {}

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

class EddsaSignature : public Signature {
public:
  EddsaSignature(std::vector<uint8_t> &&Sign)
      : Signature(SignatureAlgorithm::Ed25519, std::move(Sign)) {}

  static WasiCryptoExpect<std::unique_ptr<Signature>>
  import(Span<uint8_t const> Encoded, __wasi_signature_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_signature_encoding_e_t Encoding) override;
};

class EddsaSignState : public SignState {
public:
  EddsaSignState(EVP_MD_CTX *MdCtx) : Ctx(MdCtx) {}

  WasiCryptoExpect<void> update(Span<uint8_t const> Input) override;

  WasiCryptoExpect<std::unique_ptr<Signature>> sign() override;

private:
  std::shared_mutex Mutex;
  std::vector<uint8_t> Cache;
  EvpMdCtxPtr Ctx;
};

class EddsaVerificationState : public VerificationState {
public:
  EddsaVerificationState(EVP_MD_CTX *Ctx) : Ctx(Ctx) {}

  ~EddsaVerificationState() override;

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
