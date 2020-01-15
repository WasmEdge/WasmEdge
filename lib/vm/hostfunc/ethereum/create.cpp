// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/create.h"

namespace SSVM {
namespace Executor {

ErrCode EEICreate::body(VM::EnvironmentManager &EnvMgr,
                        Instance::MemoryInstance &MemInst, uint32_t &Ret,
                        uint32_t ValueOffset, uint32_t DataOffset,
                        uint32_t DataLength, uint32_t ResultOffset) {
  /// TODO: Implement this function.
  Ret = 2;
  return ErrCode::Revert;
}

} // namespace Executor
} // namespace SSVM
