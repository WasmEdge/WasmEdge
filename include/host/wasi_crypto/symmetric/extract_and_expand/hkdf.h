// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/evpwrapper.h"
#include "host/wasi_crypto/symmetric/extract_and_expand/expand.h"
#include "host/wasi_crypto/symmetric/extract_and_expand/extract.h"
#include "host/wasi_crypto/symmetric/state.h"
#include "openssl/kdf.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

template <int ShaNid> class Hkdf {
public:
  class Extract {
  public:
    class KeyBuilder final : public Key::Builder {
    public:
      using Builder::Builder;

      WasiCryptoExpect<std::unique_ptr<Key>>
      generate(std::shared_ptr<Options> OptOption) override;

      WasiCryptoExpect<std::unique_ptr<Key>>
      import(Span<uint8_t const> Raw) override;
    };

    class State final : public ExtractState {
    public:
      State(std::shared_ptr<Options> OptOption, EvpPkeyCtxPtr Ctx)
          : OptOption(OptOption), Ctx(std::move(Ctx)) {}

      static WasiCryptoExpect<std::unique_ptr<State>>
      open(std::shared_ptr<Key> OptKey, std::shared_ptr<Options> OptOption);

      /// absorbs the salt of the key(Extract)/info(Expand) information.
      WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override;

      /// Extract:
      /// returns the PRK, whose algorithm type is set to the EXPAND counterpart
      /// of the EXTRACT operation
      WasiCryptoExpect<std::unique_ptr<Key>>
      squeezeKey(SymmetricAlgorithm Alg) override;

      WasiCryptoExpect<std::vector<uint8_t>>
      optionsGet(std::string_view Name) override;

      WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) override;

    private:
      std::shared_mutex Mutex;
      std::vector<uint8_t> Salt;
      std::shared_ptr<Options> OptOption;
      EvpPkeyCtxPtr Ctx;
    };
  };
  class Expand {
  public:
    class KeyBuilder final : public Key::Builder {
    public:
      using Builder::Builder;

      WasiCryptoExpect<std::unique_ptr<Key>>
      generate(std::shared_ptr<Options> OptOption) override;

      WasiCryptoExpect<std::unique_ptr<Key>>
      import(Span<uint8_t const> Raw) override;
    };
    class State final : public ExpandState {
    public:
      State(std::shared_ptr<Options> OptOption, EvpPkeyCtxPtr Ctx)
          : OptOption(OptOption), Ctx(std::move(Ctx)) {}

      static WasiCryptoExpect<std::unique_ptr<State>>
      open(std::shared_ptr<Key> OptKey, std::shared_ptr<Options> OptOption);

      WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override;

      WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) override;

      WasiCryptoExpect<std::vector<uint8_t>>
      optionsGet(std::string_view Name) override;

      WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) override;

    private:
      std::shared_ptr<Options> OptOption;
      EvpPkeyCtxPtr Ctx;
    };
  };

private:
  constexpr static uint32_t getKeySize();

  /// Type erasure to pass clang-tidy check
  constexpr static void *getShaCtx();
};

using Hkdf256Extract = Hkdf<NID_sha256>::Extract;
using Hkdf512Extract = Hkdf<NID_sha512>::Extract;
using Hkdf256Expand = Hkdf<NID_sha256>::Expand;
using Hkdf512Expand = Hkdf<NID_sha512>::Expand;

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
