// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/mac/mac_state.h"
#include "host/wasi_crypto/symmetric/options.h"
#include "host/wasi_crypto/util.h"

#include "openssl/evp.h"
#include "openssl/hmac.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

template <int Sha> class HmacSha2 {
public:
  class KeyBuilder : public Key::Builder {
  public:
    using Builder::Builder;

    WasiCryptoExpect<std::unique_ptr<Key>>
    generate(std::shared_ptr<Option> OptOption) override;

    WasiCryptoExpect<std::unique_ptr<Key>>
    import(Span<uint8_t const> Raw) override;

    WasiCryptoExpect<__wasi_size_t> keyLen() override;
  };

  class State : public MACState {
    using EvpMdCtxPtr = OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free>;

  public:
    State(std::shared_ptr<Option> OptOption, EVP_MD_CTX *Ctx)
        : OptOption(OptOption), Ctx(Ctx) {}

    static WasiCryptoExpect<std::unique_ptr<State>>
    open(std::shared_ptr<Key> OptKey, std::shared_ptr<Option> OptOption);

    WasiCryptoExpect<std::vector<uint8_t>>
    optionsGet(std::string_view Name) override;

    WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) override;

    WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override;

    WasiCryptoExpect<Tag> squeezeTag() override;

  private:
    std::shared_ptr<Option> OptOption;
    EvpMdCtxPtr Ctx;
  };
};

using HmacSha256 = HmacSha2<256>;
using HmacSha512 = HmacSha2<512>;

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
