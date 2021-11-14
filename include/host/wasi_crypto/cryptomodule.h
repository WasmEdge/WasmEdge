// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/ctx.h"
#include "runtime/importobj.h"

namespace WasmEdge {
namespace Host {

class WasiCryptoModule : public Runtime::ImportObject {
public:
  WasiCryptoModule();

  virtual ~WasiCryptoModule() = default;

  WASICrypto::WasiCryptoContext &getContext() { return Ctx; }
private:
  WASICrypto::WasiCryptoContext Ctx;
};

} // namespace Host
} // namespace WasmEdge
