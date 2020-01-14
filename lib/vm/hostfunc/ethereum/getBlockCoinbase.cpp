// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getBlockCoinbase.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetBlockCoinbase::body(VM::EnvironmentManager &EnvMgr,
                                  Instance::MemoryInstance &MemInst,
                                  uint32_t ResultOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get block coinbase and store bytes20.
  return MemInst.setArray(Cxt->host->get_tx_context(Cxt).block_coinbase.bytes,
                          ResultOffset, 20);
}

} // namespace Executor
} // namespace SSVM
