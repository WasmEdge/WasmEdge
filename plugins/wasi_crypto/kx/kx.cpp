// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "kx/kx.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

WasiCryptoExpect<EncapsulatedSecret> encapsulate(PkVariant &) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::vector<uint8_t>>
decapsulate(SkVariant &, Span<const uint8_t>) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
