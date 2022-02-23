// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/kx/alg.h - Key Exchange alg definition ----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of key exchange algorithms.
///
//===----------------------------------------------------------------------===//
#pragma once

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

enum class Algorithm { X25519, P256_SHA256 };

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge