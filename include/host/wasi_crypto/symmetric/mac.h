// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/symmetric/mac.h - Mac realtive declaration ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of Mac realtive.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/symmetric/options.h"
#include "host/wasi_crypto/symmetric/tag.h"
#include "host/wasi_crypto/utils/evp_wrapper.h"
#include "host/wasi_crypto/utils/optional.h"
#include "host/wasi_crypto/utils/secret_vec.h"

#include <cstdint>
#include <optional>
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
  /// current mac not support any options
  WasiCryptoExpect<size_t> optionsGet(std::string_view,
                                      Span<uint8_t>) const noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  /// current mac not support any options
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
                                           Span<uint8_t>) noexcept {
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
    Key(std::shared_ptr<SecretVec> Data) noexcept : Data(std::move(Data)) {}

    static WasiCryptoExpect<Key> import(Span<const uint8_t> Data) noexcept;

    static WasiCryptoExpect<Key>
    generate(OptionalRef<const Options> Options) noexcept;

    std::vector<uint8_t> exportData() const noexcept { return Data->raw(); }

    const std::vector<uint8_t> &ref() const noexcept { return Data->raw(); }

  private:
    const std::shared_ptr<SecretVec> Data;
  };

  class State : public MacState<Key> {
  public:
    State(EvpMdCtxPtr Ctx) : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<State>
    open(const Key &Key, OptionalRef<const Options> OptOption) noexcept;

    /// Adds input data to the state.
    ///
    /// @param[in] Data Input data
    /// @return Nothing or WasiCrypto error
    WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) noexcept;

    /// authenticates the input received up to the function call.
    /// If finalization is required, the implementation MUST duplicate the
    /// internal state and apply the finalization on the copy, leaving the state
    /// unchanged from the guest perspective.
    ///
    /// @return Nothing or WasiCrypto error
    WasiCryptoExpect<Tag> squeezeTag() noexcept;

  private:
    const std::shared_ptr<EVP_MD_CTX> Ctx;
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
