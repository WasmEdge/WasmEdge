// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/kx/dh/x25519.h - X25519 alg implement ===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of x25519 algorithm.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "kx/options.h"
#include "utils/error.h"
#include "utils/evp_wrapper.h"
#include "utils/optional.h"
#include "utils/secret_vec.h"

#include "common/span.h"

#include <utility>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

class X25519 {
public:
  class PublicKey {
  public:
    PublicKey(EvpPkeyPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    PublicKey(SharedEvpPkey Ctx) noexcept : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<PublicKey>
    import(Span<const uint8_t> Encoded,
           __wasi_publickey_encoding_e_t Encoding) noexcept;

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_publickey_encoding_e_t Encoding) const noexcept;

    WasiCryptoExpect<void> verify() const noexcept;

    const auto &raw() const { return Ctx; }

  private:
    SharedEvpPkey Ctx;
  };

  class KeyPair;

  class SecretKey {
  public:
    SecretKey(EvpPkeyPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    SecretKey(SharedEvpPkey Ctx) noexcept : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<SecretKey>
    import(Span<const uint8_t> Encoded,
           __wasi_secretkey_encoding_e_t Encoding) noexcept;

    WasiCryptoExpect<SecretVec>
    exportData(__wasi_secretkey_encoding_e_t Encoding) const noexcept;

    WasiCryptoExpect<PublicKey> publicKey() const noexcept;

    WasiCryptoExpect<SecretVec> dh(const PublicKey &Pk) const noexcept;

    WasiCryptoExpect<KeyPair> toKeyPair(const PublicKey &Pk) const noexcept;

  private:
    SharedEvpPkey Ctx;
  };

  class KeyPair {
  public:
    KeyPair(EvpPkeyPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    KeyPair(SharedEvpPkey Ctx) noexcept : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<KeyPair>
    generate(OptionalRef<const Options> Options) noexcept;

    static WasiCryptoExpect<KeyPair>
    import(Span<const uint8_t> Raw,
           __wasi_keypair_encoding_e_t Encoding) noexcept;

    WasiCryptoExpect<PublicKey> publicKey() const noexcept;

    WasiCryptoExpect<SecretKey> secretKey() const noexcept;

    WasiCryptoExpect<SecretVec>
    exportData(__wasi_keypair_encoding_e_t Encoding) const noexcept;

  private:
    SharedEvpPkey Ctx;
  };
};

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
