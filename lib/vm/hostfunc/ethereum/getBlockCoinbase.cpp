// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getBlockCoinbase.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetBlockCoinbase::body(VM::EnvironmentManager &EnvMgr,
                                  Instance::MemoryInstance &MemInst) {
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
