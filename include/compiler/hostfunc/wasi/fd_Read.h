// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Compiler {

class WasiFdRead : public Wasi {
public:
  WasiFdRead(Library &Lib, VM::WasiEnvironment &Env) : Wasi(Lib, Env) {}

  void *getFunction() override { return proxy<WasiFdRead>(); }

  uint32_t run(uint32_t Fd, uint32_t IOVecPtr, uint32_t IOVecCnt,
               uint32_t NReadPtr);
};

} // namespace Compiler
} // namespace SSVM
