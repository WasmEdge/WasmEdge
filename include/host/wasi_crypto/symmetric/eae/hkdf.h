// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/symmetric/eae/hkdf.h - Hkdf class declaration ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of Hkdf class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/symmetric/eae/helper.h"
#include "host/wasi_crypto/symmetric/options.h"
#include "host/wasi_crypto/utils/evp_wrapper.h"
#include "host/wasi_crypto/utils/optional.h"
#include "host/wasi_crypto/utils/secret_vec.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

template <int ShaNid> class Hkdf {
public:
  class Expand {
  public:
    class Key {
    public:
      Key(std::shared_ptr<SecretVec> Data) noexcept : Data(std::move(Data)) {}

      static WasiCryptoExpect<Key> import(Span<const uint8_t> Data) noexcept;

      static WasiCryptoExpect<Key>
      generate(OptionalRef<Options> Options) noexcept;

      std::vector<uint8_t> exportData() const noexcept { return Data->raw(); }

      const std::vector<uint8_t> &ref() const noexcept { return Data->raw(); }

    private:
      const std::shared_ptr<SecretVec> Data;
    };

    class State : public ExpandState<Key> {
    public:
      static WasiCryptoExpect<State>
      open(Key &Key, OptionalRef<Options> OptOption) noexcept;

      State(EvpPkeyCtxPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

      WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) noexcept;

      WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) noexcept;

    private:
      const std::shared_ptr<EVP_PKEY_CTX> Ctx;
    };
  };

  class Extract {
  public:
    class Key {
    public:
      Key(std::shared_ptr<SecretVec> Data) noexcept
          : Data(std::move(Data)) {}

      static WasiCryptoExpect<Key> import(Span<const uint8_t> Data) noexcept;

      static WasiCryptoExpect<Key>
      generate(OptionalRef<Options> Options) noexcept;

      std::vector<uint8_t> exportData() const noexcept { return Data->raw(); }

      const std::vector<uint8_t> &ref() const noexcept { return Data->raw(); }

    private:
      const std::shared_ptr<SecretVec> Data;
    };

    class State : public ExtractState<Key> {
    public:
      State(EvpPkeyCtxPtr Ctx) noexcept
          : Ctx(std::make_shared<Inner>(std::move(Ctx))) {}

      static WasiCryptoExpect<State>
      open(Key &Key, OptionalRef<Options> OptOption) noexcept;

      /// absorbs the salt of the key(Extract)/info(Expand) information.
      WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) noexcept;

      /// Extract:
      /// returns the PRK, whose algorithm type is set to the EXPAND counterpart
      /// of the EXTRACT operation
      WasiCryptoExpect<typename Expand::Key> squeezeKey(Algorithm Alg) noexcept;

    private:
      struct Inner {
        Inner(EvpPkeyCtxPtr RawCtx) : RawCtx(std::move(RawCtx)) {}
        std::shared_mutex Mutex;
        std::vector<uint8_t> Salt;
        EvpPkeyCtxPtr RawCtx;
      };
      const std::shared_ptr<Inner> Ctx;
    };
  };

private:
  constexpr static uint32_t getKeySize() noexcept;

  /// Type erasure to pass clang-tidy check
  constexpr static void *getShaCtx() noexcept;

  constexpr static Symmetric::Algorithm getExpandAlg() noexcept;

  static WasiCryptoExpect<EvpPkeyCtxPtr>
  openStateImpl(const std::vector<uint8_t> &Key, int Mode) noexcept;
};

using HkdfSha256Extract = Hkdf<NID_sha256>::Extract;
using HkdfSha512Extract = Hkdf<NID_sha512>::Extract;
using HkdfSha256Expand = Hkdf<NID_sha256>::Expand;
using HkdfSha512Expand = Hkdf<NID_sha512>::Expand;

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
