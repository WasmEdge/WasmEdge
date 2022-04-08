// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/signatures/ecdsa.h - Ecdsa algorithm implement --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of ecdsa relative algorithm
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/signatures/options.h"
#include "host/wasi_crypto/utils/error.h"
#include "host/wasi_crypto/utils/evp_wrapper.h"
#include "host/wasi_crypto/utils/optional.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

template <int CurveNid> class Ecdsa {
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
    const std::vector<uint8_t> Data;
  };

  class SignState {
  public:
    SignState(EvpMdCtxPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    WasiCryptoExpect<void> update(Span<const uint8_t> Input) noexcept;

    WasiCryptoExpect<Signature> sign() noexcept;

  private:
    std::shared_ptr<EVP_MD_CTX> Ctx;
  };

  class VerificationState {
  public:
    VerificationState(EvpMdCtxPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    WasiCryptoExpect<void> update(Span<const uint8_t> Input) noexcept;

    WasiCryptoExpect<void> verify(const Signature &Sig) noexcept;

  private:
    std::shared_ptr<EVP_MD_CTX> Ctx;
  };

  class PublicKey {
  public:
    PublicKey(EvpPkeyPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<PublicKey>
    import(Span<const uint8_t> Encoded,
           __wasi_publickey_encoding_e_t Encoding) noexcept;

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_publickey_encoding_e_t Encoding) const noexcept;

    WasiCryptoExpect<void> verify() const noexcept;

    WasiCryptoExpect<VerificationState> openVerificationState() const noexcept;

  private:
    static WasiCryptoExpect<PublicKey> importPkcs8(Span<const uint8_t> EncodeSd,
                                                   bool Compressed) noexcept;

    static WasiCryptoExpect<PublicKey> importPem(Span<const uint8_t> Encoded,
                                                 bool Compressed) noexcept;

    static WasiCryptoExpect<PublicKey> importSec(Span<const uint8_t> Encoded,
                                                 bool Compressed) noexcept;

    static WasiCryptoExpect<EvpPkeyPtr> checkValid(EvpPkeyPtr Ctx,
                                                   bool Compressed) noexcept;

    WasiCryptoExpect<std::vector<uint8_t>>
    exportSec(bool Compressed) const noexcept;

    WasiCryptoExpect<std::vector<uint8_t>>
    exportPem(bool Compressed) const noexcept;

    WasiCryptoExpect<std::vector<uint8_t>>
    exportPkcs8(bool Compressed) const noexcept;

    std::shared_ptr<EVP_PKEY> Ctx;
  };

  class KeyPair;

  class SecretKey {
  public:
    SecretKey(EvpPkeyPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<SecretKey>
    import(Span<const uint8_t> Encoded,
           __wasi_secretkey_encoding_e_t Encoding) noexcept;

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_secretkey_encoding_e_t Encoding) const noexcept;

    WasiCryptoExpect<PublicKey> publicKey() const noexcept;

    WasiCryptoExpect<KeyPair> toKeyPair(const PublicKey &Pk) const noexcept;

  private:
    static WasiCryptoExpect<SecretKey>
    importPkcs8(Span<const uint8_t> Encoded) noexcept;

    static WasiCryptoExpect<SecretKey>
    importPem(Span<const uint8_t> Encoded) noexcept;

    static WasiCryptoExpect<SecretKey>
    importRaw(Span<const uint8_t> Encoded) noexcept;

    static WasiCryptoExpect<EvpPkeyPtr> checkValid(EvpPkeyPtr Ctx) noexcept;

    WasiCryptoExpect<std::vector<uint8_t>> exportPkcs8() const noexcept;

    WasiCryptoExpect<std::vector<uint8_t>> exportPem() const noexcept;

    WasiCryptoExpect<std::vector<uint8_t>> exportRaw() const noexcept;

    std::shared_ptr<EVP_PKEY> Ctx;
  };

  class KeyPair {
  public:
    KeyPair(EvpPkeyPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<KeyPair>
    generate(OptionalRef<const Options> Options) noexcept;

    static WasiCryptoExpect<KeyPair>
    import(Span<const uint8_t> Encoded,
           __wasi_keypair_encoding_e_t Encoding) noexcept;

    static WasiCryptoExpect<EvpPkeyPtr> checkValid(EvpPkeyPtr Ctx,
                                                   bool Compressed) noexcept;

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_keypair_encoding_e_t Encoding) const noexcept;

    WasiCryptoExpect<PublicKey> publicKey() const noexcept;

    WasiCryptoExpect<SecretKey> secretKey() const noexcept;

    WasiCryptoExpect<SignState> openSignState() const noexcept;

  private:
    static WasiCryptoExpect<KeyPair> importPkcs8(Span<const uint8_t> Encoded,
                                                 bool Compressed) noexcept;

    static WasiCryptoExpect<KeyPair> importPem(Span<const uint8_t> Encoded,
                                               bool Compressed) noexcept;

    static WasiCryptoExpect<KeyPair>
    importRaw(Span<const uint8_t> Encoded) noexcept;

    static WasiCryptoExpect<EvpPkeyPtr> checkValid(EvpPkeyPtr Ctx) noexcept;

    WasiCryptoExpect<std::vector<uint8_t>> exportPkcs8() const noexcept;

    WasiCryptoExpect<std::vector<uint8_t>> exportPem() const noexcept;

    WasiCryptoExpect<std::vector<uint8_t>> exportRaw() const noexcept;

    std::shared_ptr<EVP_PKEY> Ctx;
  };
};

using EcdsaP256 = Ecdsa<NID_X9_62_prime256v1>;
using EcdsaK256 = Ecdsa<NID_secp256k1>;

} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
