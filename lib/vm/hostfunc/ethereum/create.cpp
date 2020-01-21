// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/create.h"
#include <boost/multiprecision/cpp_int.hpp>

namespace SSVM {
namespace Executor {

ErrCode EEICreate::body(VM::EnvironmentManager &EnvMgr,
                        Instance::MemoryInstance &MemInst, uint32_t &Ret,
                        uint32_t ValueOffset, uint32_t DataOffset,
                        uint32_t DataLength, uint32_t ResultOffset) {
  evmc_context *Cxt = Env.getEVMCContext();
  ErrCode Status = ErrCode::Success;

  /// Prepare creation message.
  evmc_message CreateMsg;
  CreateMsg.destination = {};
  std::memcpy(CreateMsg.sender.bytes, &Env.getAddress()[0], 20);
  MemInst.getArray(CreateMsg.value.bytes + 16, ValueOffset, 16, true);

  /// Check depth.
  if (Env.getDepth() >= 1024) {
    Ret = 1;
    return ErrCode::Revert;
  }
  /// Check balance.
  evmc_uint256be Balance = Cxt->host->get_balance(Cxt, &(CreateMsg.sender));
  boost::multiprecision::uint128_t DstBalance = 0, ValBalance = 0;
  for (uint32_t I = 16; I < 32; ++I) {
    DstBalance <<= 8;
    ValBalance <<= 8;
    DstBalance |= Balance.bytes[I];
    ValBalance |= CreateMsg.value.bytes[I];
  }
  if (DstBalance <= ValBalance) {
    Ret = 1;
    return ErrCode::Revert;
  }

  /// Setup message.
  std::vector<uint8_t> Code;
  CreateMsg.input_data = nullptr;
  if (DataLength > 0) {
    MemInst.setBytes(Code, DataOffset, 0, DataLength);
    CreateMsg.input_data = &Code[0];
  }
  CreateMsg.input_size = Code.size();
  CreateMsg.depth = Env.getDepth() + 1;
  CreateMsg.kind = EVMC_CREATE;
  CreateMsg.flags = 0;
  CreateMsg.gas = Env.getGasLeft() - Env.getGasLeft() / 64;
  EnvMgr.addCost(CreateMsg.gas);

  /// Create.
  evmc_result CreateRes = Cxt->host->call(Cxt, &CreateMsg);

  /// Return left gas.
  if (CreateRes.gas_left < 0) {
    return ErrCode::Revert;
  }
  EnvMgr.subCost(CreateRes.gas_left);
  if (CreateRes.status_code == EVMC_SUCCESS) {
    Status = MemInst.setArray(CreateRes.create_address.bytes, ResultOffset, 20);
    Env.getReturnData().clear();
  } else if (CreateRes.output_data) {
    Env.getReturnData().assign(CreateRes.output_data,
                               CreateRes.output_data + CreateRes.output_size);
  } else {
    Env.getReturnData().clear();
  }

  /// Return: Result(i32)
  switch (CreateRes.status_code) {
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
