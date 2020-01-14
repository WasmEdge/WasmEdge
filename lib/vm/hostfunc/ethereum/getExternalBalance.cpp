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
  evmc_address Addr;
  MemInst.getArray(Addr.bytes, AddressOffset, 20);

  /// Get balance uint256 big-endian value.
  evmc_uint256be Balance = Cxt->host->get_balance(Cxt, &Addr);

  /// Store uint128 little-endian value.
  return MemInst.setArray(Balance.bytes + 16, ResultOffset, 16, true);
}

} // namespace Executor
} // namespace SSVM
