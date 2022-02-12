// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/mac/mac_state.h"
#include "host/wasi_crypto/symmetric/options.h"

#include "host/wasi_crypto/evpwrapper.h"
#include "openssl/hmac.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

template <uint32_t ShaNid> class HmacSha2 {
public:
  class KeyBuilder final : public Key::Builder {
  public:
    using Builder::Builder;

    WasiCryptoExpect<std::unique_ptr<Key>>
    generate(std::shared_ptr<Options> OptOption) override;

    WasiCryptoExpect<std::unique_ptr<Key>>
    import(Span<uint8_t const> Raw) override;
  };

  class State final : public MACState {

  public:
    State(std::shared_ptr<Options> OptOption, EvpMdCtxPtr Ctx)
        : OptOption(OptOption), Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<std::unique_ptr<State>>
    open(std::shared_ptr<Key> OptKey, std::shared_ptr<Options> OptOption);

    WasiCryptoExpect<std::vector<uint8_t>>
    optionsGet(std::string_view Name) override;

    WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) override;

    WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override;

    WasiCryptoExpect<Tag> squeezeTag() override;

  private:
    std::shared_ptr<Options> OptOption;
    EvpMdCtxPtr Ctx;
  };

private:
  constexpr static size_t getKeySize() {
    if constexpr (ShaNid == NID_sha256) {
      return 32;
    }
    if constexpr (ShaNid == NID_sha512) {
      return 64;
    }
  }
};

using HmacSha256 = HmacSha2<NID_sha256>;
using HmacSha512 = HmacSha2<NID_sha512>;

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
