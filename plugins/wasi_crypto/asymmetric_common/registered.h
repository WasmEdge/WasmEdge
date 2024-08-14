// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/asymmetric/registered.h - Registered -===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the register asymmetric common algorithm definitions.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "kx/registered.h"
#include "signatures/registered.h"
#include "utils/error.h"

#include <variant>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace AsymmetricCommon {

template <typename... T> struct Registered {
  using PkVariant = std::variant<typename T::PublicKey...>;
  using SkVariant = std::variant<typename T::SecretKey...>;
  using KpVariant = std::variant<typename T::KeyPair...>;
  using Variant = std::variant<T...>;
};

template <typename... Ts1, typename... Ts2>
struct Registered<Signatures::Registered<Ts1...>, Kx::Registered<Ts2...>> {
  using Alg = Registered<Ts1..., Ts2...>;
};

/// Combine the signatures and kx algorithms.
using RegistedAlg = Registered<Signatures::RegistedAlg, Kx::RegistedAlg>::Alg;

using Algorithm = RegistedAlg::Variant;

} // namespace AsymmetricCommon
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
