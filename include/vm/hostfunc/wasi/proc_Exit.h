// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiProcExit : public Wasi<WasiProcExit> {
public:
  WasiProcExit(VM::WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, int32_t Status);
};

} // namespace Executor
} // namespace SSVM
