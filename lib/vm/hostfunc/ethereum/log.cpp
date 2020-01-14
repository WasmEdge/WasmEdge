// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/log.h"

namespace SSVM {
namespace Executor {

ErrCode EEILog::body(VM::EnvironmentManager &EnvMgr,
                     Instance::MemoryInstance &MemInst, uint32_t DataOffset,
                     uint32_t DataLength, uint32_t NumberOfTopics,
                     uint32_t Topic1, uint32_t Topic2, uint32_t Topic3,
                     uint32_t Topic4) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Copy topics to array.
  std::vector<evmc::bytes32> Topics(4, evmc::bytes32());
  if (NumberOfTopics >= 1) {
    MemInst.getArray(Topics[0].bytes, Topic1, 32);
  }
  if (NumberOfTopics >= 2) {
    MemInst.getArray(Topics[1].bytes, Topic2, 32);
  }
  if (NumberOfTopics >= 3) {
    MemInst.getArray(Topics[2].bytes, Topic3, 32);
  }
  if (NumberOfTopics == 4) {
    MemInst.getArray(Topics[3].bytes, Topic4, 32);
  }

  /// Load data.
  std::vector<uint8_t> Data;
  MemInst.getBytes(Data, DataOffset, DataLength);

  /// Get address data.
  evmc::address Addr;
  std::memcpy(Addr.bytes, &Env.getAddress()[0], 20);

  /// Call emit_log.
  Cxt->host->emit_log(Cxt, &Addr, &Data[0], DataLength, &Topics[0],
                      NumberOfTopics);
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
