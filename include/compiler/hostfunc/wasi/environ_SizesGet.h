// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Compiler {

class WasiEnvironSizesGet : public Wasi {
public:
  WasiEnvironSizesGet(Library &Lib, VM::WasiEnvironment &Env)
      : Wasi(Lib, Env) {}

  void *getFunction() override { return proxy<WasiEnvironSizesGet>(); }

  uint32_t run(uint32_t EnvCntPtr, uint32_t EnvBufSizePtr);
};

} // namespace Compiler
} // namespace SSVM
