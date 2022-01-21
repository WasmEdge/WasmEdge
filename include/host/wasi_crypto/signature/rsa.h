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
#include "openssl/rsa.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

template <uint32_t Pad, uint32_t Size, uint32_t Sha> class Rsa {
  static constexpr SignatureAlgorithm getAlg() {
    if constexpr (Pad == RSA_PKCS1_PADDING) {
      if constexpr (Size == 2048) {
        if constexpr (Sha == 256) {
          return SignatureAlgorithm::RSA_PKCS1_2048_SHA256;
        }
        if constexpr (Sha == 384) {
          return SignatureAlgorithm::RSA_PKCS1_2048_SHA384;
        }
        if constexpr (Sha == 512) {
          return SignatureAlgorithm::RSA_PKCS1_2048_SHA512;
        }
      }

      if constexpr (Size == 3072) {
        if constexpr (Sha == 384) {
          return SignatureAlgorithm::RSA_PKCS1_3072_SHA384;
        }
        if constexpr (Sha == 512) {
          return SignatureAlgorithm::RSA_PKCS1_3072_SHA512;
        }
      }

      if constexpr (Size == 4096) {
        if constexpr (Sha == 512) {
          return SignatureAlgorithm::RSA_PKCS1_4096_SHA512;
        }
      }
    }

    if constexpr (Pad == RSA_PKCS1_PSS_PADDING) {
      if constexpr (Size == 2048) {
        if constexpr (Sha == 256) {
          return SignatureAlgorithm::RSA_PSS_2048_SHA256;
        }
        if constexpr (Sha == 384) {
          return SignatureAlgorithm::RSA_PSS_2048_SHA384;
        }
        if constexpr (Sha == 512) {
          return SignatureAlgorithm::RSA_PSS_2048_SHA512;
        }
      }

      if constexpr (Size == 3072) {
        if constexpr (Sha == 384) {
          return SignatureAlgorithm::RSA_PSS_3072_SHA384;
        }
        if constexpr (Sha == 512) {
          return SignatureAlgorithm::RSA_PSS_3072_SHA512;
        }
      }

      if constexpr (Size == 4096) {
        if constexpr (Sha == 512) {
          return SignatureAlgorithm::RSA_PSS_4096_SHA512;
        }
      }
    }
  }

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
    importPem(Span<uint8_t const> Encoded);

    static WasiCryptoExpect<std::unique_ptr<PublicKey>>
    importPkcs8(Span<uint8_t const> Encoded);

    static WasiCryptoExpect<std::unique_ptr<PublicKey>>
    importLocal(Span<uint8_t const> Encoded);

    WasiCryptoExpect<std::vector<uint8_t>> exportPem();

    WasiCryptoExpect<std::vector<uint8_t>> exportPkcs8();

    WasiCryptoExpect<std::vector<uint8_t>> exportLocal();

    EvpPkeyPtr Ctx;
  };

  class SecretKey final : public Signatures::SecretKey {
  public:
    SecretKey(EvpPkeyPtr Ctx) : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<std::unique_ptr<SecretKey>>
    import(Span<const uint8_t> Encoded, __wasi_secretkey_encoding_e_t Encoding);

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_secretkey_encoding_e_t Encoding) override;

  private:
    static WasiCryptoExpect<std::unique_ptr<SecretKey>>
    importPem(Span<uint8_t const> Encoded);

    static WasiCryptoExpect<std::unique_ptr<SecretKey>>
    importPkcs8(Span<uint8_t const> Encoded);

    static WasiCryptoExpect<std::unique_ptr<SecretKey>>
    importLocal(Span<uint8_t const> Encoded);

    WasiCryptoExpect<std::vector<uint8_t>> exportPem();

    WasiCryptoExpect<std::vector<uint8_t>> exportPkcs8();

    WasiCryptoExpect<std::vector<uint8_t>> exportLocal();

    EvpPkeyPtr Ctx;
  };

  class KeyPair final : public Signatures::KeyPair {
  public:
    KeyPair(EvpPkeyPtr Ctx) : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<std::unique_ptr<KeyPair>>
    import(Span<const uint8_t> Encoded, __wasi_keypair_encoding_e_t Encoding);

    static WasiCryptoExpect<std::unique_ptr<KeyPair>>
    generate(std::shared_ptr<Options> OptOptions);

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_keypair_encoding_e_t Encoding) override;

    WasiCryptoExpect<std::unique_ptr<Signatures::SignState>>
    openSignState() override;

    WasiCryptoExpect<std::unique_ptr<Signatures::PublicKey>>
    publicKey() override;

    WasiCryptoExpect<std::unique_ptr<Signatures::SecretKey>>
    secretKey() override;

  private:
    static WasiCryptoExpect<std::unique_ptr<KeyPair>>
    importPem(Span<uint8_t const> Encoded);

    static WasiCryptoExpect<std::unique_ptr<KeyPair>>
    importPkcs8(Span<uint8_t const> Encoded);

    static WasiCryptoExpect<std::unique_ptr<KeyPair>>
    importLocal(Span<uint8_t const> Encoded);

    WasiCryptoExpect<std::vector<uint8_t>> exportPem();

    WasiCryptoExpect<std::vector<uint8_t>> exportPkcs8();

    WasiCryptoExpect<std::vector<uint8_t>> exportLocal();

    EvpPkeyPtr Ctx;
  };

  class Signature final : public Signatures::Signature {
  public:
    Signature(std::vector<uint8_t> &&Data)
        : Signatures::Signature(getAlg(), std::move(Data)) {}

    static WasiCryptoExpect<std::unique_ptr<Signature>>
    import(Span<const uint8_t> Encoded, __wasi_signature_encoding_e_t Encoding);

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_signature_encoding_e_t Encoding) override;
  };

  class SignState final : public Signatures::SignState {
  public:
    SignState(EvpMdCtxPtr Ctx) : Ctx(std::move(Ctx)) {}

    WasiCryptoExpect<void> update(Span<uint8_t const> Data) override;

    WasiCryptoExpect<std::unique_ptr<Signatures::Signature>> sign() override;

  private:
    EvpMdCtxPtr Ctx;
  };

  class VerificationState final : public Signatures::VerificationState {
  public:
    VerificationState(EvpMdCtxPtr Ctx) : Ctx(std::move(Ctx)) {}

    WasiCryptoExpect<void> update(Span<uint8_t const> Data) override;

    WasiCryptoExpect<void>
    verify(std::shared_ptr<Signatures::Signature> Sig) override;

  private:
    EvpMdCtxPtr Ctx;
  };

  static EvpPkeyPtr initRsa();
};

