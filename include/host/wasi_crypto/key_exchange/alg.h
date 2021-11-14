// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/util.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

enum class KxAlgorithm {
  X25519,
  Kyber768
};

template<>
constexpr WasiCryptoExpect<KxAlgorithm>
tryFrom(std::string_view AlgStr) noexcept {
  if (AlgStr == "X25519") {
    return KxAlgorithm::X25519;
  }
  if (AlgStr == "KYBER768") {
    return KxAlgorithm::Kyber768;
  }
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
