// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdWrite : public Wasi<WasiFdWrite> {
public:
  WasiFdWrite(VM::WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &ErrNo, int32_t Fd,
               uint32_t IOVSPtr, uint32_t IOVSCnt, uint32_t NWrittenPtr);
};

} // namespace Executor
} // namespace SSVM
