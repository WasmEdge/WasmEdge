// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/signatures/rsa.h - Rsa alg implement -===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of rsa and related classes.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "signatures/options.h"
#include "utils/error.h"
#include "utils/evp_wrapper.h"
#include "utils/optional.h"
#include "utils/secret_vec.h"

#include "common/span.h"

#include <openssl/evp.h>
#include <openssl/rsa.h>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

template <int PadMode, int KeyBits, int ShaNid> class Rsa {
public:
  class Signature {
  public:
    Signature(std::vector<uint8_t> &&Data) noexcept : Data(std::move(Data)) {}

    static WasiCryptoExpect<Signature>
    import(Span<const uint8_t> Encoded,
           __wasi_signature_encoding_e_t Encoding) noexcept;

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_signature_encoding_e_t Encoding) const noexcept;

    const std::vector<uint8_t> &ref() const { return Data; }

  private:
    std::vector<uint8_t> Data;
  };

  class SignState {
  public:
    SignState(EvpMdCtxPtr Ctx) noexcept
        : Ctx(std::make_shared<Inner>(std::move(Ctx))) {}

    WasiCryptoExpect<void> update(Span<uint8_t const> Data) noexcept;

    WasiCryptoExpect<Signature> sign() noexcept;

  private:
    struct Inner {
      Inner(EvpMdCtxPtr RawCtx) noexcept : RawCtx(std::move(RawCtx)) {}
      std::mutex Mutex;
      EvpMdCtxPtr RawCtx;
    };
    std::shared_ptr<Inner> Ctx;
  };

  class VerificationState {
  public:
    VerificationState(EvpMdCtxPtr Ctx) noexcept
        : Ctx(std::make_shared<Inner>(std::move(Ctx))) {}

    WasiCryptoExpect<void> update(Span<uint8_t const> Data) noexcept;

    WasiCryptoExpect<void> verify(const Signature &Sig) noexcept;

  private:
    struct Inner {
      Inner(EvpMdCtxPtr RawCtx) noexcept : RawCtx(std::move(RawCtx)) {}
      std::mutex Mutex;
      EvpMdCtxPtr RawCtx;
    };
    std::shared_ptr<Inner> Ctx;
  };

  class PublicKey {
  public:
    PublicKey(EvpPkeyPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    PublicKey(SharedEvpPkey Ctx) noexcept : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<PublicKey>
    import(Span<uint8_t const> Encoded,
           __wasi_publickey_encoding_e_t Encoding) noexcept;

    WasiCryptoExpect<void> verify() const noexcept;

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_publickey_encoding_e_t Encoding) const noexcept;

    WasiCryptoExpect<VerificationState> openVerificationState() const noexcept;

  private:
    static WasiCryptoExpect<PublicKey>
    importPem(Span<uint8_t const> Encoded) noexcept;

    static WasiCryptoExpect<PublicKey>
    importPkcs8(Span<uint8_t const> Encoded) noexcept;

    static WasiCryptoExpect<EvpPkeyPtr> checkValid(EvpPkeyPtr Ctx) noexcept;

    WasiCryptoExpect<std::vector<uint8_t>> exportPem() const noexcept;

    WasiCryptoExpect<std::vector<uint8_t>> exportPkcs8() const noexcept;

    SharedEvpPkey Ctx;
  };

  class KeyPair;

  class SecretKey {
  public:
    SecretKey(EvpPkeyPtr Ctx) : Ctx(std::move(Ctx)) {}

    SecretKey(SharedEvpPkey Ctx) noexcept : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<SecretKey>
    import(Span<const uint8_t> Encoded,
           __wasi_secretkey_encoding_e_t Encoding) noexcept;

    WasiCryptoExpect<SecretVec>
    exportData(__wasi_secretkey_encoding_e_t Encoding) const noexcept;

    WasiCryptoExpect<PublicKey> publicKey() const noexcept;

    WasiCryptoExpect<KeyPair> toKeyPair(const PublicKey &Pk) const noexcept;

  private:
    static WasiCryptoExpect<SecretKey>
    importPem(Span<uint8_t const> Encoded) noexcept;

    static WasiCryptoExpect<SecretKey>
    importPkcs8(Span<uint8_t const> Encoded) noexcept;

    static WasiCryptoExpect<EvpPkeyPtr> checkValid(EvpPkeyPtr Ctx) noexcept;

    WasiCryptoExpect<SecretVec> exportPem() const noexcept;

    WasiCryptoExpect<SecretVec> exportPkcs8() const noexcept;

    SharedEvpPkey Ctx;
  };

  class KeyPair {
  public:
    KeyPair(EvpPkeyPtr Ctx) : Ctx(std::move(Ctx)) {}

    KeyPair(SharedEvpPkey Ctx) noexcept : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<KeyPair>
    import(Span<const uint8_t> Encoded,
           __wasi_keypair_encoding_e_t Encoding) noexcept;

    static WasiCryptoExpect<KeyPair>
    generate(OptionalRef<const Options> OptOptions) noexcept;

    WasiCryptoExpect<SecretVec>
    exportData(__wasi_keypair_encoding_e_t Encoding) const noexcept;

    WasiCryptoExpect<PublicKey> publicKey() const noexcept;

    WasiCryptoExpect<SecretKey> secretKey() const noexcept;

    WasiCryptoExpect<SignState> openSignState() const noexcept;

  private:
    static WasiCryptoExpect<KeyPair>
    importPem(Span<uint8_t const> Encoded) noexcept;

    static WasiCryptoExpect<KeyPair>
    importPkcs8(Span<uint8_t const> Encoded) noexcept;

    static WasiCryptoExpect<EvpPkeyPtr> checkValid(EvpPkeyPtr Ctx) noexcept;

    WasiCryptoExpect<SecretVec> exportPem() const noexcept;

    WasiCryptoExpect<SecretVec> exportPkcs8() const noexcept;

    SharedEvpPkey Ctx;
  };

