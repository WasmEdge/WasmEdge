#pragma once

#include <string>
#include <vector>

namespace SSVM {
namespace VM {

class Environment {
public:
  Environment() = default;
  ~Environment() = default;

  unsigned int &getGasLeft() { return GasLeft; }
  std::vector<unsigned char> &getCallData() { return CallData; }
  std::vector<unsigned char> &getReturnData() { return ReturnData; }
  std::string &getCaller() { return Caller; }
  std::string &getCallValue() { return CallValue; }

private:
  unsigned int GasLeft;
  std::vector<unsigned char> CallData;
  std::vector<unsigned char> ReturnData;
  std::string Caller;
  std::string CallValue;
};

} // namespace VM
} // namespace SSVM