using RsaPkcs12048SHA256 = Rsa<RSA_PKCS1_PADDING, 2048, 256>;
using RsaPkcs12048SHA384 = Rsa<RSA_PKCS1_PADDING, 2048, 384>;
using RsaPkcs12048SHA512 = Rsa<RSA_PKCS1_PADDING, 2048, 512>;

using RsaPkcs13072SHA384 = Rsa<RSA_PKCS1_PADDING, 3072, 384>;
using RsaPkcs13072SHA512 = Rsa<RSA_PKCS1_PADDING, 3072, 512>;

using RsaPkcs14096SHA512 = Rsa<RSA_PKCS1_PADDING, 4096, 512>;

using RsaPss2048SHA256 = Rsa<RSA_PKCS1_PSS_PADDING, 2048, 256>;
using RsaPss2048SHA384 = Rsa<RSA_PKCS1_PSS_PADDING, 2048, 384>;
using RsaPss2048SHA512 = Rsa<RSA_PKCS1_PSS_PADDING, 2048, 512>;

using RsaPss3072SHA384 = Rsa<RSA_PKCS1_PSS_PADDING, 3072, 384>;
using RsaPss3072SHA512 = Rsa<RSA_PKCS1_PSS_PADDING, 3072, 512>;

using RsaPss4096SHA512 = Rsa<RSA_PKCS1_PSS_PADDING, 4096, 512>;

} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
