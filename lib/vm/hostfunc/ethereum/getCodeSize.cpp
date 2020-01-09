// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getCodeSize.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetCodeSize::body(VM::EnvironmentManager &EnvMgr,
                             Instance::MemoryInstance &MemInst) {
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
