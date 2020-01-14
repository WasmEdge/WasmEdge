// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getCodeSize.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetCodeSize::body(VM::EnvironmentManager &EnvMgr,
                             Instance::MemoryInstance &MemInst, uint32_t &Ret) {
  /// Return: CodeSize(u32)
  Ret = Env.getCode().size();
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
