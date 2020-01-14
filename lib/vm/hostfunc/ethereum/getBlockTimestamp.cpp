// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getBlockTimestamp.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetBlockTimestamp::body(VM::EnvironmentManager &EnvMgr,
                                   Instance::MemoryInstance &MemInst,
                                   uint64_t &BlockTimestamp) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Return: BlockNumber(u64)
  BlockTimestamp =
      static_cast<uint64_t>(Cxt->host->get_tx_context(Cxt).block_timestamp);
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
