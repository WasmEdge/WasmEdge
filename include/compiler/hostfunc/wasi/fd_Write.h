// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Compiler {

class WasiFdWrite : public Wasi {
public:
  WasiFdWrite(Library &Lib, VM::WasiEnvironment &Env) : Wasi(Lib, Env) {}

  void *getFunction() override { return proxy<WasiFdWrite>(); }

  uint32_t run(uint32_t Fd, uint32_t IOVecPtr, uint32_t IOVecCnt,
               uint32_t NWrittenPtr);
};

} // namespace Compiler
} // namespace SSVM
