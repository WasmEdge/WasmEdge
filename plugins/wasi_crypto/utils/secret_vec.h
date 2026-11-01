// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

/// A vector wrapper, but swipe the secret key info on destroy.
class SecretVec {
public:
  SecretVec() noexcept = default;
  SecretVec(const SecretVec &) = default;

  /// Copies then moves, so the replaced content goes through the wipe below.
  /// Defined after the class: constructing a `SecretVec` before `data()` is
  /// declared makes `Span` treat the class as a non-contiguous range.
  SecretVec &operator=(const SecretVec &Rhs);

  /// Wipe the replaced content before handing its buffer to `Rhs`.
  SecretVec &operator=(SecretVec &&Rhs) noexcept {
    if (this != &Rhs) {
      cleanse();
      Data.swap(Rhs.Data);
    }
    return *this;
  }

  SecretVec(SecretVec &&) noexcept = default;

  SecretVec(Span<const uint8_t> Data) noexcept
      : Data(Data.begin(), Data.end()) {}

  /// Take over the buffer so only one copy of the content exists.
  SecretVec(std::vector<uint8_t> &&Data) noexcept : Data(std::move(Data)) {}

  SecretVec(size_t Size) noexcept : Data(Size) {}

  ~SecretVec() noexcept { cleanse(); }

  void resize(size_t Size) {
    if (Size <= Data.size()) {
      if (Size < Data.size()) {
        OPENSSL_cleanse(Data.data() + Size, Data.size() - Size);
      }
      Data.resize(Size);
      return;
    }
    if (Size <= Data.capacity()) {
      Data.resize(Size);
      return;
    }
    std::vector<uint8_t> NewData(Size);
    for (size_t I = 0; I < Data.size(); ++I) {
      NewData[I] = Data[I];
    }
    cleanse();
    Data.swap(NewData);
  }

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
  void cleanse() noexcept {
    if (auto *Ptr = Data.data(); Ptr != nullptr && Data.capacity() != 0) {
      OPENSSL_cleanse(Ptr, Data.capacity());
    }
  }

  std::vector<uint8_t> Data;
};

inline SecretVec &SecretVec::operator=(const SecretVec &Rhs) {
  return *this = SecretVec(Rhs);
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
