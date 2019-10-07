#pragma once

#include <map>
#include <string>
#include <vector>

namespace SSVM {
namespace VM {

class Environment {
public:
  Environment() = default;
  ~Environment() = default;

  std::map<std::string, std::string> &getStorage() { return Storage; }
  unsigned int &getGasLeft() { return GasLeft; }
  std::vector<unsigned char> &getCallData() { return CallData; }
  std::vector<unsigned char> &getReturnData() { return ReturnData; }
  std::string &getCaller() { return Caller; }
  std::string &getCallValue() { return CallValue; }

private:
  unsigned int GasLeft;
  std::map<std::string, std::string> Storage;
  std::vector<unsigned char> CallData = {
      0xB1U, 0xAAU, 0x1FU, 0x4EU, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
      0U,    0U,    0U,    0U,    0U, 0U, 0U, 0U, 0U, 0U, 0U,
      0U,    0U,    0U,    0U,    0U, 0U, 0U, 0U, 0U, 0U};
  std::vector<unsigned char> ReturnData;
  std::string Caller = "1234567890123456789012345678901234567890";
  std::string CallValue = "ffffffffffffffffffffffffffffffff";
};

} // namespace VM
} // namespace SSVM