// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/symmetric/hash/sha2.h - Sha2 class declaration --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of Sha2 class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/symmetric/hash/helper.h"
#include "host/wasi_crypto/symmetric/options.h"
#include "host/wasi_crypto/utils/evp_wrapper.h"
#include "host/wasi_crypto/utils/optional.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

template <int ShaNid> class Sha2 {
public:
  /// in fact, sha2 key will never produce. This design for remove forward
  /// declare
  class Key : public HashKey<Key> {};

  class State : public HashState<Key> {
  public:
    State(EvpMdCtxPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<State>
    open(OptionalRef<Options> OptOption) noexcept;

    WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) noexcept;

    WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) noexcept;

  private:
    const std::shared_ptr<EVP_MD_CTX> Ctx;
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