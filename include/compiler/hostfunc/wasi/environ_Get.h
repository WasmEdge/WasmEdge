// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Compiler {

class WasiEnvironGet : public Wasi {
public:
  WasiEnvironGet(Library &Lib, VM::WasiEnvironment &Env) : Wasi(Lib, Env) {}

  void *getFunction() override { return proxy<WasiEnvironGet>(); }

  uint32_t run(uint32_t ArgvPtr, uint32_t ArgvBufPtr);
};

} // namespace Compiler
} // namespace SSVM
