// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "executor/hostfunc.h"
#include "vm/environment.h"
#include <boost/multiprecision/cpp_int.hpp>

namespace SSVM {
namespace Executor {

template <typename T> class EEI : public HostFunction<T> {
public:
  EEI(VM::EVMEnvironment &HostEnv, const std::string &FuncName = "",
      const uint64_t &Cost = 0)
      : HostFunction<T>("ethereum", FuncName, Cost), Env(HostEnv) {}

protected:
  VM::EVMEnvironment &Env;

  /// Helper function of add copy cost.
  ErrCode addCopyCost(VM::EnvironmentManager &EnvMgr, uint64_t Length) {
    uint64_t TakeGas = 3 * ((Length + 31) / 32);
    return EnvMgr.addCost(TakeGas) ? ErrCode::Success : ErrCode::Revert;
  }

  /// Helper function to load value and store to evmc_uint256be.
  evmc_uint256be loadUInt(Instance::MemoryInstance &MemInst, uint32_t Off,
                          uint32_t Bytes = 32) {
    if (Bytes > 32) {
      Bytes = 32;
    }
    evmc_uint256be Val = {};
    MemInst.getArray(Val.bytes + (32 - Bytes), Off, Bytes, true);
    return Val;
  }

  /// Helper function to load evmc_address from memory instance.
  evmc_address loadAddress(Instance::MemoryInstance &MemInst, uint32_t Off) {
    evmc_address Addr;
    MemInst.getArray(Addr.bytes, Off, 20);
    return Addr;
  }

  /// Helper function to load evmc_bytes32 from memory instance.
  evmc_bytes32 loadBytes32(Instance::MemoryInstance &MemInst, uint32_t Off) {
    evmc_bytes32 Bytes;
    MemInst.getArray(Bytes.bytes, Off, 32);
    return Bytes;
  }

  /// Helper function to reverse and store evmc_uint256be to memory instance.
  ErrCode storeUInt(Instance::MemoryInstance &MemInst,
                    const evmc_uint256be &Src, uint32_t Off,
                    uint32_t Bytes = 32) {
    if (Bytes > 32) {
      Bytes = 32;
    }
    return MemInst.setArray(Src.bytes + (32 - Bytes), Off, Bytes, true);
  }

  /// Helper function to store evmc_address to memory instance.
  ErrCode storeAddress(Instance::MemoryInstance &MemInst,
                       const evmc_address &Addr, uint32_t Off) {
    return MemInst.setArray(Addr.bytes, Off, 20);
  }

  /// Helper function to store evmc_bytes32 to memory instance.
  ErrCode storeBytes32(Instance::MemoryInstance &MemInst,
                       const evmc_bytes32 &Bytes, uint32_t Off) {
    return MemInst.setArray(Bytes.bytes, Off, 32);
  }

  /// Helper function to convert evmc_bytes32 to uint128_t.
  boost::multiprecision::uint128_t convToUInt128(const evmc_uint256be &Src) {
    boost::multiprecision::uint128_t Res = 0;
    for (uint32_t I = 16; I < 32; ++I) {
      Res <<= 8;
      Res |= Src.bytes[I];
    }
    return Res;
  }
};

} // namespace Executor
} // namespace SSVM
