// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/signatures/ecdsa.h - Ecdsa alg -------===//
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
#include "signatures/options.h"
#include "utils/error.h"
#include "utils/evp_wrapper.h"
#include "utils/optional.h"
#include "utils/secret_vec.h"

#include "common/span.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

template <int CurveNid> class Ecdsa {
public:
  class PublicKey;
  class KeyPair;
  class SecretKey;
  using Base =
      AsymmetricCommon::Ecdsa<CurveNid, PublicKey, SecretKey, KeyPair, Options>;
  class Signature {
  public:
    Signature(std::vector<uint8_t> Data) noexcept : Data(std::move(Data)) {}

    static WasiCryptoExpect<Signature>
    import(Span<const uint8_t> Encoded,
           __wasi_signature_encoding_e_t Encoding) noexcept;

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_signature_encoding_e_t Encoding) const noexcept;

    const std::vector<uint8_t> &ref() const { return Data; }

  private:
    // Inner represent as der because OpenSSL use der format for evp interface.
    std::vector<uint8_t> Data;
  };

  class SignState {
  public:
    SignState(EvpMdCtxPtr Ctx) noexcept
        : Ctx(std::make_shared<Inner>(std::move(Ctx))) {}

    WasiCryptoExpect<void> update(Span<const uint8_t> Input) noexcept;

    WasiCryptoExpect<Signature> sign() noexcept;

  private:
    struct Inner {
      Inner(EvpMdCtxPtr RawCtx) noexcept : RawCtx(std::move(RawCtx)) {}
      std::mutex Mutex;
      EvpMdCtxPtr RawCtx;
    };
    std::shared_ptr<Inner> Ctx;
  };

  class VerificationState {
  public:
    VerificationState(EvpMdCtxPtr Ctx) noexcept
        : Ctx(std::make_shared<Inner>(std::move(Ctx))) {}

    WasiCryptoExpect<void> update(Span<const uint8_t> Input) noexcept;

    WasiCryptoExpect<void> verify(const Signature &Sig) noexcept;

  private:
    struct Inner {
      Inner(EvpMdCtxPtr RawCtx) noexcept : RawCtx(std::move(RawCtx)) {}
      std::mutex Mutex;
      EvpMdCtxPtr RawCtx;
    };
    std::shared_ptr<Inner> Ctx;
  };

  class PublicKey : public Base::PublicKeyBase {
  public:
    using Base::PublicKeyBase::PublicKeyBase;

    WasiCryptoExpect<VerificationState> openVerificationState() const noexcept;
  };

  class SecretKey : public Base::SecretKeyBase {
  public:
    using Base::SecretKeyBase::SecretKeyBase;
  };

  class KeyPair : public Base::KeyPairBase {
  public:
    using Base::KeyPairBase::KeyPairBase;

    WasiCryptoExpect<SignState> openSignState() const noexcept;
  };
};

using EcdsaP256 = Ecdsa<NID_X9_62_prime256v1>;
using EcdsaK256 = Ecdsa<NID_secp256k1>;
using EcdsaP384 = Ecdsa<NID_secp384r1>;

} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
