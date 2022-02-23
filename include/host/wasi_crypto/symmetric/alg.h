// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/symmetric/alg.h - Symmetric Alg definition ------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of symmetric algorithms.
///
//===----------------------------------------------------------------------===//
#pragma once

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

enum class Algorithm {
  HmacSha256,
  HmacSha512,
  HkdfSha256Extract,
  HkdfSha512Extract,
  HkdfSha256Expand,
  HkdfSha512Expand,
  Sha256,
  Sha512,
  Sha512_256,
  Aes128Gcm,
  Aes256Gcm,
  ChaCha20Poly1305
};

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
