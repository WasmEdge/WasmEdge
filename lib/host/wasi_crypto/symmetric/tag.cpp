// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/tag.h"
#include "host/wasi_crypto/symmetric/alg.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

//SymmetricTag::SymmetricTag(SymmetricAlgorithm Alg, Span<uint8_t> Raw)
//    : Alg(Alg), Raw(Raw.begin(),  Raw.end()) {}

WasiCryptoExpect<void> SymmetricTag::verify(Span<uint8_t const> RawTag) {
  if (!std::equal(RawTag.begin(), RawTag.end(), Raw.begin(), Raw.end())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  return {};
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
