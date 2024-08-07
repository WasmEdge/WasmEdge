// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/signatures/verificationstate.h -------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the verification state related functions.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "signatures/registered.h"
#include "signatures/signatures.h"
#include "utils/error.h"

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
