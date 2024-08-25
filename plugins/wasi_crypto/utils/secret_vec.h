// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/utils/secret_vec.h - Secret Vec def --===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of the secret vec.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "utils/error.h"

#include "common/span.h"

#include <openssl/crypto.h>
#include <openssl/rand.h>

#include <climits>
#include <cstdint>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

/// A vector wrapper, but swipe the secret key info on destroy.
class SecretVec {
public:
  SecretVec(const SecretVec &) = default;
  SecretVec &operator=(const SecretVec &) = default;
  SecretVec &operator=(SecretVec &&) noexcept = default;
  SecretVec(SecretVec &&) noexcept = default;

  SecretVec(Span<const uint8_t> Data) noexcept
      : Data(Data.begin(), Data.end()) {}

  SecretVec(size_t Size) noexcept : Data(Size) {}

  ~SecretVec() noexcept { OPENSSL_cleanse(Data.data(), Data.size()); }

  auto begin() noexcept { return Data.begin(); }
  auto begin() const noexcept { return Data.begin(); }

  auto end() noexcept { return Data.end(); }
  auto end() const noexcept { return Data.end(); }

  auto size() const noexcept { return Data.size(); }

  auto data() noexcept { return Data.data(); }
  auto data() const noexcept { return Data.data(); }

  using difference_type = std::vector<uint8_t>::difference_type;

  /// Generate random size vector. Notice that the size shouldn't beyond
  /// std::numeric_limits<int>::max() because of the limitations of openssl.
  template <size_t Size> static WasiCryptoExpect<SecretVec> random() noexcept {
    static_assert(
        Size <= std::numeric_limits<int>::max(),
        "Random key size shouldn't beyond std::numeric_limits<int>::max()");

    SecretVec Res(Size);
    ensureOrReturn(RAND_bytes(Res.data(), static_cast<int>(Size)),
                   __WASI_CRYPTO_ERRNO_RNG_ERROR);
    return Res;
  }

private:
  std::vector<uint8_t> Data;
};

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
