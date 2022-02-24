// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/wasi_crypto/module.h - Module class definition -----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the wasi-crypto module class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/ctx.h"
#include "runtime/importobj.h"

namespace WasmEdge {
namespace Host {

class WasiCryptoModule : public Runtime::ImportObject {
public:
  WasiCryptoModule();

  WasiCrypto::Context &getContext() { return Ctx; }

private:
  WasiCrypto::Context Ctx;
};

} // namespace Host
} // namespace WasmEdge
