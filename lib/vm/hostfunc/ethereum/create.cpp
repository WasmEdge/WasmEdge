// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/create.h"

namespace SSVM {
namespace Executor {

ErrCode EEICreate::body(VM::EnvironmentManager &EnvMgr,
                        Instance::MemoryInstance &MemInst, uint32_t &Ret,
                        uint32_t ValueOffset, uint32_t DataOffset,
                        uint32_t DataLength, uint32_t ResultOffset) {
  /// Prepare creation message.
  evmc_message CreateMsg = {.kind = evmc_call_kind::EVMC_CREATE,
                            .flags = 0,
                            .depth = static_cast<int32_t>(Env.getDepth() + 1),
                            .gas = static_cast<int64_t>(getMaxCallGas()),
                            .destination = {},
                            .sender = Env.getAddressEVMC(),
                            .input_data = nullptr,
                            .input_size = 0,
                            .value = loadUInt(MemInst, ValueOffset, 16)};

  /// Return: Result(i32)
  return callContract(EnvMgr, MemInst, Ret, CreateMsg, DataOffset, DataLength,
                      ResultOffset);
}

} // namespace Executor
} // namespace SSVM
