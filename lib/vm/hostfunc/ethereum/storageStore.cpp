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
  evmc_address Addr = Env.getAddressEVMC();
  evmc_bytes32 Path = loadBytes32(MemInst, PathOffset);
  evmc_bytes32 Value = loadBytes32(MemInst, ValueOffset);
  evmc_bytes32 CurrValue = Cxt->host->get_storage(Cxt, &Addr, &Path);

  /// Take additional gas if create case.
  if (evmc::is_zero(CurrValue) && !evmc::is_zero(Value)) {
    if (!EnvMgr.addCost(15000ULL)) {
      return ErrCode::Revert;
    }
  }

  /// Store value into storage.
  Cxt->host->set_storage(Cxt, &Addr, &Path, &Value);
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
