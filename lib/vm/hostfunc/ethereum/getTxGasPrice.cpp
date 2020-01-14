// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getTxGasPrice.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetTxGasPrice::body(VM::EnvironmentManager &EnvMgr,
                               Instance::MemoryInstance &MemInst,
                               uint64_t ResultOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get tx gas price uint256 big-endian value.
  evmc_uint256be Price = Cxt->host->get_tx_context(Cxt).tx_gas_price;

  /// Store uint128 little-endian value.
  return MemInst.setArray(Price.bytes + 16, ResultOffset, 16, true);
}

} // namespace Executor
} // namespace SSVM
