// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/symmetric/hash.h - Hash relative declaration ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of Hash relative class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/symmetric/options.h"
#include "host/wasi_crypto/symmetric/tag.h"
#include "host/wasi_crypto/utils/evp_wrapper.h"
#include "host/wasi_crypto/utils/optional.h"

#include <shared_mutex>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

/// Hash never have key, just a placement, every hash key should inherent from
/// this class
///
/// More detailed:
/// https://github.com/WebAssembly/wasi-crypto/blob/main/docs/wasi-crypto.md#hash-functions
template <typename Key> class HashKey {
public:
  static WasiCryptoExpect<Key> import(Span<const uint8_t>) noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_NOT_SUPPORTED);
  }

  static WasiCryptoExpect<Key> generate(OptionalRef<const Options>) noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_NOT_SUPPORTED);
  }

  SecretVec exportData() const noexcept { assumingUnreachable(); }
};

/// Hash invalid operation, every hash state should inherent from this class
template <typename Key> class HashState {
public:
  /// current hash not support any options
  WasiCryptoExpect<size_t> optionsGet(std::string_view,
                                      Span<uint8_t>) const noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  /// current hash not support any options
  WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view) const noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  WasiCryptoExpect<void> ratchet() noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<size_t> encrypt(Span<uint8_t>,
                                   Span<const uint8_t>) noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<Tag> encryptDetached(Span<uint8_t>,
                                        Span<const uint8_t>) noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<size_t> decrypt(Span<uint8_t>,
                                   Span<const uint8_t>) noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<size_t> decryptDetached(Span<uint8_t>, Span<const uint8_t>,
                                           Span<const uint8_t>) noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<Key> squeezeKey() noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<Tag> squeezeTag() noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<size_t> maxTagLen() const noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
};

template <int ShaNid> class Sha2 {
public:
  /// in fact, sha2 key will never produce. This design for remove forward
  /// declare
  class Key : public HashKey<Key> {};

  class State : public HashState<Key> {
  public:
    State(EvpMdCtxPtr Ctx) noexcept
        : Ctx(std::make_shared<Inner>(std::move(Ctx))) {}

    static WasiCryptoExpect<State>
    open(OptionalRef<const Options> OptOption) noexcept;

    WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) noexcept;

    WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) noexcept;

    WasiCryptoExpect<State> clone() const noexcept;

  private:
    struct Inner {
      Inner(EvpMdCtxPtr Ctx) noexcept : RawCtx(std::move(Ctx)) {}
      EvpMdCtxPtr RawCtx;
      std::shared_mutex Mutex;
    };
    std::shared_ptr<Inner> Ctx;
  };

private:
  /// return sha digest size
  constexpr static size_t getDigestSize() noexcept;
};

using Sha256 = Sha2<NID_sha256>;
using Sha512 = Sha2<NID_sha512>;
using Sha512_256 = Sha2<NID_sha512_256>;

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
