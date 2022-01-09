// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/signature/alg.h"
#include "host/wasi_crypto/signature/signature.h"

#include <utility>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

class SignState {
public:
  virtual ~SignState() = default;

  virtual WasiCryptoExpect<void> update(Span<uint8_t const> Input) = 0;

  virtual WasiCryptoExpect<std::unique_ptr<Signature>> sign() = 0;
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
