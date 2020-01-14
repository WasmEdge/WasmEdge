// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getCallValue.h"
#include "support/hexstr.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetCallValue::body(VM::EnvironmentManager &EnvMgr,
                              Instance::MemoryInstance &MemInst,
                              uint32_t ResultOffset) {
  return MemInst.setBytes(Env.getCallValue(), ResultOffset, 0, 16);
}

} // namespace Executor
} // namespace SSVM
