// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"
#include "eeienv.h"
#include "evmc/evmc.hpp"

#include <boost/multiprecision/cpp_int.hpp>

namespace SSVM {
namespace Host {

template <typename T> class EEI : public Runtime::HostFunction<T> {
public:
  EEI(EVMEnvironment &HostEnv, const uint64_t Cost = 0)
      : Runtime::HostFunction<T>(Cost), Env(HostEnv) {}

protected:
  EVMEnvironment &Env;

  /// Helper function of add copy cost.
  Expect<void> addCopyCost(const uint64_t Length) {
    uint64_t TakeGas = 3 * ((Length + 31) / 32);
    if (!Env.consumeGas(TakeGas)) {
      return Unexpect(ErrCode::CostLimitExceeded);
    }
    return {};
  }

  /// Helper function to get max call gas.
  uint64_t getMaxCallGas() {
    return Env.getGasLeft() - (Env.getGasLeft() / 64);
  }

  /// Helper function to load value and store to evmc_uint256be.
  Expect<evmc_uint256be> loadUInt(Runtime::Instance::MemoryInstance &MemInst,
                                  uint32_t Off, uint32_t Bytes = 32) {
    if (Bytes > 32) {
      Bytes = 32;
    }
    evmc_uint256be Dst = {};
    if (auto Res =
            MemInst.getArray(Dst.bytes + (32 - Bytes), Off, Bytes, true)) {
      return Dst;
    } else {
      return Unexpect(Res);
    }
  }

  /// Helper function to load evmc_address from memory instance.
  Expect<evmc_address> loadAddress(Runtime::Instance::MemoryInstance &MemInst,
                                   uint32_t Off) {
    evmc_address Dst = {};
    if (auto Res = MemInst.getArray(Dst.bytes, Off, 20)) {
      return Dst;
    } else {
      return Unexpect(Res);
    }
  }

  /// Helper function to load evmc_bytes32 from memory instance.
  Expect<evmc_bytes32> loadBytes32(Runtime::Instance::MemoryInstance &MemInst,
                                   uint32_t Off) {
    evmc_bytes32 Dst = {};
    if (auto Res = MemInst.getArray(Dst.bytes, Off, 32)) {
      return Dst;
    } else {
      return Unexpect(Res);
    }
  }

  /// Helper function to reverse and store evmc_uint256be to memory instance.
  Expect<void> storeUInt(Runtime::Instance::MemoryInstance &MemInst,
                         const evmc_uint256be &Src, uint32_t Off,
                         uint32_t Bytes = 32) {
    if (Bytes > 32) {
      Bytes = 32;
    }
    for (uint32_t I = 0; I < 32 - Bytes; ++I) {
      if (Src.bytes[I]) {
        return Unexpect(ErrCode::ExecutionFailed);
      }
    }
    return MemInst.setArray(Src.bytes + (32 - Bytes), Off, Bytes, true);
  }

  /// Helper function to store evmc_address to memory instance.
  Expect<void> storeAddress(Runtime::Instance::MemoryInstance &MemInst,
                            const evmc_address &Addr, uint32_t Off) {
    return MemInst.setArray(Addr.bytes, Off, 20);
  }

  /// Helper function to store evmc_bytes32 to memory instance.
  Expect<void> storeBytes32(Runtime::Instance::MemoryInstance &MemInst,
                            const evmc_bytes32 &Bytes, uint32_t Off) {
    return MemInst.setArray(Bytes.bytes, Off, 32);
  }

  /// Helper function to convert evmc_bytes32 to uint128_t.
  Expect<boost::multiprecision::uint128_t>
  convToUInt128(const evmc_uint256be &Src) {
    boost::multiprecision::uint128_t Dst = 0;
    for (uint32_t I = 0; I < 16; ++I) {
      if (Src.bytes[I]) {
        return Unexpect(ErrCode::ExecutionFailed);
      }
    }
    for (uint32_t I = 16; I < 32; ++I) {
      Dst <<= 8;
      Dst |= Src.bytes[I];
    }
    return Dst;
  }

  /// Helper function to make call operation.
  Expect<uint32_t> callContract(Runtime::Instance::MemoryInstance &MemInst,
                                evmc_message &Msg, uint32_t DataOffset,
                                uint32_t DataLength,
                                uint32_t CreateResOffset = 0) {
    evmc_context *Cxt = Env.getEVMCContext();

    /// Check depth.
    if (Env.getDepth() >= 1024) {
      return 1;
    }

    /// Setup input data.
    std::vector<uint8_t> Code;
    if (DataLength > 0) {
      if (auto Res = MemInst.getBytes(DataOffset, DataLength)) {
        Code = *Res;
      } else {
        return Unexpect(Res);
      }
      Msg.input_data = &Code[0];
      Msg.input_size = Code.size();
    }

    /// Check flag.
    if ((Msg.kind == evmc_call_kind::EVMC_CREATE ||
         (Msg.kind == evmc_call_kind::EVMC_CALL &&
          !evmc::is_zero(Msg.value))) &&
        (Env.getFlag() & evmc_flags::EVMC_STATIC)) {
      return Unexpect(ErrCode::ExecutionFailed);
    }

    /// Take additional gas.
    if ((Msg.kind == evmc_call_kind::EVMC_CALL ||
         Msg.kind == evmc_call_kind::EVMC_CALLCODE) &&
        !evmc::is_zero(Msg.value)) {
      /// Take transfer gas.
      if (!Env.consumeGas(9000ULL)) {
        return Unexpect(ErrCode::CostLimitExceeded);
      }

      /// Take gas if create new account.
      if (!Cxt->host->account_exists(Cxt, &(Msg.destination))) {
        if (!Env.consumeGas(25000ULL)) {
          return Unexpect(ErrCode::CostLimitExceeded);
        }
      }
    }

    /// Check balance.
    if (((Msg.kind == evmc_call_kind::EVMC_CALL ||
          Msg.kind == evmc_call_kind::EVMC_CALLCODE) &&
         !evmc::is_zero(Msg.value)) ||
        Msg.kind == evmc_call_kind::EVMC_CREATE) {
      boost::multiprecision::uint128_t DstBalance = 0, ValBalance = 0;
      if (auto Res =
              convToUInt128(Cxt->host->get_balance(Cxt, &(Msg.sender)))) {
        DstBalance = *Res;
      } else {
        return Unexpect(ErrCode::ExecutionFailed);
      }
      if (auto Res = convToUInt128(Msg.value)) {
        ValBalance = *Res;
      } else {
        return Unexpect(ErrCode::ExecutionFailed);
      }
      if (DstBalance < ValBalance) {
        return 1;
      }
    }

    /// Assign gas to callee. Msg.gas is ensured <= remain gas in caller.
    Env.consumeGas(Msg.gas);

    // Add gas stipend for value transfers.
    if (!evmc::is_zero(Msg.value) && Msg.kind != evmc_call_kind::EVMC_CREATE) {
      Msg.gas += 2300ULL;
    }

    /// Call.
    evmc_result CallRes = Cxt->host->call(Cxt, &Msg);

    /// Return left gas.
    if (CallRes.gas_left < 0) {
      return Unexpect(ErrCode::ExecutionFailed);
    }
    Env.returnGas(CallRes.gas_left);

    /// Return data.
    if (Msg.kind == evmc_call_kind::EVMC_CREATE &&
        CallRes.status_code == EVMC_SUCCESS) {
      if (auto Res =
              storeAddress(MemInst, CallRes.create_address, CreateResOffset);
          !Res) {
        return Unexpect(Res);
      }
      Env.getReturnData().clear();
    } else if (CallRes.output_data) {
      Env.getReturnData().assign(CallRes.output_data,
                                 CallRes.output_data + CallRes.output_size);
    } else {
      Env.getReturnData().clear();
    }

    /// Return status.
    switch (CallRes.status_code) {
    case evmc_status_code::EVMC_SUCCESS:
      return 0;
      break;
    case evmc_status_code::EVMC_REVERT:
      return 2;
      break;
    default:
      break;
    }
    return 1;
  }
};

} // namespace Host
} // namespace SSVM
