// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Compiler {

class WasiFdPrestatDirName : public Wasi {
public:
  WasiFdPrestatDirName(Library &Lib, VM::WasiEnvironment &Env)
      : Wasi(Lib, Env) {}

  void *getFunction() override { return proxy<WasiFdPrestatDirName>(); }

  uint32_t run(uint32_t Fd, uint32_t PathBufPtr, uint32_t PathLenPtr);
};

} // namespace Compiler
} // namespace SSVM
