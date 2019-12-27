// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Compiler {

class WasiFdFdstatGet : public Wasi {
public:
  WasiFdFdstatGet(Library &Lib, VM::WasiEnvironment &Env) : Wasi(Lib, Env) {}

  void *getFunction() override { return proxy<WasiFdFdstatGet>(); }

  uint32_t run(uint32_t Fd, uint32_t FdStatPtr);
};

} // namespace Compiler
} // namespace SSVM
