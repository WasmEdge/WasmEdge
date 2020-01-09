// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getTxOrigin.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetTxOrigin::body(VM::EnvironmentManager &EnvMgr,
                             Instance::MemoryInstance &MemInst) {
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
