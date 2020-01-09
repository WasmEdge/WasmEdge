// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getGasLeft.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetGasLeft::body(VM::EnvironmentManager &EnvMgr,
                            Instance::MemoryInstance &MemInst) {
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
