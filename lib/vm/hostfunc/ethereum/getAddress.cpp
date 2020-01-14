// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getAddress.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetAddress::body(VM::EnvironmentManager &EnvMgr,
                            Instance::MemoryInstance &MemInst,
                            uint32_t ResultOffset) {
  return MemInst.setBytes(Env.getAddress(), ResultOffset, 0, 20);
}

} // namespace Executor
} // namespace SSVM
