// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/finish.h"

namespace SSVM {
namespace Executor {

ErrCode EEIFinish::body(VM::EnvironmentManager &EnvMgr,
                        Instance::MemoryInstance &MemInst, uint32_t DataOffset,
                        uint32_t DataLength) {
  Env.getReturnData().clear();
  MemInst.getBytes(Env.getReturnData(), DataOffset, DataLength);
  Env.getReturnData().resize(DataLength);
  return ErrCode::Terminated;
}

} // namespace Executor
} // namespace SSVM
