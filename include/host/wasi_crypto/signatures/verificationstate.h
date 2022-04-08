// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/signatures/verificationstate.h - VerificationState ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains implementation of verification state relative.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/signatures/registed.h"
#include "host/wasi_crypto/signatures/signatures.h"
#include "host/wasi_crypto/utils/error.h"

#include <variant>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

/// Signatures verify
///
/// More detailed:
/// https://github.com/WebAssembly/wasi-crypto/blob/main/docs/wasi-crypto.md#signature-verification
using VerificationStateVariant = RegistedAlg::VerificationStateVariant;
using PkVariant = RegistedAlg::PkVariant;

WasiCryptoExpect<VerificationStateVariant>
verificationStateOpen(const PkVariant &PkVariant) noexcept;

WasiCryptoExpect<void>
verificationStateUpdate(VerificationStateVariant &VerificationStateVariant,
                        Span<const uint8_t> Input) noexcept;

WasiCryptoExpect<void>
verificationStateVerify(VerificationStateVariant &VerificationStateVariant,
                        const SigVariant &SigVariant) noexcept;

} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge