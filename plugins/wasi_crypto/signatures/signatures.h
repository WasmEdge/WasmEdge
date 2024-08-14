// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/signatures/signatures.h - Signatures -===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation of signatures.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "signatures/registered.h"
#include "utils/error.h"

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
