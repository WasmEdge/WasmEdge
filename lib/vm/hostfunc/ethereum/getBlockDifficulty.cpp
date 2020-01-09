// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getBlockDifficulty.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetBlockDifficulty::body(VM::EnvironmentManager &EnvMgr,
                                    Instance::MemoryInstance &MemInst) {
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
