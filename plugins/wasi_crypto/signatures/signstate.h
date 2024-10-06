// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/signatures/signstate.h - SignState ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation of sign state.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "signatures/registered.h"
#include "signatures/signatures.h"
#include "utils/error.h"

#include <variant>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

/// Signatures computation.
///
/// More detailed:
/// https://github.com/WebAssembly/wasi-crypto/blob/main/docs/wasi-crypto.md#signature-creation
using SignStateVariant = RegistedAlg::SignStateVariant;
using KpVariant = RegistedAlg::KpVariant;

WasiCryptoExpect<SignStateVariant>
sigStateOpen(const KpVariant &PkVariant) noexcept;

WasiCryptoExpect<void> sigStateUpdate(SignStateVariant &SignStateVariant,
                                      Span<const uint8_t> Input) noexcept;

WasiCryptoExpect<SigVariant>
sigStateSign(SignStateVariant &SignStateVariant) noexcept;

} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