private:
  static constexpr size_t getSigSize() { return KeyBits / 8; }

  static const EVP_MD *getShaCtx() { return EVP_get_digestbynid(ShaNid); }
};

using RSA_PKCS1_2048_SHA256 = Rsa<RSA_PKCS1_PADDING, 2048, NID_sha256>;
using RSA_PKCS1_2048_SHA384 = Rsa<RSA_PKCS1_PADDING, 2048, NID_sha384>;
using RSA_PKCS1_2048_SHA512 = Rsa<RSA_PKCS1_PADDING, 2048, NID_sha512>;

using RSA_PKCS1_3072_SHA384 = Rsa<RSA_PKCS1_PADDING, 3072, NID_sha384>;
using RSA_PKCS1_3072_SHA512 = Rsa<RSA_PKCS1_PADDING, 3072, NID_sha512>;

using RSA_PKCS1_4096_SHA512 = Rsa<RSA_PKCS1_PADDING, 4096, NID_sha512>;

using RSA_PSS_2048_SHA256 = Rsa<RSA_PKCS1_PSS_PADDING, 2048, NID_sha256>;
using RSA_PSS_2048_SHA384 = Rsa<RSA_PKCS1_PSS_PADDING, 2048, NID_sha384>;
using RSA_PSS_2048_SHA512 = Rsa<RSA_PKCS1_PSS_PADDING, 2048, NID_sha512>;

using RSA_PSS_3072_SHA384 = Rsa<RSA_PKCS1_PSS_PADDING, 3072, NID_sha384>;
using RSA_PSS_3072_SHA512 = Rsa<RSA_PKCS1_PSS_PADDING, 3072, NID_sha512>;

using RSA_PSS_4096_SHA512 = Rsa<RSA_PKCS1_PSS_PADDING, 4096, NID_sha512>;

} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
