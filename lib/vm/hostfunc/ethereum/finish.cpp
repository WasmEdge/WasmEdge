// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/finish.h"

namespace SSVM {
namespace Executor {

ErrCode EEIFinish::body(VM::EnvironmentManager &EnvMgr,
                        Instance::MemoryInstance &MemInst, uint32_t DataOffset,
                        uint32_t DataLength) {
  return MemInst.getBytes(Env.getReturnData(), DataOffset, DataLength);
}

} // namespace Executor
} // namespace SSVM
