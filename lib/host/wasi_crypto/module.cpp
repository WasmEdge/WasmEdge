// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/module.h"

namespace WasmEdge {
namespace Host {

WasiCryptoModule::WasiCryptoModule() : ImportObject("wasi_ephemeral_crypto") {}

} // namespace Host
} // namespace WasmEdge