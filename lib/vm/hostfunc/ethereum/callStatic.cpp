// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/callStatic.h"
#include "keccak/Keccak.h"
#include "support/hexstr.h"

namespace SSVM {
namespace Executor {

ErrCode EEICallStatic::body(VM::EnvironmentManager &EnvMgr,
                            Instance::MemoryInstance &MemInst, uint32_t &Ret,
                            uint64_t Gas, uint32_t AddressOffset,
                            uint32_t DataOffset, uint32_t DataLength) {
  /// Load address and convert to uint256.
  evmc_address Addr = loadAddress(MemInst, AddressOffset);
  boost::multiprecision::uint256_t AddrNum = 0;
  for (auto &I : Addr.bytes) {
    AddrNum <<= 8;
    AddrNum |= I;
  }

  if (AddrNum == 9) {
    /// Check data copy cost.
    if (addCopyCost(EnvMgr, DataLength) != ErrCode::Success) {
      return ErrCode::CostLimitExceeded;
    }

    /// Prepare call data.
    std::vector<unsigned char> Data;
    if (ErrCode Status = MemInst.getBytes(Data, DataOffset, DataLength);
        Status != ErrCode::Success) {
      return Status;
    }

    /// Run Keccak
    Keccak K(256);
    for (auto &I : Data) {
      K.addData(I);
    }
    Env.getReturnData() = K.digest();

    /// Return: Result(i32)
    Ret = 0U;
  } else {
    /// Prepare call message.
    evmc_message CallMsg = {
        .kind = evmc_call_kind::EVMC_CALL,
        .flags = evmc_flags::EVMC_STATIC,
        .depth = static_cast<int32_t>(Env.getDepth() + 1),
        .gas = static_cast<int64_t>(std::min(Gas, getMaxCallGas())),
        .destination = Addr,
        .sender = Env.getAddressEVMC(),
        .input_data = nullptr,
        .input_size = 0,
        .value = {}};

    /// Return: Result(i32)
    return callContract(EnvMgr, MemInst, Ret, CallMsg, DataOffset, DataLength);
  }
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
