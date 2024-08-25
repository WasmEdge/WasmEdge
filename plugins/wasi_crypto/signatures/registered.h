// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/signatures/registered.h - Registered
//-----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the register signature algorithm definitions.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "signatures/ecdsa.h"
#include "signatures/eddsa.h"
#include "signatures/rsa.h"
#include "utils/error.h"

#include <variant>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

template <typename... T> struct Registered {
  using PkVariant = std::variant<typename T::PublicKey...>;
  using SkVariant = std::variant<typename T::SecretKey...>;
  using KpVariant = std::variant<typename T::KeyPair...>;
  using Variant = std::variant<T...>;
  using SigVariant = std::variant<typename T::Signature...>;
  using SignStateVariant = std::variant<typename T::SignState...>;
  using VerificationStateVariant =
      std::variant<typename T::VerificationState...>;
};

using RegistedAlg =
    Registered<EcdsaK256, EcdsaP256, EcdsaP384, Eddsa, RSA_PKCS1_2048_SHA256,
               RSA_PKCS1_2048_SHA384, RSA_PKCS1_2048_SHA512,
               RSA_PKCS1_3072_SHA384, RSA_PKCS1_3072_SHA512,
               RSA_PKCS1_4096_SHA512, RSA_PSS_2048_SHA256, RSA_PSS_2048_SHA384,
               RSA_PSS_2048_SHA512, RSA_PSS_3072_SHA384, RSA_PSS_3072_SHA512,
               RSA_PSS_4096_SHA512>;

using Algorithm = RegistedAlg::Variant;

} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
