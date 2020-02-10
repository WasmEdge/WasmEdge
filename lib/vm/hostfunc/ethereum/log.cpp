// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/log.h"

namespace SSVM {
namespace Executor {

ErrCode EEILog::body(VM::EnvironmentManager &EnvMgr,
                     Instance::MemoryInstance &MemInst, uint32_t DataOffset,
                     uint32_t DataLength, uint32_t NumberOfTopics,
                     uint32_t Topic1, uint32_t Topic2, uint32_t Topic3,
                     uint32_t Topic4) {
  /// Take additional gas of logs.
  uint64_t TakeGas = 375ULL * NumberOfTopics + 8ULL * DataLength;
  if (!EnvMgr.addCost(TakeGas)) {
    return ErrCode::Revert;
  }
  evmc_context *Cxt = Env.getEVMCContext();

  /// Copy topics to array.
  std::vector<evmc_bytes32> Topics(4, evmc_bytes32());
  if (NumberOfTopics >= 1) {
    Topics[0] = loadBytes32(MemInst, Topic1);
  }
  if (NumberOfTopics >= 2) {
    Topics[1] = loadBytes32(MemInst, Topic2);
  }
  if (NumberOfTopics >= 3) {
    Topics[2] = loadBytes32(MemInst, Topic3);
  }
  if (NumberOfTopics == 4) {
    Topics[3] = loadBytes32(MemInst, Topic4);
  }

  /// Load data.
  std::vector<uint8_t> Data;
  MemInst.getBytes(Data, DataOffset, DataLength);

  /// Get address data.
  evmc_address Addr = Env.getAddressEVMC();

  /// Call emit_log.
  Cxt->host->emit_log(Cxt, &Addr, &Data[0], DataLength, &Topics[0],
                      NumberOfTopics);
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
