// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/kx/dh/ecdsa.h - Ecdsa algorithm implement -------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of ecdsa algorithm
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/asymmetric_common/ecdsa.h"
#include "host/wasi_crypto/kx/options.h"
#include "host/wasi_crypto/utils/error.h"
#include "host/wasi_crypto/utils/evp_wrapper.h"
#include "host/wasi_crypto/utils/optional.h"
#include "host/wasi_crypto/utils/secret_vec.h"

#include <optional>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

class Ecdsa {

public:
  class PublicKey;
  class SecretKey;
  class KeyPair;
  using Base = AsymmetricCommon::Ecdsa<NID_X9_62_prime256v1, PublicKey,
                                       SecretKey, KeyPair, Options>;
  class PublicKey : public Base::PublicKeyBase {
  public:
    using Base::PublicKeyBase::PublicKeyBase;

    const auto &raw() const { return Ctx; }
  };

  class SecretKey : public Base::SecretKeyBase {
  public:
    using Base::SecretKeyBase::SecretKeyBase;

    WasiCryptoExpect<SecretVec> dh(const PublicKey &Pk) const noexcept;
  };

  class KeyPair : public Base::KeyPairBase {
  public:
    using Base::KeyPairBase::KeyPairBase;
  };
};

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
