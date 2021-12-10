// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/lock.h"
#include "host/wasi_crypto/signature/alg.h"

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

    virtual Span<uint8_t const> asRef() = 0;

    virtual std::vector<uint8_t> asRaw() {
      auto R = asRef();
      return std::vector<uint8_t>{R.begin(), R.end()};
    }
  };

  Signature(std::unique_ptr<Base> Inner)
      : Inner(
            std::make_shared<Mutex<std::unique_ptr<Base>>>(std::move(Inner))) {}

  static WasiCryptoExpect<Signature>
  import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
         __wasi_signature_encoding_e_t Encoding);

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

    virtual WasiCryptoExpect<void> verify(std::unique_ptr<Signature::Base> &Sig) = 0;
  };

  SignatureVerificationState(std::unique_ptr<Base> Inner)
      : Inner(
            std::make_shared<Mutex<std::unique_ptr<Base>>>(std::move(Inner))) {}

  auto &inner() { return Inner; }

private:
  std::shared_ptr<Mutex<std::unique_ptr<Base>>> Inner;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
