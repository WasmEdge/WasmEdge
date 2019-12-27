// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Compiler {

class WasiArgsGet : public Wasi {
public:
  WasiArgsGet(Library &Lib, VM::WasiEnvironment &Env) : Wasi(Lib, Env) {}

  void *getFunction() override { return proxy<WasiArgsGet>(); }

  uint32_t run(uint32_t EnvPtr, uint32_t EnvBufPtr);
};

} // namespace Compiler
} // namespace SSVM
