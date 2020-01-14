// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiArgsGet : public Wasi<WasiArgsGet> {
public:
  WasiArgsGet(VM::WasiEnvironment &HostEnv) : Wasi(HostEnv, "args_get") {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
               uint32_t ArgvPtr, uint32_t ArgvBufPtr);
};

} // namespace Executor
} // namespace SSVM
