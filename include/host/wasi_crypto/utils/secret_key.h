// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/utils/secret_key.h - Secret key definition ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains definition of secret key
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/utils/error.h"
#include "common/span.h"
#include "openssl/rand.h"

#include <climits>
#include <cstdint>
#include <memory>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

/// A vector wrapper, but swipe secret key info on destory
class SecretKey {
public:
  SecretKey(std::vector<uint8_t> Data) : Data(std::move(Data)) {}
  
  SecretKey(Span<const uint8_t> Data) : Data(Data.begin(), Data.end()) {}

  SecretKey(size_t Size) : Data(Size) {}

  ~SecretKey() { std::fill(Data.begin(), Data.end(), 0); }

  auto &raw() { return Data; }

  /// Generate random size vector. Notice Size shouldn't beyond INT_MAX
  /// because of the limitations of openssl
  static WasiCryptoExpect<std::shared_ptr<SecretKey>> random(size_t Size) {
    ensureOrReturn(Size <= INT_MAX, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    auto Res = std::make_shared<SecretKey>(Size);
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