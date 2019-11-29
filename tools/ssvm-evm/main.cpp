#include "support/hexstr.h"
#include "vm/configure.h"
#include "vm/result.h"
#include "vm/vm.h"

#include <iostream>

int main(int Argc, char *Argv[]) {
  if (Argc < 3) {
    /// Arg0: ./ssvm-evm
    /// Arg1: ewasm file
    /// Arg2: call data
    std::cout << "Usage: ./ssvm-evm ethereum/erc20.wasm call_data" << std::endl;
    return 0;
  }

  const std::string Erc20Path(Argv[1]);
  SSVM::VM::Configure Conf(SSVM::VM::Configure::VMType::Ewasm);
  SSVM::VM::VM EVM(Conf);
  SSVM::VM::EVMEnvironment *Env =
      dynamic_cast<SSVM::VM::EVMEnvironment *>(EVM.getEnvironment());
  Env->clear();

  /// Set caller
  std::string &Caller = Env->getCaller();
  Caller = "1234567890123456789012345678901234567890";

  /// Set call value
  std::string &CallValue = Env->getCallValue();
  CallValue = "ffffffffffffffffffffffffffffffff";

  /// Set call data
  std::string CallDataStr(Argv[2]);
  std::vector<unsigned char> &CallData = Env->getCallData();
  if (CallDataStr.length() & 0x01U) {
    CallDataStr += "0";
  }
  for (auto It = CallDataStr.cbegin(); It != CallDataStr.cend(); It += 2) {
    char CH = *It;
    char CL = *(It + 1);
    CallData.push_back(SSVM::Support::convertCharToHex(CL) +
                       (SSVM::Support::convertCharToHex(CH) << 4));
  }

  EVM.setPath(Erc20Path);
  EVM.execute();

  std::map<std::string, std::string> &FinalStorage = Env->getStorage();
  std::cout << "    --- result storage: " << std::endl;
  for (auto it = FinalStorage.begin(); it != FinalStorage.end(); ++it) {
    std::cout << "         " << it->first << " " << it->second << std::endl;
  }

  std::vector<unsigned char> &FinalReturn = Env->getReturnData();
  std::cout << "    --- return data: " << std::endl << "         ";
  for (auto it = FinalReturn.begin(); it != FinalReturn.end(); ++it) {
    printf("%u ", *it);
  }
  std::cout << std::endl;
  return 0;
}
