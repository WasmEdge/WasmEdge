// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getGasLeft.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetGasLeft::body(VM::EnvironmentManager &EnvMgr,
                            Instance::MemoryInstance &MemInst,
                            uint64_t &GasLeft) {
  GasLeft = Env.getGasLeft();
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
