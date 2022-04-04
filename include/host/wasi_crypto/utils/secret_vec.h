// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/utils/secret_vec.h - Secret Vec definition ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains definition of secret vec
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/utils/error.h"
#include "openssl/crypto.h"
#include "openssl/rand.h"

#include <climits>
#include <cstdint>
#include <memory>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

/// A vector wrapper, but swipe secret key info on destory
class SecretVec {
public:
  SecretVec(std::vector<uint8_t> Data) noexcept : Data(std::move(Data)) {}

  SecretVec(Span<const uint8_t> Data) noexcept
      : Data(Data.begin(), Data.end()) {}

  SecretVec(size_t Size) noexcept : Data(Size) {}

  ~SecretVec() noexcept { OPENSSL_cleanse(Data.data(), Data.size()); }

  std::vector<uint8_t> &raw() noexcept { return Data; }

  /// Generate random size vector. Notice Size shouldn't beyond
  /// std::numeric_limits<int>::max() because of the limitations of openssl
  template <size_t Size>
  static WasiCryptoExpect<std::shared_ptr<SecretVec>> random() noexcept {
    static_assert(
        Size <= std::numeric_limits<int>::max(),
        "Random key size shouldn't beyond std::numeric_limits<int>::max()");

    auto Res = std::make_shared<SecretVec>(Size);
    ensureOrReturn(RAND_bytes(Res->raw().data(), static_cast<int>(Size)),
                   __WASI_CRYPTO_ERRNO_RNG_ERROR);
    return Res;
  }

private:
  std::vector<uint8_t> Data;
};

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge