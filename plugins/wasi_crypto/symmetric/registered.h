// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/symmetric/registered.h - Registered --===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the register symmetric algorithm definitions.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "symmetric/aeads.h"
#include "symmetric/hash.h"
#include "symmetric/kdf.h"
#include "symmetric/mac.h"
#include "utils/error.h"

#include <variant>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

/// Registered algorithm
template <typename... T> struct Registered {
  using Key = std::variant<typename T::Key...>;
  using State = std::variant<typename T::State...>;
  using Variant = std::variant<T...>;
};

using RegistedAlg =
    Registered<Sha256, Sha512, Sha512_256, HmacSha256, HmacSha512,
               HkdfSha256Expand, HkdfSha256Extract, HkdfSha512Expand,
               HkdfSha512Extract, Aes128Gcm, Aes256Gcm, ChaCha20Poly1305>;

using Algorithm = RegistedAlg::Variant;

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
