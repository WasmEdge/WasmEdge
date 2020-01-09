// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getExternalCodeSize.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetExternalCodeSize::body(VM::EnvironmentManager &EnvMgr,
                                     Instance::MemoryInstance &MemInst) {
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
