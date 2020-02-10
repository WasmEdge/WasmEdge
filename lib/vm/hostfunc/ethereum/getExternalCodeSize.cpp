// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getExternalCodeSize.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetExternalCodeSize::body(VM::EnvironmentManager &EnvMgr,
                                     Instance::MemoryInstance &MemInst,
                                     uint32_t &Ret, uint32_t AddressOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get address from memory instance.
  evmc_address Addr = loadAddress(MemInst, AddressOffset);

  /// Return: ExtCodeSize(u32)
  Ret = Cxt->host->get_code_size(Cxt, &Addr);
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
