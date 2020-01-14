// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdFdstatGet : public Wasi<WasiFdFdstatGet> {
public:
  WasiFdFdstatGet(VM::WasiEnvironment &HostEnv) : Wasi(HostEnv, "fd_fdstat_get") {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &ErrNo, int32_t Fd,
               uint32_t FdStatPtr);
};

} // namespace Executor
} // namespace SSVM
