// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getBlockGasLimit.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetBlockGasLimit::body(VM::EnvironmentManager &EnvMgr,
                                  Instance::MemoryInstance &MemInst,
                                  uint64_t &GasLimit) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Return: GasLimit(u64)
  GasLimit =
      static_cast<uint64_t>(Cxt->host->get_tx_context(Cxt).block_gas_limit);
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
