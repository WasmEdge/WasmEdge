// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/asymmetric_common/factory.h - Asymmetric Factory ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the asymmetric Factory of wasi-crypto
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/asymmetric_common/keypair.h"
#include "host/wasi_crypto/asymmetric_common/publickey.h"
#include "host/wasi_crypto/asymmetric_common/registed.h"
#include "host/wasi_crypto/asymmetric_common/secretkey.h"
#include "host/wasi_crypto/utils/error.h"

#include <cstdint>
#include <variant>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace AsymmetricCommon {

using FactoryVariant = RegistedAlg::Variant;

WasiCryptoExpect<FactoryVariant> makeFactory(__wasi_algorithm_type_e_t AlgType,
                                             std::string_view AlgStr) noexcept;

WasiCryptoExpect<PkVariant>
importPk(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
         Span<const uint8_t> Encoded,
         __wasi_publickey_encoding_e_t Encoding) noexcept;

WasiCryptoExpect<SkVariant>
importSk(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
         Span<const uint8_t> Encoded,
         __wasi_secretkey_encoding_e_t Encoding) noexcept;

WasiCryptoExpect<KpVariant>
importKp(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
         Span<const uint8_t> Encoded,
         __wasi_keypair_encoding_e_t Encoding) noexcept;

WasiCryptoExpect<KpVariant>
generateKp(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
           OptionalRef<Common::Options> OptOptions) noexcept;

WasiCryptoExpect<KpVariant> kpFromPkAndSk(PkVariant &Pk,
                                          SkVariant &Sk) noexcept;

} // namespace AsymmetricCommon
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge