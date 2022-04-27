// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/kx/dh/ecdsa.h - Ecdsa algorithm implement -------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of ecdsa algorithm
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/kx/options.h"
#include "host/wasi_crypto/utils/error.h"
#include "host/wasi_crypto/utils/evp_wrapper.h"
#include "host/wasi_crypto/utils/optional.h"
#include "host/wasi_crypto/utils/secret_vec.h"

#include <optional>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

class Ecdsa {
public:
  class PublicKey {
  public:
    PublicKey(EvpPkeyPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<PublicKey>
    import(Span<const uint8_t> Encoded,
           __wasi_publickey_encoding_e_t Encoding) noexcept;

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_publickey_encoding_e_t Encoding) const noexcept;

    WasiCryptoExpect<void> verify() const noexcept;

    const auto &raw() const { return Ctx; }

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

    WasiCryptoExpect<SecretVec>
    exportData(__wasi_secretkey_encoding_e_t Encoding) const noexcept;

    WasiCryptoExpect<PublicKey> publicKey() const noexcept;

    WasiCryptoExpect<SecretVec> dh(const PublicKey &Pk) const noexcept;

    WasiCryptoExpect<KeyPair> toKeyPair(const PublicKey &Pk) const noexcept;

  private:
    static WasiCryptoExpect<SecretKey>
    importPkcs8(Span<const uint8_t> Encoded) noexcept;

    static WasiCryptoExpect<SecretKey>
    importPem(Span<const uint8_t> Encoded) noexcept;

    static WasiCryptoExpect<SecretKey>
    importRaw(Span<const uint8_t> Encoded) noexcept;

    static WasiCryptoExpect<EvpPkeyPtr> checkValid(EvpPkeyPtr Ctx) noexcept;

    WasiCryptoExpect<SecretVec> exportPkcs8() const noexcept;

    WasiCryptoExpect<SecretVec> exportPem() const noexcept;

    WasiCryptoExpect<SecretVec> exportRaw() const noexcept;

    std::shared_ptr<EVP_PKEY> Ctx;
  };

  class KeyPair {
  public:
    static WasiCryptoExpect<KeyPair>
    generate(OptionalRef<const Options> Options) noexcept;

    static WasiCryptoExpect<KeyPair>
    import(Span<const uint8_t> Raw,
           __wasi_keypair_encoding_e_t Encoding) noexcept;

    KeyPair(EvpPkeyPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    WasiCryptoExpect<PublicKey> publicKey() const noexcept;

    WasiCryptoExpect<SecretKey> secretKey() const noexcept;

    WasiCryptoExpect<SecretVec>
    exportData(__wasi_keypair_encoding_e_t Encoding) const noexcept;

  private:
    static WasiCryptoExpect<KeyPair> importPkcs8(Span<const uint8_t> Encoded,
                                                 bool Compressed) noexcept;

    static WasiCryptoExpect<KeyPair> importPem(Span<const uint8_t> Encoded,
                                               bool Compressed) noexcept;

    static WasiCryptoExpect<KeyPair>
    importRaw(Span<const uint8_t> Encoded) noexcept;

    static WasiCryptoExpect<EvpPkeyPtr> checkValid(EvpPkeyPtr Ctx,
                                                   bool Compressed) noexcept;

    WasiCryptoExpect<SecretVec> exportPkcs8() const noexcept;

    WasiCryptoExpect<SecretVec> exportPem() const noexcept;

    WasiCryptoExpect<SecretVec> exportRaw() const noexcept;

    std::shared_ptr<EVP_PKEY> Ctx;
  };
};

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
