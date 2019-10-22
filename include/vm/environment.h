#pragma once

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

private:
  unsigned int GasLeft;
  std::map<std::string, std::string> Storage;
  std::vector<unsigned char> CallData;
  std::vector<unsigned char> ReturnData;
  std::string Caller;
  std::string CallValue;
};

class WasiEnvironment : public Environment {
public:
  WasiEnvironment() = default;
  virtual ~WasiEnvironment() = default;

  virtual void clear() { CmdArgs.clear(); }

  std::vector<std::string> &getCmdArgs() { return CmdArgs; }

private:
  std::vector<std::string> CmdArgs;
};

} // namespace VM
} // namespace SSVM