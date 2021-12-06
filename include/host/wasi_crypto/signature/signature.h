// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/lock.h"
#include "host/wasi_crypto/signature/alg.h"
#include "host/wasi_crypto/signature/keypair.h"
#include "host/wasi_crypto/signature/publickey.h"

#include <memory>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class Signature {
public:
  class Base {
  public:
    virtual ~Base() = default;

    virtual std::vector<uint8_t> asRaw() = 0;
  };

  Signature(std::unique_ptr<Base> Inner)
      : Inner(
            std::make_shared<Mutex<std::unique_ptr<Base>>>(std::move(Inner))) {}

  static WasiCryptoExpect<Signature> fromRaw(SignatureAlgorithm Alg,
                                             Span<uint8_t const> Encoded);

  auto &inner() { return Inner; }

private:
  std::shared_ptr<Mutex<std::unique_ptr<Base>>> Inner;
};

class SignatureState {
public:
  class Base {
  public:
    virtual ~Base() = default;

    virtual WasiCryptoExpect<void> update(Span<uint8_t const> Input) = 0;

    virtual WasiCryptoExpect<Signature> sign() = 0;
  };

  SignatureState(std::unique_ptr<Base> Inner)
      : Inner(
            std::make_shared<Mutex<std::unique_ptr<Base>>>(std::move(Inner))) {}

  static WasiCryptoExpect<SignatureState> open(SignatureKeyPair Kp);

  auto &inner() { return Inner; }

private:
  std::shared_ptr<Mutex<std::unique_ptr<Base>>> Inner;
};

class SignatureVerificationState {
public:
  class Base {
  public:
    virtual ~Base() = default;

    virtual WasiCryptoExpect<void> update(Span<uint8_t const> Input) = 0;

    virtual WasiCryptoExpect<void> verify(Signature &Sig) = 0;
  };

  SignatureVerificationState(std::unique_ptr<Base> Inner);

  static WasiCryptoExpect<SignatureVerificationState>
  open(SignaturePublicKey SigPk);

  auto &inner() { return Inner; }

private:
  std::shared_ptr<Mutex<std::unique_ptr<Base>>> Inner;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
