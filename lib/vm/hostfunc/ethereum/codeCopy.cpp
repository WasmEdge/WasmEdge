// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/codeCopy.h"

namespace SSVM {
namespace Executor {

ErrCode EEICodeCopy::body(VM::EnvironmentManager &EnvMgr,
                          Instance::MemoryInstance &MemInst,
                          uint32_t ResultOffset, uint32_t CodeOffset,
                          uint32_t Length) {
  return MemInst.setBytes(Env.getCode(), ResultOffset, CodeOffset, Length);
}

} // namespace Executor
} // namespace SSVM
