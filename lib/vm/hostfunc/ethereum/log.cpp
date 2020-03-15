// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/log.h"

namespace SSVM {
namespace Executor {

ErrCode EEILog::body(VM::EnvironmentManager &EnvMgr,
                     Instance::MemoryInstance &MemInst, uint32_t DataOffset,
                     uint32_t DataLength, uint32_t NumberOfTopics,
                     uint32_t Topic1, uint32_t Topic2, uint32_t Topic3,
                     uint32_t Topic4) {
  /// Check number of topics
  if (NumberOfTopics > 4) {
    return ErrCode::ExecutionFailed;
  }

  /// Take additional gas of logs.
  uint64_t TakeGas = 375ULL * NumberOfTopics + 8ULL * DataLength;
  if (!EnvMgr.addCost(TakeGas)) {
    return ErrCode::CostLimitExceeded;
  }
  evmc_context *Cxt = Env.getEVMCContext();

  /// Copy topics to array.
  std::vector<evmc_bytes32> Topics(4, evmc_bytes32());
  if (NumberOfTopics >= 1) {
    if (ErrCode Status = loadBytes32(MemInst, Topics[0], Topic1);
        Status != ErrCode::Success) {
      return Status;
    }
  }
  if (NumberOfTopics >= 2) {
    if (ErrCode Status = loadBytes32(MemInst, Topics[1], Topic2);
        Status != ErrCode::Success) {
      return Status;
    }
  }
  if (NumberOfTopics >= 3) {
    if (ErrCode Status = loadBytes32(MemInst, Topics[2], Topic3);
        Status != ErrCode::Success) {
      return Status;
    }
  }
  if (NumberOfTopics == 4) {
    if (ErrCode Status = loadBytes32(MemInst, Topics[3], Topic4);
        Status != ErrCode::Success) {
      return Status;
    }
  }

  /// Load data.
  std::vector<uint8_t> Data;
  if (auto Status = MemInst.getBytes(Data, DataOffset, DataLength);
      Status != ErrCode::Success) {
    return Status;
  }

  /// Get address data.
  evmc_address Addr = Env.getAddressEVMC();

  /// Call emit_log.
  Cxt->host->emit_log(Cxt, &Addr, &Data[0], DataLength, &Topics[0],
                      NumberOfTopics);
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
