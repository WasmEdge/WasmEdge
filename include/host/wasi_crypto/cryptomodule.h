// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/common/ctx.h"
#include "host/wasi_crypto/symmetric/ctx.h"
#include "host/wasi_crypto/asymmetric_common/ctx.h"
#include "runtime/importobj.h"

namespace WasmEdge {
namespace Host {

class WasiCryptoModule : public Runtime::ImportObject {
public:
  WasiCryptoModule();

  virtual ~WasiCryptoModule() = default;

  WASICrypto::CommonContext& getContext() { return CommonCtx; }
private:
  WASICrypto::CommonContext CommonCtx;
  WASICrypto::SymmetricContext SymmetricCtx{CommonCtx};
  WASICrypto::AsymmetricCommonContext AsymmetricCtx{CommonCtx};
};



} // namespace Host
} // namespace WasmEdge
