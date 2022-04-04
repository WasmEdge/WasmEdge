// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/kx/factory.h"

#include <utility>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

FactoryVariant makeFactory(Algorithm Alg) noexcept {
  switch (Alg) {
  case Algorithm::X25519:
    return FactoryVariant{std::in_place_type<X25519>};
  default:
    assumingUnreachable();
  }
}

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge