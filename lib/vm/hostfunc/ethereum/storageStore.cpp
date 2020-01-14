// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/storageStore.h"
#include "support/hexstr.h"

namespace SSVM {
namespace Executor {

ErrCode EEIStorageStore::body(VM::EnvironmentManager &EnvMgr,
                              Instance::MemoryInstance &MemInst,
                              uint32_t PathOffset, uint32_t ValueOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get destination, path, value data, and current storage value.
  evmc::bytes32 Path, Value, CurrValue;
  evmc::address Addr;
  MemInst.getArray(Path.bytes, PathOffset, 32);
  MemInst.getArray(Value.bytes, ValueOffset, 32);
  std::memcpy(Addr.bytes, &Env.getAddress()[0], 20);
  CurrValue = Cxt->host->get_storage(Cxt, &Addr, &Path);

  /// TODO: Charge gas.

  /// Store value into storage.
  Cxt->host->set_storage(Cxt, &Addr, &Path, &Value);
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
