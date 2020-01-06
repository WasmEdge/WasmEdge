// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiArgsSizesGet : public Wasi {
public:
  WasiArgsSizesGet(VM::WasiEnvironment &Env);

  ErrCode run(VM::EnvironmentManager &EnvMgr, StackManager &StackMgr,
              Instance::MemoryInstance &MemInst) override;

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
               uint32_t ArgcPtr, uint32_t ArgvBufSizePtr);
};

} // namespace Executor
} // namespace SSVM
