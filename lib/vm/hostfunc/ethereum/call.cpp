// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/call.h"
#include <boost/multiprecision/cpp_int.hpp>

namespace SSVM {
namespace Executor {

ErrCode EEICall::body(VM::EnvironmentManager &EnvMgr,
                      Instance::MemoryInstance &MemInst, uint32_t &Ret,
                      uint64_t Gas, uint32_t AddressOffset,
                      uint32_t ValueOffset, uint32_t DataOffset,
                      uint32_t DataLength) {
  evmc_context *Cxt = Env.getEVMCContext();
  ErrCode Status = ErrCode::Success;

  /// Prepare call message.
  evmc_message CallMsg;
  MemInst.getArray(CallMsg.destination.bytes, AddressOffset, 20);
  CallMsg.flags = Env.getFlag() & evmc_flags::EVMC_STATIC;
  CallMsg.depth = Env.getDepth() + 1;
  CallMsg.kind = evmc_call_kind::EVMC_CALL;
  std::memcpy(CallMsg.sender.bytes, &Env.getAddress()[0], 20);
  MemInst.getArray(CallMsg.value.bytes + 16, ValueOffset, 16, true);

  /// Check flag.
  if (!evmc::is_zero(CallMsg.value)) {
    if (Env.getFlag() & evmc_flags::EVMC_STATIC) {
      return ErrCode::ExecutionFailed;
    }
  }

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

  if (!evmc::is_zero(CallMsg.value)) {
    /// Take transfer gas.
    if (!EnvMgr.addCost(9000ULL)) {
      return ErrCode::Revert;
    }
    /// Check balance.
    evmc_uint256be Balance = Cxt->host->get_balance(Cxt, &(CallMsg.sender));
    boost::multiprecision::uint128_t DstBalance = 0, ValBalance = 0;
    for (uint32_t I = 16; I < 32; ++I) {
      DstBalance <<= 8;
      ValBalance <<= 8;
      DstBalance |= Balance.bytes[I];
      ValBalance |= CallMsg.value.bytes[I];
    }
    if (DstBalance <= ValBalance) {
      Ret = 1;
      return ErrCode::Revert;
    }
    /// Take gas if create new account.
    if (!Cxt->host->account_exists(Cxt, &(CallMsg.destination))) {
      if (!EnvMgr.addCost(25000ULL)) {
        return ErrCode::Revert;
      }
    }
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
