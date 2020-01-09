// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/call.h"

namespace SSVM {
namespace Executor {

ErrCode EEICall::body(VM::EnvironmentManager &EnvMgr,
                      Instance::MemoryInstance &MemInst) {
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
