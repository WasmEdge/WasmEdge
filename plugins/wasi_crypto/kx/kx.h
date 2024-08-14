// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/kx/kx.h - Key Exchange related -------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the key exchange related functions.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "kx/registered.h"
#include "utils/error.h"

#include <cstdint>
#include <type_traits>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

using PkVariant = RegistedAlg::PkVariant;
using SkVariant = RegistedAlg::SkVariant;

/// Diffie-Hellman based key agreement.
///
/// More detailed:
/// https://github.com/WebAssembly/wasi-crypto/blob/main/docs/wasi-crypto.md#diffie-hellman-based-key-agreement
WasiCryptoExpect<SecretVec> dh(const PkVariant &PkVariant,
                               const SkVariant &SkVariant) noexcept;

/// Key encapsulation mechanisms.
///
/// More detailed
/// https://github.com/WebAssembly/wasi-crypto/blob/main/docs/wasi-crypto.md#key-encapsulation-mechanisms
struct EncapsulatedSecret {
  std::vector<uint8_t> EncapsulatedSecretData;
  std::vector<uint8_t> Secret;
};

WasiCryptoExpect<EncapsulatedSecret> encapsulate(PkVariant &PkVariant) noexcept;

WasiCryptoExpect<std::vector<uint8_t>>
decapsulate(SkVariant &SkVariant,
            Span<const uint8_t> EncapsulatedSecret) noexcept;

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
