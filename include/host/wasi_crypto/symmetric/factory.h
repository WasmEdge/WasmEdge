// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/wasi_crypto/symmetric/factory.h - Symmetric factory ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of symmetric factory, it produce factory,
/// key and state
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/symmetric/alg.h"
#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/options.h"
#include "host/wasi_crypto/symmetric/registed.h"
#include "host/wasi_crypto/symmetric/state.h"
#include "host/wasi_crypto/utils/error.h"
#include "host/wasi_crypto/utils/optional.h"

#include <variant>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

using FactoryVariant = RegistedAlg::Variant;

FactoryVariant makeFactory(Algorithm Alg) noexcept;

WasiCryptoExpect<KeyVariant> importKey(Algorithm Alg,
                                       Span<const uint8_t> Data) noexcept;

WasiCryptoExpect<KeyVariant>
generateKey(Algorithm Alg, OptionalRef<Options> OptOptions) noexcept;

WasiCryptoExpect<StateVariant>
openState(Algorithm Alg, OptionalRef<KeyVariant> OptKeyVariant,
          OptionalRef<Options> OptOptions) noexcept;

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
