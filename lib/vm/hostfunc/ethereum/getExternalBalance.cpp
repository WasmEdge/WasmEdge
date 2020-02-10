// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getExternalBalance.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetExternalBalance::body(VM::EnvironmentManager &EnvMgr,
                                    Instance::MemoryInstance &MemInst,
                                    uint32_t AddressOffset,
                                    uint32_t ResultOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get address from memory instance.
  evmc_address Addr = loadAddress(MemInst, AddressOffset);

  /// Get balance uint256 big-endian value.
  evmc_uint256be Balance = Cxt->host->get_balance(Cxt, &Addr);

  /// Store uint128 little-endian value.
  return storeUInt(MemInst, Balance, ResultOffset, 16);
}

} // namespace Executor
} // namespace SSVM
