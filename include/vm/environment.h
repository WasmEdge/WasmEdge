// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "evmc/evmc.h"

#include <map>
#include <string>
#include <vector>

namespace SSVM {
namespace VM {

class Environment {
public:
  Environment() = default;
  virtual ~Environment() = default;

  virtual void clear() = 0;
};

/// Return type if is base class is Environment
template <typename T>
using TypeEnv = typename std::enable_if_t<std::is_base_of_v<Environment, T>, T>;

class EVMEnvironment : public Environment {
public:
  EVMEnvironment() = default;
  virtual ~EVMEnvironment() = default;

  virtual void clear() {
    GasLeft = 0;
    Storage.clear();
    CallData.clear();
    ReturnData.clear();
    Caller.clear();
    CallValue.clear();
  }

  std::map<std::string, std::string> &getStorage() { return Storage; }
  unsigned int &getGasLeft() { return GasLeft; }
  std::vector<unsigned char> &getCallData() { return CallData; }
  std::vector<unsigned char> &getReturnData() { return ReturnData; }
  std::string &getCaller() { return Caller; }
  std::string &getCallValue() { return CallValue; }
  void setContext(struct evmc_context *context) { Context = context; }

private:
  unsigned int GasLeft;
  std::map<std::string, std::string> Storage;
  std::vector<unsigned char> CallData;
  std::vector<unsigned char> ReturnData;
  std::string Caller;
  std::string CallValue;
  struct evmc_context *Context;
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

  virtual void clear() { CmdArgs.clear(); }

  int32_t getStatus() const { return Status; }
  void setStatus(int32_t S) { Status = S; }
  std::vector<std::string> &getCmdArgs() { return CmdArgs; }
  std::vector<PreStat> &getPreStats() { return PreStats; }

private:
  int32_t Status;
  std::vector<std::string> CmdArgs;
  std::vector<PreStat> PreStats;
};

} // namespace VM
} // namespace SSVM
