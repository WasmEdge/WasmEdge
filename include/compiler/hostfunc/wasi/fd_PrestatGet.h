// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Compiler {

class WasiFdPrestatGet : public Wasi {
public:
  WasiFdPrestatGet(Library &Lib, VM::WasiEnvironment &Env) : Wasi(Lib, Env) {}

  void *getFunction() override { return proxy<WasiFdPrestatGet>(); }

  uint32_t run(uint32_t Fd, uint32_t PreStatPtr);
};

} // namespace Compiler
} // namespace SSVM
