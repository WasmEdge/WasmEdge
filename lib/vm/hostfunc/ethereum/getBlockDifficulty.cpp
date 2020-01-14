// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getBlockDifficulty.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetBlockDifficulty::body(VM::EnvironmentManager &EnvMgr,
                                    Instance::MemoryInstance &MemInst,
                                    uint32_t ResultOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get block difficulty and store uint256 little-endian value.
  return MemInst.setArray(Cxt->host->get_tx_context(Cxt).block_difficulty.bytes,
                          ResultOffset, 32, true);
}

} // namespace Executor
} // namespace SSVM
