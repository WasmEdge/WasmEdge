// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiEnvironGet : public Wasi<WasiEnvironGet> {
public:
  WasiEnvironGet(VM::WasiEnvironment &HostEnv) : Wasi(HostEnv, "environ_get") {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
               uint32_t EnvPtr, uint32_t EnvBufPtr);
};

} // namespace Executor
} // namespace SSVM
