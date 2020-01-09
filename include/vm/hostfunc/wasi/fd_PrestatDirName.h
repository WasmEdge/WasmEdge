// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdPrestatDirName : public Wasi<WasiFdPrestatDirName> {
public:
  WasiFdPrestatDirName(VM::WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &ErrNo, int32_t Fd,
               uint32_t PathBufPtr, uint32_t PathLen);
};

} // namespace Executor
} // namespace SSVM
