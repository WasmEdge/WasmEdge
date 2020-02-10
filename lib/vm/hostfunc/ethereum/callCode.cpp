// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/callCode.h"

namespace SSVM {
namespace Executor {

ErrCode EEICallCode::body(VM::EnvironmentManager &EnvMgr,
                          Instance::MemoryInstance &MemInst, uint32_t &Ret,
                          uint64_t Gas, uint32_t AddressOffset,
                          uint32_t ValueOffset, uint32_t DataOffset,
                          uint32_t DataLength) {
  evmc_context *Cxt = Env.getEVMCContext();
  ErrCode Status = ErrCode::Success;

  /// Prepare call message.
  evmc_message CallMsg;
  CallMsg.destination = loadAddress(MemInst, AddressOffset);
  CallMsg.flags = Env.getFlag() & evmc_flags::EVMC_STATIC;
  CallMsg.depth = Env.getDepth() + 1;
  CallMsg.kind = evmc_call_kind::EVMC_CALLCODE;
  CallMsg.sender = Env.getAddressEVMC();
  CallMsg.value = loadUInt(MemInst, ValueOffset, 16);

  /// Prepare input data.
  std::vector<uint8_t> Code;
  CallMsg.input_data = nullptr;
  if (DataLength > 0) {
    MemInst.getBytes(Code, DataOffset, DataLength);
    CallMsg.input_data = &Code[0];
    CallMsg.input_size = Code.size();
  }

  /// Check depth.
  if (Env.getDepth() >= 1024) {
    Ret = 1;
    return ErrCode::Revert;
  }

  /// Assign gas to callee.
  CallMsg.gas = Env.getGasLeft() - Env.getGasLeft() / 64;
  EnvMgr.addCost(CallMsg.gas);

  /// Add gas stipend for value transfers
  if (!evmc::is_zero(CallMsg.value)) {
    CallMsg.gas += 2300U;
  }

  /// Call.
  evmc_result CallRes = Cxt->host->call(Cxt, &CallMsg);

  /// Assign output data.
  if (CallRes.output_data) {
    Env.getReturnData().assign(CallRes.output_data,
                               CallRes.output_data + CallRes.output_size);
  } else {
    Env.getReturnData().clear();
  }

  /// Return left gas.
  if (CallRes.gas_left < 0) {
    return ErrCode::Revert;
  }
  EnvMgr.subCost(CallRes.gas_left);

  /// Return: Result(i32)
  switch (CallRes.status_code) {
  case EVMC_SUCCESS:
    Ret = 0;
    break;
  case EVMC_REVERT:
    Ret = 2;
    Status = ErrCode::Revert;
    break;
  default:
    Ret = 1;
    Status = ErrCode::Revert;
  }
  return Status;
}

} // namespace Executor
} // namespace SSVM
