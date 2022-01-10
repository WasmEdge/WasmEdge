// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/signature/alg.h"
#include "host/wasi_crypto/signature/keypair.h"
#include "host/wasi_crypto/signature/options.h"
#include "host/wasi_crypto/signature/publickey.h"
#include "host/wasi_crypto/signature/secretkey.h"
#include "host/wasi_crypto/signature/signature.h"
#include <openssl/evp.h>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

using NID = int;
template <NID Nid> class Ecdsa {
public:
  class PublicKey : public Signatures::PublicKey {
  public:
    PublicKey(EVP_PKEY *Ctx) : Ctx(Ctx) {}

    static WasiCryptoExpect<std::unique_ptr<PublicKey>>
    import(Span<uint8_t const> Encoded, __wasi_publickey_encoding_e_t Encoding);

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_publickey_encoding_e_t Encoding) override;

    WasiCryptoExpect<std::unique_ptr<Signatures::VerificationState>>
    openVerificationState() override;

  private:
    OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Ctx;
  };

  class SecretKey : public Signatures::SecretKey {
  public:
    SecretKey(EVP_PKEY *Ctx) : Ctx(Ctx) {}

    static WasiCryptoExpect<std::unique_ptr<SecretKey>>
    import(Span<uint8_t const> Encoded, __wasi_secretkey_encoding_e_t Encoding);

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_secretkey_encoding_e_t Encoding) override;

  private:
    OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Ctx;
  };

  class KeyPair : public Signatures::KeyPair {
  public:
    KeyPair(EVP_PKEY *Ctx) : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<std::unique_ptr<KeyPair>>
    generate(std::shared_ptr<Options> Options);

    static WasiCryptoExpect<std::unique_ptr<KeyPair>>
    import(Span<uint8_t const> Encoded, __wasi_keypair_encoding_e_t Encoding);

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_keypair_encoding_e_t Encoding) override;

    WasiCryptoExpect<std::unique_ptr<Signatures::PublicKey>>
    publicKey() override;

    WasiCryptoExpect<std::unique_ptr<Signatures::SecretKey>>
    secretKey() override;

    WasiCryptoExpect<std::unique_ptr<Signatures::SignState>>
    openSignState() override;

  private:
    OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Ctx;
  };

  class Signature : public Signatures::Signature {
  public:
    Signature(std::vector<uint8_t> &&Sign) : Sign(std::move(Sign)) {}

    static WasiCryptoExpect<std::unique_ptr<Signature>>
    import(Span<uint8_t const> Encoded, __wasi_signature_encoding_e_t Encoding);

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_signature_encoding_e_t Encoding) override;

    Span<uint8_t const> asRef() override { return Sign; }

  private:
    std::vector<uint8_t> Sign;
  };

  class SignState : public Signatures::SignState {
  public:
    SignState(EVP_MD_CTX *Ctx) : Ctx(Ctx) {}

    WasiCryptoExpect<void> update(Span<uint8_t const> Input) override;

    WasiCryptoExpect<std::unique_ptr<Signatures::Signature>> sign() override;

  private:
    OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx;
  };

  class VerificationState : public Signatures::VerificationState {
  public:
    VerificationState(EVP_MD_CTX *Ctx) : Ctx(Ctx){};

    WasiCryptoExpect<void> update(Span<const uint8_t> Input) override;

    WasiCryptoExpect<void>
    verify(std::shared_ptr<Signatures::Signature> Sig) override;

  private:
    OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx;
  };
};

using EcdsaP256 = Ecdsa<NID_X9_62_prime256v1>;
using EcdsaK256 = Ecdsa<NID_secp256k1>;

} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
