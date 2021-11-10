// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/random.h"
#include "openssl/rand.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<void> CryptoRandom::fill(Span<uint8_t> Bytes) {
  if(!RAND_bytes(Bytes.data(), Bytes.size())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_RNG_ERROR);
  }
  return {};
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
