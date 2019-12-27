// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Compiler {

class WasiArgsSizesGet : public Wasi {
public:
  WasiArgsSizesGet(Library &Lib, VM::WasiEnvironment &Env) : Wasi(Lib, Env) {}

  void *getFunction() override { return proxy<WasiArgsSizesGet>(); }

  uint32_t run(uint32_t ArgcPtr, uint32_t ArgvBufSizePtr);
};

} // namespace Compiler
} // namespace SSVM
