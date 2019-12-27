// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Compiler {

class WasiPathOpen : public Wasi {
public:
  WasiPathOpen(Library &Lib, VM::WasiEnvironment &Env) : Wasi(Lib, Env) {}

  void *getFunction() override { return proxy<WasiPathOpen>(); }

  uint32_t run(uint32_t DirFd, uint32_t DirFlags, uint32_t PathPtr,
               uint32_t PathLen, uint32_t OFlags, uint32_t FsRightsBase,
               uint32_t FsRightsInheriting, uint32_t FsFlags, uint32_t FdPtr);
};

} // namespace Compiler
} // namespace SSVM
