// SPDX-License-Identifier: Apache-2.0
#include "support/hexstr.h"
#include "support/statistics.h"
#include "vm/configure.h"
#include "vm/result.h"
#include "vm/vm.h"

#include <iostream>
#include <string>

int main(int Argc, char *Argv[]) {
  if (Argc < 4) {
    /// Arg0: ./ssvm-evm
    /// Arg1: ewasm file
    /// Arg2: call data
    /// Arg3: gas
    std::cout << "Usage: ./ssvm-evm ethereum/erc20.wasm call_data gas"
              << std::endl;
    return 0;
  }

  const std::string Erc20Path(Argv[1]);
  SSVM::VM::Configure Conf;
  Conf.addVMType(SSVM::VM::Configure::VMType::Ewasm);
  SSVM::VM::VM EVM(Conf);
  SSVM::VM::EVMEnvironment *Env = EVM.getEnvironment<SSVM::VM::EVMEnvironment>(
      SSVM::VM::Configure::VMType::Ewasm);
  Env->clear();

  /// Set caller
  Env->setCaller("1234567890123456789012345678901234567890");

  /// Set call value
  Env->setCallValue("00000000000000000000000000000000");

  /// Set call data
  std::string CallDataStr(Argv[2]);
  std::vector<unsigned char> &CallData = Env->getCallData();
  SSVM::Support::convertHexStrToBytes(CallDataStr, CallData);

  EVM.setCostLimit(std::stoull(Argv[3]));
  EVM.setPath(Erc20Path);
  EVM.execute("main");

  /*
  std::map<std::string, std::string> &FinalStorage = Env->getStorage();
  std::cout << "    --- result storage: " << std::endl;
  for (auto it = FinalStorage.begin(); it != FinalStorage.end(); ++it) {
    std::cout << "         " << it->first << " " << it->second << std::endl;
  }
  */

  std::vector<unsigned char> &FinalReturn = Env->getReturnData();
  std::cout << "    --- return data: " << std::endl << "         ";
  for (auto it = FinalReturn.begin(); it != FinalReturn.end(); ++it) {
    printf("%02x", *it);
  }
  std::cout << std::endl;

  SSVM::Support::statistics.show();
  return 0;
}
