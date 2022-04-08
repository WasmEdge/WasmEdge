// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/symmetric/registed.h - Symmetric Registed implement ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains register symmetric algorithm
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/symmetric/aeads.h"
#include "host/wasi_crypto/symmetric/hash.h"
#include "host/wasi_crypto/symmetric/kdf.h"
#include "host/wasi_crypto/symmetric/mac.h"
#include "host/wasi_crypto/utils/error.h"

#include <variant>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

/// Registed algorithm
template <typename... T> struct Registed {
  using Key = std::variant<typename T::Key...>;
  using State = std::variant<typename T::State...>;
  using Variant = std::variant<T...>;
};

using RegistedAlg =
    Registed<Sha256, Sha512, Sha512_256, HmacSha256, HmacSha512,
             HkdfSha256Expand, HkdfSha256Extract, HkdfSha512Expand,
             HkdfSha512Extract, Aes128Gcm, Aes256Gcm, ChaCha20Poly1305>;

using Algorithm = RegistedAlg::Variant;

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge