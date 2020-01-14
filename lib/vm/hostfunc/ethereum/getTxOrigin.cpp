// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getTxOrigin.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetTxOrigin::body(VM::EnvironmentManager &EnvMgr,
                             Instance::MemoryInstance &MemInst,
                             uint64_t ResultOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get block coinbase and store bytes20.
  return MemInst.setArray(Cxt->host->get_tx_context(Cxt).tx_origin.bytes,
                          ResultOffset, 20);
}

} // namespace Executor
} // namespace SSVM
