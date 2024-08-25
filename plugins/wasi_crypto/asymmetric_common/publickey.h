// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/asymmetric_common/publickey.h --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the asymmetric common PubicKey of wasi-crypto.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "asymmetric_common/registered.h"
#include "utils/error.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace AsymmetricCommon {

using PkVariant = RegistedAlg::PkVariant;

WasiCryptoExpect<PkVariant>
importPk(AsymmetricCommon::Algorithm Alg, Span<const uint8_t> Encoded,
         __wasi_publickey_encoding_e_t Encoding) noexcept;

WasiCryptoExpect<std::vector<uint8_t>>
pkExportData(const PkVariant &PkVariant,
             __wasi_publickey_encoding_e_t Encoding) noexcept;

WasiCryptoExpect<void> pkVerify(const PkVariant &PkVariant) noexcept;

} // namespace AsymmetricCommon
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
