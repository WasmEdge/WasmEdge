// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/types.h"
#include "common/value.h"
#include "evmc/evmc.hpp"
#include "support/hexstr.h"

#include <cstring>
#include <string>
#include <vector>

namespace SSVM {
namespace Host {

class EVMEnvironment {
public:
  EVMEnvironment() = delete;
  EVMEnvironment(uint64_t &CostLimit, uint64_t &CostSum)
      : GasLimit(CostLimit), GasUsed(CostSum) {}
  ~EVMEnvironment() = default;

  /// Getter of remain gas. Gas limit can be set by EnvironmentManager.
  uint64_t getGasLeft() { return GasLimit - GasUsed; }

  /// Consume gas.
  bool consumeGas(const uint64_t Gas);

  /// Return gas.
  bool returnGas(const uint64_t Gas);

  /// Getter and setter of depth.
  uint32_t &getDepth() { return Depth; }

  /// Getter and setter of flag.
  uint32_t &getFlag() { return Flag; }

  /// Getter and setter of call kind.
  evmc_call_kind &getCallKind() { return CallKind; }

  /// Getter of caller and converting into hex string.
  std::string getCallerStr() {
    std::string Str;
    Support::convertBytesToHexStr(Caller, Str, 40);
    return Str;
  }

  /// Getter of caller in EVMC version.
  evmc_address getCallerEVMC() {
    evmc_address Addr;
    std::memcpy(Addr.bytes, &Caller[0], 20);
    return Addr;
  }

  /// Getter of caller vector.
  Bytes &getCaller() { return Caller; }

  /// Setter of caller by hex string.
  void setCaller(const std::string &Str) {
    Support::convertHexStrToBytes(Str, Caller, 40);
  }

  /// Getter of call value and converting into hex string.
  std::string getCallValueStr() {
    std::string Str;
    Support::convertValVecToHexStr(CallValue, Str, 64);
    return Str;
  }

  /// Getter of call value in EVMC version.
  evmc_bytes32 getCallValueEVMC() {
    evmc_bytes32 Val;
    std::memcpy(Val.bytes, &CallValue[0], 32);
    return Val;
  }

  /// Getter of call value vector.
  Bytes &getCallValue() { return CallValue; }

  /// Setter of call value by hex string.
  void setCallValue(const std::string &Str) {
    Support::convertHexStrToValVec(Str, CallValue, 64);
  }

  /// Getter of call data vector.
  Bytes &getCallData() { return CallData; }

  /// Getter of address and converting into hex string.
  std::string getAddressStr() {
    std::string Str;
    Support::convertBytesToHexStr(Address, Str, 40);
    return Str;
  }

  /// Getter of address in EVMC version.
  evmc_address getAddressEVMC() {
    evmc_address Addr;
    std::memcpy(Addr.bytes, &Address[0], 20);
    return Addr;
  }

  /// Getter of address vector.
  Bytes &getAddress() { return Address; }

  /// Setter of address by hex string.
  void setAddress(const std::string &Str) {
    Support::convertHexStrToBytes(Str, Address, 40);
  }

  /// Getter of return data vector.
  Bytes &getReturnData() { return ReturnData; }

  /// Getter of code vector.
  Bytes &getCode() { return Code; }

  /// Setter of EVMC context.
  void setEVMCContext(struct evmc_context *Cxt) { EVMCContext = Cxt; }

  /// Getter of EVMC context.
  struct evmc_context *getEVMCContext() {
    return EVMCContext;
  }

  /// Initialize by EVMC message.
  void setEVMCMessage(const struct evmc_message *Msg);

  /// Set code by EVMC.
  void setEVMCCode(const uint8_t *Buf, const uint32_t Size) {
    Code = Bytes(Buf, Buf + Size);
  }

private:
  /// Gas measurement
  uint64_t &GasLimit;
  uint64_t &GasUsed;

  /// Caller: 20 bytes sendor address.
  Bytes Caller;
  /// CallValue: 32 bytes little endian. Reversed value.
  Bytes CallValue;
  /// CallData: inputs, may be 0-length.
  Bytes CallData;
  /// Address: 20 bytes destination address.
  Bytes Address;
  /// ReturnData: return value list.
  Bytes ReturnData;
  /// Code:
  Bytes Code;
  /// Depth:
  uint32_t Depth = 0;
  /// Call flag:
  uint32_t Flag = 0;
  /// Call kind:
  evmc_call_kind CallKind = evmc_call_kind::EVMC_CALL;
  /// EVMC context:
  struct evmc_context *EVMCContext;
};

} // namespace Host
} // namespace SSVM