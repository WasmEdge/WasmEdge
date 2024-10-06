// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/asymmetric_common/keypair.h ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the asymmetric common Keypair of wasi-crypto.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "asymmetric_common/publickey.h"
#include "asymmetric_common/registered.h"
#include "asymmetric_common/secretkey.h"
#include "common/options.h"
#include "utils/error.h"
#include "utils/optional.h"

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
