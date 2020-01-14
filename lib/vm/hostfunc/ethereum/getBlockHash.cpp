// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getBlockHash.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetBlockHash::body(VM::EnvironmentManager &EnvMgr,
                              Instance::MemoryInstance &MemInst, uint32_t &Ret,
                              uint64_t Number, uint32_t ResultOffset) {
  evmc_context *Cxt = Env.getEVMCContext();
  ErrCode Status = ErrCode::Success;
  Ret = 0U;

  /// Get the block hash value.
  const evmc::bytes32 Hash = Cxt->host->get_block_hash(Cxt, Number);

  /// Check is zero.
  if (evmc::is_zero(Hash)) {
    Ret = 1U;
  } else {
    /// Store bytes32.
    Status = MemInst.setArray(Hash.bytes, ResultOffset, 32);
    if (Status != ErrCode::Success) {
      Ret = 1U;
    }
  }

  /// Return: Result(u32)
  return Status;
}

} // namespace Executor
} // namespace SSVM
