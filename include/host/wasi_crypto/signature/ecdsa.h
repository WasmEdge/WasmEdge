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

#include "host/wasi_crypto/evpwrapper.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

template <uint32_t Nid> class Ecdsa {
public:
  class PublicKey final : public Signatures::PublicKey {
  public:
    PublicKey(EvpPkeyPtr Ctx) : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<std::unique_ptr<PublicKey>>
    import(Span<uint8_t const> Encoded, __wasi_publickey_encoding_e_t Encoding);

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_publickey_encoding_e_t Encoding) override;

    WasiCryptoExpect<std::unique_ptr<Signatures::VerificationState>>
    openVerificationState() override;

  private:
    static WasiCryptoExpect<std::unique_ptr<PublicKey>>
    importPkcs8(Span<uint8_t const> Encoded);

    static WasiCryptoExpect<std::unique_ptr<PublicKey>>
    importPem(Span<uint8_t const> Encoded);

    // all ok, compress or not compress
    static WasiCryptoExpect<std::unique_ptr<PublicKey>>
    importRaw(Span<uint8_t const> Encoded);

    WasiCryptoExpect<std::vector<uint8_t>> exportSec();

    WasiCryptoExpect<std::vector<uint8_t>> exportCompressedSec();

    EvpPkeyPtr Ctx;
  };

  class SecretKey final : public Signatures::SecretKey {
  public:
    SecretKey(EvpPkeyPtr Ctx) : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<std::unique_ptr<SecretKey>>
    import(Span<uint8_t const> Encoded, __wasi_secretkey_encoding_e_t Encoding);

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_secretkey_encoding_e_t Encoding) override;

  private:
    static WasiCryptoExpect<std::unique_ptr<SecretKey>>
    importPkcs8(Span<uint8_t const> Encoded);

    static WasiCryptoExpect<std::unique_ptr<SecretKey>>
    importPem(Span<uint8_t const> Encoded);

    static WasiCryptoExpect<std::unique_ptr<SecretKey>>
    importRaw(Span<uint8_t const> Encoded);

    WasiCryptoExpect<std::vector<uint8_t>> exportPkcs8();

    WasiCryptoExpect<std::vector<uint8_t>> exportPem();

    WasiCryptoExpect<std::vector<uint8_t>> exportRaw();

    EvpPkeyPtr Ctx;
  };

  class KeyPair final : public Signatures::KeyPair {
  public:
    KeyPair(EvpPkeyPtr Ctx) : Ctx(std::move(Ctx)) {}

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
    static WasiCryptoExpect<std::unique_ptr<KeyPair>>
    importPkcs8(Span<uint8_t const> Encoded);

    static WasiCryptoExpect<std::unique_ptr<KeyPair>>
    importPem(Span<uint8_t const> Encoded);

    static WasiCryptoExpect<std::unique_ptr<KeyPair>>
    importRaw(Span<uint8_t const> Encoded);

    WasiCryptoExpect<std::vector<uint8_t>> exportPkcs8();

    WasiCryptoExpect<std::vector<uint8_t>> exportPem();

    WasiCryptoExpect<std::vector<uint8_t>> exportRaw();

    EvpPkeyPtr Ctx;
  };

  class Signature final : public Signatures::Signature {
  public:
    Signature(std::vector<uint8_t> &&Data)
        : Signatures::Signature(getAlg(), std::move(Data)) {}

    static WasiCryptoExpect<std::unique_ptr<Signature>>
    import(Span<uint8_t const> Encoded, __wasi_signature_encoding_e_t Encoding);

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_signature_encoding_e_t Encoding) override;
  };

  class SignState final : public Signatures::SignState {
  public:
    SignState(EvpMdCtxPtr Ctx) : Ctx(std::move(Ctx)) {}

    WasiCryptoExpect<void> update(Span<uint8_t const> Input) override;

    WasiCryptoExpect<std::unique_ptr<Signatures::Signature>> sign() override;

  private:
    EvpMdCtxPtr Ctx;
  };

  class VerificationState final : public Signatures::VerificationState {
  public:
    VerificationState(EvpMdCtxPtr Ctx) : Ctx(std::move(Ctx)) {}

    WasiCryptoExpect<void> update(Span<const uint8_t> Input) override;

    WasiCryptoExpect<void>
    verify(std::shared_ptr<Signatures::Signature> Sig) override;

  private:
    EvpMdCtxPtr Ctx;
  };

private:
  static constexpr SignatureAlgorithm getAlg() {
    if constexpr (Nid == NID_X9_62_prime256v1)
      return SignatureAlgorithm::ECDSA_P256_SHA256;
    else if constexpr (Nid == NID_secp256k1)
      return SignatureAlgorithm::ECDSA_K256_SHA256;
    else
      assumingUnreachable();
  }

  static EvpPkeyPtr initEC();
};

using EcdsaP256 = Ecdsa<NID_X9_62_prime256v1>;
using EcdsaK256 = Ecdsa<NID_secp256k1>;

} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
