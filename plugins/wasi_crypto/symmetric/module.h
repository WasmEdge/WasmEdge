// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/symmetric/module.h - Module ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the wasi-crypto symmetric module
/// class.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "ctx.h"

#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasiCryptoSymmetricModule : public Runtime::Instance::ModuleInstance {
public:
  WasiCryptoSymmetricModule(std::shared_ptr<WasiCrypto::Context>);

  WasiCrypto::Context &getContext() { return *Ctx.get(); }

private:
  std::shared_ptr<WasiCrypto::Context> Ctx;
};

} // namespace Host
} // namespace WasmEdge
