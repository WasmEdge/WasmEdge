// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/symmetric/mac/hmac.h - Hmac class declaration ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of Hmac class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/symmetric/mac/helper.h"
#include "host/wasi_crypto/symmetric/options.h"
#include "host/wasi_crypto/utils/evp_wrapper.h"
#include "host/wasi_crypto/utils/optional.h"
#include "host/wasi_crypto/utils/secret_key.h"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

template <int ShaNid> class Hmac {
public:
  class Key {
  public:
    Key(std::shared_ptr<SecretKey> Data)
        : Data(std::move(Data)) {}

    static WasiCryptoExpect<Key> import(Span<const uint8_t> Data) noexcept;

    static WasiCryptoExpect<Key>
    generate(OptionalRef<Options> Options) noexcept;

    std::vector<uint8_t> exportData() const noexcept { return Data->raw(); }

    const std::vector<uint8_t> &ref() const noexcept { return Data->raw(); }

  private:
    const std::shared_ptr<SecretKey> Data;
  };

  class State : public MacState<Key> {
  public:
    State(EvpMdCtxPtr Ctx) : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<State>
    open(Key &Key, OptionalRef<Options> OptOption) noexcept;

    /// Adds input data to the state.
    ///
    /// @param[in] Data Input data
    /// @return Nothing or WasiCrypto error
    WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) noexcept;

    /// authenticates the input received up to the function call.
    ///
    ///
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