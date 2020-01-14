// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdPrestatGet : public Wasi<WasiFdPrestatGet> {
public:
  WasiFdPrestatGet(VM::WasiEnvironment &HostEnv) : Wasi(HostEnv, "fd_prestat_get") {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &ErrNo, int32_t Fd,
               uint32_t PreStatPtr);
};

} // namespace Executor
} // namespace SSVM
