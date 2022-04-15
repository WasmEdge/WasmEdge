// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/kx/kx.h"
#include "host/wasi_crypto/utils/error.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

WasiCryptoExpect<SecretVec> dh(PkVariant &PkVariant,
                               SkVariant &SkVariant) noexcept {
  return std::visit([](auto &&Pk, auto &&Sk) { return Sk.dh(Pk); }, PkVariant,
                    SkVariant);
}

WasiCryptoExpect<EncapsulatedSecret>
encapsulate(PkVariant &PkVariant) noexcept {
  return std::visit(
      [](auto &&) {
        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
      },
      PkVariant);
}

WasiCryptoExpect<std::vector<uint8_t>>
decapsulate(SkVariant &, Span<const uint8_t>) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge