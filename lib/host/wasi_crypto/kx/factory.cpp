// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/kx/factory.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

FactoryVariant makeFactory(Algorithm Alg) noexcept {
  switch (Alg) {
    default:
    assumingUnreachable();
  }
}

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge