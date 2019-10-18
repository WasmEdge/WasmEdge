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
};

class EVMEnvironment : public Environment {
public:
  EVMEnvironment() = default;
  virtual ~EVMEnvironment() = default;

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
};

} // namespace VM
} // namespace SSVM