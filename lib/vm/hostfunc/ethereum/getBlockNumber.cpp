// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getBlockNumber.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetBlockNumber::body(VM::EnvironmentManager &EnvMgr,
                                Instance::MemoryInstance &MemInst) {
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
