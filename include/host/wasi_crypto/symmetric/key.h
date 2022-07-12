// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/symmetric/key.h - Symmetric Key class -----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains Symmetric Key definition.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/symmetric/registed.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

/// Object represents a key and an algorithm
///
/// More detail:
/// https://github.com/WebAssembly/wasi-crypto/blob/main/docs/wasi-crypto.md#symmetric-keys-1
using KeyVariant = RegistedAlg::Key;

WasiCryptoExpect<KeyVariant> importKey(Algorithm Alg,
                                       Span<const uint8_t> Data) noexcept;

WasiCryptoExpect<KeyVariant>
generateKey(Algorithm Alg, OptionalRef<const Options> OptOptions) noexcept;

/// get inner represent
SecretVec keyExportData(const KeyVariant &Key) noexcept;

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge