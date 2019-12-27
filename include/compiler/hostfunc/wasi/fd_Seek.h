// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Compiler {

class WasiFdSeek : public Wasi {
public:
  WasiFdSeek(Library &Lib, VM::WasiEnvironment &Env) : Wasi(Lib, Env) {}

  void *getFunction() override { return proxy<WasiFdSeek>(); }

  uint32_t run(uint32_t Fd, uint32_t Offset, uint32_t Whence,
               uint32_t NewOffsetPtr);
};

} // namespace Compiler
} // namespace SSVM
