// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Compiler {

class WasiProcExit : public Wasi {
public:
  WasiProcExit(Library &Lib, VM::WasiEnvironment &Env) : Wasi(Lib, Env) {}

  void *getFunction() override { return proxy<WasiProcExit>(); }

  void run(uint32_t ExitCode);
};

} // namespace Compiler
} // namespace SSVM
