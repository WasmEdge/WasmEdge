// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/asymmetric_common/publickey.h - Asymmetric PubicKey ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the asymmetric common PubicKey of wasi-crypto
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/asymmetric_common/registed.h"
#include "host/wasi_crypto/utils/error.h"
#include "wasi_crypto/api.hpp"

#include <cstdint>
#include <variant>
#include <vector>

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