// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/signatures/registed.h - Signatures implement ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains implementation of signatures.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/signatures/registed.h"
#include "host/wasi_crypto/utils/error.h"

#include <variant>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

using SigVariant = RegistedAlg::SigVariant;

WasiCryptoExpect<SigVariant>
sigImport(Algorithm Alg, Span<const uint8_t> Encoded,
          __wasi_signature_encoding_e_t Encoding) noexcept;

WasiCryptoExpect<std::vector<uint8_t>>
sigExportData(const SigVariant &SigVariant,
              __wasi_signature_encoding_e_t Encoding) noexcept;

} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge