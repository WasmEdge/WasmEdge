// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdSeek : public Wasi {
public:
  WasiFdSeek(VM::WasiEnvironment &Env);

  ErrCode run(VM::EnvironmentManager &EnvMgr, StackManager &StackMgr,
              Instance::MemoryInstance &MemInst) override;

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &ErrNo, int32_t Fd,
               int32_t Offset, uint32_t Whence, uint32_t NewOffsetPtr);
};

} // namespace Executor
} // namespace SSVM
