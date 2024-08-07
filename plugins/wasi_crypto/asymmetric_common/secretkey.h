// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/asymmetric_common/secretkey.h --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the asymmetric common SecretKey of wasi-crypto.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "asymmetric_common/publickey.h"
#include "asymmetric_common/registered.h"
#include "wasi_crypto/api.hpp"

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace AsymmetricCommon {

using SkVariant = RegistedAlg::SkVariant;

WasiCryptoExpect<SkVariant>
importSk(AsymmetricCommon::Algorithm Alg, Span<const uint8_t> Encoded,
         __wasi_secretkey_encoding_e_t Encoding) noexcept;

WasiCryptoExpect<SecretVec>
skExportData(const SkVariant &SkVariant,
             __wasi_secretkey_encoding_e_t Encoding) noexcept;

WasiCryptoExpect<PkVariant> skPublicKey(const SkVariant &SkVariant) noexcept;

} // namespace AsymmetricCommon
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
