// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/asymmetric_common/keypair.h - Asymmetric Keypair ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the asymmetric Keypair of wasi-crypto
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/asymmetric_common/publickey.h"
#include "host/wasi_crypto/asymmetric_common/registed.h"
#include "host/wasi_crypto/asymmetric_common/secretkey.h"
#include "host/wasi_crypto/common/options.h"
#include "host/wasi_crypto/utils/error.h"
#include "host/wasi_crypto/utils/optional.h"
#include "wasi_crypto/api.hpp"

#include <string_view>
#include <variant>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace AsymmetricCommon {

using KpVariant = RegistedAlg::KpVariant;

WasiCryptoExpect<KpVariant>
importKp(AsymmetricCommon::Algorithm Alg, Span<const uint8_t> Encoded,
         __wasi_keypair_encoding_e_t Encoding) noexcept;

WasiCryptoExpect<KpVariant>
generateKp(AsymmetricCommon::Algorithm Alg,
           OptionalRef<const Common::Options> OptOptions) noexcept;

WasiCryptoExpect<SecretVec>
kpExportData(const KpVariant &KpVariant,
             __wasi_keypair_encoding_e_t Encoding) noexcept;

WasiCryptoExpect<PkVariant> kpPublicKey(const KpVariant &KpVariant) noexcept;

WasiCryptoExpect<SkVariant> kpSecretKey(const KpVariant &KpVariant) noexcept;

WasiCryptoExpect<KpVariant> kpFromPkAndSk(const PkVariant &Pk,
                                          const SkVariant &Sk) noexcept;
} // namespace AsymmetricCommon
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge