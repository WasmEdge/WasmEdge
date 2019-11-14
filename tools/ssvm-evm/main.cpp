#include "vm/configure.h"
#include "vm/result.h"
#include "vm/vm.h"

#include <dirent.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>

int main(int Argc, char *Argv[]) {
  const std::string Erc20Path("ethereum/erc20.wasm");
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
  std::vector<unsigned char> &CallData = Env->getCallData();
  CallData = {78, 110, 194, 71, 0,  0,   0,   0,   0,  0,  0,   0,   0,  0,
              0,  0,   18,  52, 86, 120, 144, 18,  52, 86, 120, 144, 18, 52,
              86, 120, 144, 18, 52, 86,  120, 144, 0,  0,  0,   0,   0,  0,
              0,  0,   0,   0,  0,  0,   0,   0,   0,  0,  0,   0,   0,  0,
              0,  0,   0,   0,  0,  0,   0,   0,   0,  0,  0,   100};

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
