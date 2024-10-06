// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/symmetric/mac.h - Mac related --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of Mac and related classes.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "symmetric/options.h"
#include "symmetric/tag.h"
#include "utils/evp_wrapper.h"
#include "utils/optional.h"
#include "utils/secret_vec.h"

#include <cstdint>
#include <optional>
#include <shared_mutex>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

/// Mac invalid operation, every mac state should inherent from this class
///
/// More detailed:
/// https://github.com/WebAssembly/wasi-crypto/blob/main/docs/wasi-crypto.md#message-authentication-codes
template <typename Key> class MacState {
public:
  /// Current mac not support any options.
  WasiCryptoExpect<size_t> optionsGet(std::string_view,
                                      Span<uint8_t>) const noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  /// Current mac not support any options.
  WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view) const noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  WasiCryptoExpect<void> squeeze(Span<uint8_t>) noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
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

  WasiCryptoExpect<size_t> maxTagLen() const noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
};

template <int ShaNid> class Hmac {
public:
  class Key {
  public:
    Key(SecretVec Data) noexcept : Data(std::move(Data)) {}

    static WasiCryptoExpect<Key> import(Span<const uint8_t> Data) noexcept;

    static WasiCryptoExpect<Key>
    generate(OptionalRef<const Options> Options) noexcept;

    SecretVec exportData() const noexcept { return Data; }

    const SecretVec &ref() const noexcept { return Data; }

  private:
    SecretVec Data;
  };

  class State : public MacState<Key> {
  public:
    State(EvpMdCtxPtr Ctx) noexcept
        : Ctx(std::make_shared<Inner>(std::move(Ctx))) {}

    static WasiCryptoExpect<State>
    open(const Key &Key, OptionalRef<const Options> OptOption) noexcept;

    /// Adds input data to the state.
    ///
    /// @param[in] Data the input data.
    /// @return Nothing or WasiCrypto error.
    WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) noexcept;

    /// Authenticates the input received up to the function call.
    /// If the finalization is required, the implementation MUST duplicate the
    /// internal state and apply the finalization on the copy, leaving the state
    /// unchanged from the guest perspective.
    ///
    /// @return Nothing or WasiCrypto error.
    WasiCryptoExpect<Tag> squeezeTag() noexcept;

    WasiCryptoExpect<State> clone() const noexcept;

  private:
    struct Inner {
      Inner(EvpMdCtxPtr RawCtx) noexcept : RawCtx(std::move(RawCtx)) {}
      EvpMdCtxPtr RawCtx;
      std::mutex Mutex;
    };
    std::shared_ptr<Inner> Ctx;
  };

private:
  constexpr static size_t getKeySize() noexcept;
};

using HmacSha256 = Hmac<NID_sha256>;
using HmacSha512 = Hmac<NID_sha512>;

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
