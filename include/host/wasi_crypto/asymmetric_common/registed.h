// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/asymmetric/registed.h - Asymmetric Registed implement -===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains register asymmetric common algorithm.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/kx/registed.h"
#include "host/wasi_crypto/signatures/registed.h"
#include "host/wasi_crypto/utils/error.h"

#include <variant>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace AsymmetricCommon {

template <typename... T> struct Registed {
  using PkVariant = std::variant<typename T::PublicKey...>;
  using SkVariant = std::variant<typename T::SecretKey...>;
  using KpVariant = std::variant<typename T::KeyPair...>;
  using Variant = std::variant<T...>;
};

template <typename... Ts1, typename... Ts2>
struct Registed<Signatures::Registed<Ts1...>, Kx::Registed<Ts2...>> {
  using Alg = Registed<Ts1..., Ts2...>;
};

/// combine signatures and kx alg
using RegistedAlg = Registed<Signatures::RegistedAlg, Kx::RegistedAlg>::Alg;

using Algorithm = RegistedAlg::Variant;

} // namespace AsymmetricCommon
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge