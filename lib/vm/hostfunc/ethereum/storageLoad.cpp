// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/storageLoad.h"
#include "support/hexstr.h"

namespace SSVM {
namespace Executor {

ErrCode EEIStorageLoad::body(VM::EnvironmentManager &EnvMgr,
                             Instance::MemoryInstance &MemInst,
                             uint32_t PathOffset, uint32_t ValueOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get destination, path, and value data.
  evmc_address Addr = Env.getAddressEVMC();
  evmc_bytes32 Path = loadBytes32(MemInst, PathOffset);

  /// Store bytes32 into memory instance.
  return storeBytes32(MemInst, Cxt->host->get_storage(Cxt, &Addr, &Path),
                      ValueOffset);
}

} // namespace Executor
} // namespace SSVM
