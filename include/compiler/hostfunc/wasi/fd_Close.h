// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Compiler {

class WasiFdClose : public Wasi {
public:
  WasiFdClose(Library &Lib, VM::WasiEnvironment &Env) : Wasi(Lib, Env) {}

  void *getFunction() override { return proxy<WasiFdClose>(); }

  uint32_t run(uint32_t Fd);
};

} // namespace Compiler
} // namespace SSVM
