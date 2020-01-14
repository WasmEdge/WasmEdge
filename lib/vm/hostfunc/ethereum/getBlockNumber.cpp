// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getBlockNumber.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetBlockNumber::body(VM::EnvironmentManager &EnvMgr,
                                Instance::MemoryInstance &MemInst,
                                uint64_t &BlockNumber) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Return: BlockNumber(u64)
  BlockNumber =
      static_cast<uint64_t>(Cxt->host->get_tx_context(Cxt).block_number);
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
