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
  evmc::bytes32 Path, Value;
  evmc::address Addr;
  MemInst.getArray(Path.bytes, PathOffset, 32);
  std::memcpy(Addr.bytes, &Env.getAddress()[0], 20);
  Value = Cxt->host->get_storage(Cxt, &Addr, &Path);

  /// Store bytes32 into memory instance.
  return MemInst.setArray(Value.bytes, ValueOffset, 32);
}

} // namespace Executor
} // namespace SSVM
