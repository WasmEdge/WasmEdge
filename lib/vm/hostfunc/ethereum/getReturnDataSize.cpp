// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getReturnDataSize.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetReturnDataSize::body(VM::EnvironmentManager &EnvMgr,
                                   Instance::MemoryInstance &MemInst,
                                   uint32_t &DataSize) {
  /// Return: DataSize(u32)
  DataSize = Env.getReturnData().size();
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
