// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/kx/factory.h - Key Exchange Factory implement ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the key exchange Factory of wasi-crypto
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/kx/alg.h"
#include "host/wasi_crypto/kx/registed.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

using FactoryVariant = RegistedAlg::Variant;

FactoryVariant makeFactory(Algorithm Alg) noexcept;

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge