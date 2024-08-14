// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/symmetric/kdf.h - Kdf related --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of Key derivation and related classes.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "symmetric/options.h"
#include "symmetric/tag.h"
#include "utils/error.h"
#include "utils/evp_wrapper.h"
#include "utils/optional.h"
#include "utils/secret_vec.h"

#include <mutex>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

/// Expand invalid operations, every expand state should inherent from this
/// class.
///
/// More detailed:
/// https://github.com/WebAssembly/wasi-crypto/blob/main/docs/wasi-crypto.md#key-derivation-using-extract-and-expand
template <typename Key> class ExpandState {
public:
  /// Current kdf not support any options.
  WasiCryptoExpect<size_t> optionsGet(std::string_view,
                                      Span<uint8_t>) const noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  /// Current kdf not support any options.
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

/// Extract invalid operations, every extract state should inherent from this
/// class.
template <typename Key> class ExtractState {
public:
  /// Current kdf not support any options.
  WasiCryptoExpect<size_t> optionsGet(std::string_view,
                                      Span<uint8_t>) const noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  /// Current kdf not support any options.
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

  WasiCryptoExpect<Tag> squeezeTag() noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<size_t> maxTagLen() const noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
};

template <int ShaNid> class Hkdf {
public:
  class Expand {
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

    class State : public ExpandState<Key> {
    public:
      static WasiCryptoExpect<State>
      open(const Key &Key, OptionalRef<const Options> OptOption) noexcept;

      State(EvpPkeyCtxPtr Ctx) noexcept
          : Ctx(std::make_shared<Inner>(std::move(Ctx))) {}

      /// absorb info information.
      WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) noexcept;

      /// derivation
      WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) noexcept;

      WasiCryptoExpect<State> clone() const noexcept;

    private:
      struct Inner {
        Inner(EvpPkeyCtxPtr RawCtx) : RawCtx(std::move(RawCtx)) {}
        EvpPkeyCtxPtr RawCtx;
        std::mutex Mutex;
      };
      std::shared_ptr<Inner> Ctx;
    };
  };

  class Extract {
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

    class State : public ExtractState<Key> {
    public:
      State(EvpPkeyCtxPtr Ctx) noexcept
          : Ctx(std::make_shared<Inner>(std::move(Ctx))) {}

      static WasiCryptoExpect<State>
      open(const Key &Key, OptionalRef<const Options> OptOption) noexcept;

      /// Absorbs the salt information.
      WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) noexcept;

      /// Returns the PRK, whose algorithm type is set to the EXPAND counterpart
      /// of the EXTRACT operation.
      WasiCryptoExpect<typename Expand::Key> squeezeKey() noexcept;

      WasiCryptoExpect<State> clone() const noexcept;

    private:
      struct Inner {
        Inner(EvpPkeyCtxPtr RawCtx) : RawCtx(std::move(RawCtx)) {}
        std::mutex Mutex;
        std::vector<uint8_t> Salt;
        EvpPkeyCtxPtr RawCtx;
      };
      std::shared_ptr<Inner> Ctx;
    };
  };

private:
  constexpr static uint32_t getKeySize() noexcept;

  constexpr static const EVP_MD *getShaCtx() noexcept;

  static WasiCryptoExpect<EvpPkeyCtxPtr> openStateImpl(Span<const uint8_t> Key,
                                                       int Mode) noexcept;
};

using HkdfSha256Extract = Hkdf<NID_sha256>::Extract;
using HkdfSha512Extract = Hkdf<NID_sha512>::Extract;
using HkdfSha256Expand = Hkdf<NID_sha256>::Expand;
using HkdfSha512Expand = Hkdf<NID_sha512>::Expand;

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
