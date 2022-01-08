// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/lock.h"
#include "host/wasi_crypto/signature/alg.h"

#include <memory>
#include <utility>
#include <vector>
#include "host/wasi_crypto/signature/alg.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

class Signature {
public:
  Signature(std::vector<uint8_t>&& Data): Inner(Data) {}

  auto &raw() { return Inner; }
private:
//  SignatureAlgorithm Alg;
  Mutex<std::vector<uint8_t>> Inner;
};

class State {
public:
  virtual ~State() = default;

  virtual WasiCryptoExpect<void> update(Span<uint8_t const> Input) = 0;

  virtual WasiCryptoExpect<Signature> sign() = 0;
};

class VerificationState {
public:
  virtual ~VerificationState() = default;

  virtual WasiCryptoExpect<void> update(Span<uint8_t const> Input) = 0;

  virtual WasiCryptoExpect<void> verify(std::shared_ptr<Signature> Sig) = 0;
};

} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
