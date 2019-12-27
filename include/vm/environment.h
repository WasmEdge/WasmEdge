// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "evmc/evmc.hpp"
#include "support/hexstr.h"

#include <cstring>
#include <fcntl.h>
#include <map>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

namespace SSVM {
namespace VM {

class Environment {
public:
  Environment() = default;
  virtual ~Environment() = default;
  Environment(const Environment &) = delete;

  virtual void clear() = 0;
};

/// Return type if is base class is Environment
template <typename T>
using TypeEnv = typename std::enable_if_t<std::is_base_of_v<Environment, T>, T>;

class EVMEnvironment : public Environment {
public:
  EVMEnvironment() = delete;
  EVMEnvironment(uint64_t &CostLimit, uint64_t &CostSum)
      : GasLimit(CostLimit), GasUsed(CostSum) {}
  virtual ~EVMEnvironment() = default;

  void clear() override {
    CallData.clear();
    ReturnData.clear();
    Caller.clear();
    CallValue.clear();
    EVMCContext = nullptr;
  }

  /// Getter of remain gas. Gas limit can be set by EnvironmentManager.
  uint64_t getGasLeft() { return GasLimit - GasUsed; }

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
  std::vector<uint8_t> &getCaller() { return Caller; }

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
  std::vector<uint8_t> &getCallValue() { return CallValue; }

  /// Setter of call value by hex string.
  void setCallValue(const std::string &Str) {
    Support::convertHexStrToValVec(Str, CallValue, 64);
  }

  /// Getter of call data vector.
  std::vector<unsigned char> &getCallData() { return CallData; }

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
  std::vector<uint8_t> &getAddress() { return Address; }

  /// Setter of address by hex string.
  void setAddress(const std::string &Str) {
    Support::convertHexStrToBytes(Str, Address, 40);
  }

  /// Getter of return data vector.
  std::vector<unsigned char> &getReturnData() { return ReturnData; }

  /// Getter of code vector.
  std::vector<uint8_t> &getCode() { return Code; }

  void setEVMCContext(struct evmc_context *Cxt) { EVMCContext = Cxt; }
  struct evmc_context *getEVMCContext() {
    return EVMCContext;
  }
  void setEVMCMessage(const struct evmc_message *Msg) {
    /// Set depth.
    Depth = Msg->depth;

    /// Set call flag.
    Flag = Msg->flags;

    /// Set call kind.
    CallKind = Msg->kind;

    /// Set gas limit.
    GasLimit = Msg->gas;

    /// Set caller.
    Caller = std::vector<uint8_t>(Msg->sender.bytes, Msg->sender.bytes + 20);

    /// Set call value. Convert big-endian to little-endian.
    CallValue = std::vector<uint8_t>(Msg->value.bytes, Msg->value.bytes + 32);
    std::reverse(CallValue.begin(), CallValue.end());

    /// Set call data.
    CallData.clear();
    if (Msg->input_size > 0 && Msg->input_data != nullptr) {
      CallData.assign(Msg->input_data, Msg->input_data + Msg->input_size);
    }

    /// Set address.
    Address = std::vector<uint8_t>(Msg->destination.bytes,
                                   Msg->destination.bytes + 20);

    ReturnData.clear();
  }
  void setEVMCCode(const uint8_t *Buf, const uint32_t Size) {
    Code = std::vector<uint8_t>(Buf, Buf + Size);
  }

private:
  uint64_t &GasLimit;
  uint64_t &GasUsed;

  /// Caller: 20 bytes sendor address.
  std::vector<uint8_t> Caller;
  /// CallValue: 32 bytes little endian. Reversed value.
  std::vector<uint8_t> CallValue;
  /// CallData: inputs, may be 0-length.
  std::vector<uint8_t> CallData;
  /// Address: 20 bytes destination address.
  std::vector<uint8_t> Address;
  /// ReturnData: return value list.
  std::vector<uint8_t> ReturnData;
  /// Code:
  std::vector<uint8_t> Code;
  /// Depth:
  uint32_t Depth = 0;
  /// Call flag:
  uint32_t Flag = 0;
  /// Call kind:
  evmc_call_kind CallKind = evmc_call_kind::EVMC_CALL;

  struct evmc_context *EVMCContext;
};

class WasiEnvironment : public Environment {
public:
  struct PreStat {
    int32_t Fd;
    uint8_t Type;
    std::vector<unsigned char> Path;
    PreStat(int32_t F, uint8_t T, std::vector<unsigned char> P)
        : Fd(F), Type(T), Path(std::move(P)) {}
  };

  WasiEnvironment();

  virtual ~WasiEnvironment() noexcept;

  void clear() override { CmdArgs.clear(); }

  int32_t getStatus() const { return Status; }
  void setStatus(int32_t S) { Status = S; }
  std::vector<std::string> &getCmdArgs() { return CmdArgs; }
  std::vector<PreStat> &getPreStats() { return PreStats; }
  int getExitCode() const { return ExitCode; }
  void setExitCode(int ExitCode) { this->ExitCode = ExitCode; }

private:
  int32_t Status;
  std::vector<std::string> CmdArgs;
  std::vector<PreStat> PreStats;
  int ExitCode = 0;
};

} // namespace VM
} // namespace SSVM
