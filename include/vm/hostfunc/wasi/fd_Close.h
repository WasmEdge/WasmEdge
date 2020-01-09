// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdClose : public Wasi<WasiFdClose> {
public:
  WasiFdClose(VM::WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &ErrNo, int32_t Fd);
};

} // namespace Executor
} // namespace SSVM
