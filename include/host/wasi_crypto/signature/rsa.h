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

template <int Pad, int Size, int Sha> class Rsa {
public:
  class PublicKey : public Signatures::PublicKey {
  public:
    PublicKey(EVP_PKEY *Pk) : Pk(Pk) {}

    static WasiCryptoExpect<std::unique_ptr<Rsa::PublicKey>>
    import(Span<uint8_t const> Encoded, __wasi_publickey_encoding_e_t Encoding);

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_publickey_encoding_e_t Encoding) override;

    WasiCryptoExpect<std::unique_ptr<Signatures::VerificationState>>
    openVerificationState() override;

  private:
    EvpPkeyPtr Pk;
  };

  class SecretKey : public Signatures::SecretKey {
  public:
    SecretKey(EVP_PKEY *Sk) : Sk(std::move(Sk)) {}

    static WasiCryptoExpect<std::unique_ptr<SecretKey>>
    import(Span<const uint8_t> Encoded, __wasi_secretkey_encoding_e_t Encoding);

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_secretkey_encoding_e_t Encoding) override;

  private:
    EvpPkeyPtr Sk;
  };

  class KeyPair : public Signatures::KeyPair {
  public:
    KeyPair(EVP_PKEY *Kp) : Kp(std::move(Kp)) {}

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
    EvpPkeyPtr Kp;
  };

  class Signature : public Signatures::Signature {
  public:
    Signature(std::vector<uint8_t> &&Sign) : Sign(std::move(Sign)) {}

    static WasiCryptoExpect<std::unique_ptr<Signature>>
    import(Span<const uint8_t> Encoded, __wasi_signature_encoding_e_t Encoding);

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_signature_encoding_e_t Encoding) override;

  private:
    std::vector<uint8_t> Sign;
  };

  class SignState : public Signatures::SignState {
  public:
    SignState(EVP_MD_CTX *MdCtx) : MdCtx(MdCtx) {}

    WasiCryptoExpect<void> update(Span<uint8_t const> Data) override;

    WasiCryptoExpect<std::unique_ptr<Signatures::Signature>> sign() override;

  private:
    EvpMdCtxPtr MdCtx;
  };

  class VerificationState : public Signatures::VerificationState {
  public:
    VerificationState(EVP_MD_CTX *MdCtx) : MdCtx(MdCtx) {}

    WasiCryptoExpect<void> update(Span<uint8_t const> Data) override;

    WasiCryptoExpect<void>
    verify(std::shared_ptr<Signatures::Signature> Sig) override;

  private:
    EvpMdCtxPtr MdCtx;
  };
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
