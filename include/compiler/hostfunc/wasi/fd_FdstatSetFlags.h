// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Compiler {

class WasiFdFdstatSetFlags : public Wasi {
public:
  WasiFdFdstatSetFlags(Library &Lib, VM::WasiEnvironment &Env)
      : Wasi(Lib, Env) {}

  void *getFunction() override { return proxy<WasiFdFdstatSetFlags>(); }

  uint32_t run(uint32_t Fd, uint32_t FdFlags);
};

} // namespace Compiler
} // namespace SSVM
