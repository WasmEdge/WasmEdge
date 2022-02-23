// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/signatures/alg.h - Signatures Alg definition ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of signatures algorithms.
///
//===----------------------------------------------------------------------===//
#pragma once

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

enum class Algorithm {
  ECDSA_P256_SHA256,
  ECDSA_K256_SHA256,
  Ed25519,
  RSA_PKCS1_2048_SHA256,
  RSA_PKCS1_2048_SHA384,
  RSA_PKCS1_2048_SHA512,
  RSA_PKCS1_3072_SHA384,
  RSA_PKCS1_3072_SHA512,
  RSA_PKCS1_4096_SHA512,
  RSA_PSS_2048_SHA256,
  RSA_PSS_2048_SHA384,
  RSA_PSS_2048_SHA512,
  RSA_PSS_3072_SHA384,
  RSA_PSS_3072_SHA512,
  RSA_PSS_4096_SHA512
};

} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge