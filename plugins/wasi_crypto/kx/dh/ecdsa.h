// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/kx/dh/ecdsa.h - Ecdsa alg implement --===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of ecdsa algorithm.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "asymmetric_common/ecdsa.h"
#include "kx/options.h"
#include "utils/error.h"
#include "utils/evp_wrapper.h"
#include "utils/optional.h"
#include "utils/secret_vec.h"

#include "common/span.h"

#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

template <int CurveNid> class Ecdsa {

public:
  class PublicKey;
  class SecretKey;
  class KeyPair;
  using Base =
      AsymmetricCommon::Ecdsa<CurveNid, PublicKey, SecretKey, KeyPair, Options>;
  class PublicKey : public Base::PublicKeyBase {
  public:
    using Base::PublicKeyBase::PublicKeyBase;

    const auto &raw() const { return this->Ctx; }
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

using EcdsaP256 = Ecdsa<NID_X9_62_prime256v1>;
using EcdsaP384 = Ecdsa<NID_secp384r1>;

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
