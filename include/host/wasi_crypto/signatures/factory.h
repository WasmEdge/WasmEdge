// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/signatures/factory.h - Signatures Factory implement ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains implementation of signatures factory relative.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/signatures/registed.h"
#include "host/wasi_crypto/signatures/signatures.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

using FactoryVariant = RegistedAlg::Variant;

FactoryVariant makeFactory(Algorithm Alg) noexcept;

WasiCryptoExpect<SigVariant>
sigImport(Algorithm Alg, Span<const uint8_t> Encoded,
          __wasi_signature_encoding_e_t Encoding) noexcept;

} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge