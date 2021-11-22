// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/lock.h"
#include "host/wasi_crypto/signature/keypair.h"
#include "host/wasi_crypto/signature/publickey.h"
#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
class Signature;

class SignatureVerificationStateBase {
public:
  virtual ~SignatureVerificationStateBase() = default;
};

class SignatureVerificationState {
public:
  SignatureVerificationState(
      std::unique_ptr<SignatureVerificationStateBase> Inner)
      : Inner(std::make_shared<
              Mutex<std::unique_ptr<SignatureVerificationStateBase>>>(
            std::move(Inner))) {}

  static WasiCryptoExpect<SignatureVerificationState>
  make(SignaturePublicKey SigPk);

  WasiCryptoExpect<void> update(Span<uint8_t const> Input);

  WasiCryptoExpect<void> verify(Signature &Sig);

private:
  std::shared_ptr<Mutex<std::unique_ptr<SignatureVerificationStateBase>>> Inner;
};

class SignatureStateBase {
public:
  virtual ~SignatureStateBase() = default;

  virtual WasiCryptoExpect<void> update(Span<uint8_t> Input) = 0;

  virtual WasiCryptoExpect<void> sign() = 0;
};

class SignatureState {
public:
  SignatureState(std::unique_ptr<SignatureStateBase> Inner)
      : Inner(std::make_shared<Mutex<std::unique_ptr<SignatureStateBase>>>(
            std::move(Inner))) {}

  static WasiCryptoExpect<SignatureState> make(SignatureKeyPair);

  WasiCryptoExpect<void> update(Span<uint8_t const>);

  WasiCryptoExpect<Signature> sign();

private:
  std::shared_ptr<Mutex<std::unique_ptr<SignatureStateBase>>> Inner;
};

class SignatureBase {
public:
  virtual ~SignatureBase() = default;

  virtual Span<uint8_t const> ref() = 0;
};

class Signature {
public:
  Signature(std::unique_ptr<SignatureBase> Inner)
      : Inner(std::make_shared<Mutex<std::unique_ptr<SignatureBase>>>(
            std::move(Inner))) {}

  static WasiCryptoExpect<Signature> fromRaw(SignatureAlgorithm Alg,
                                             Span<uint8_t const> Encoded);

  std::shared_ptr<Mutex<std::unique_ptr<SignatureBase>>> &inner() {
    return Inner;
  }

  std::vector<uint8_t> asRaw() {
    return Inner->locked([](std::unique_ptr<SignatureBase> &Sig) {
      auto Res = Sig->ref();
      return std::vector<uint8_t>{Res.begin(), Res.end()};
    });
  }

private:
  std::shared_ptr<Mutex<std::unique_ptr<SignatureBase>>> Inner;
};
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
