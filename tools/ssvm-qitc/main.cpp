#include "vm/configure.h"
#include "vm/result.h"
#include "vm/vm.h"

#include <dirent.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>

int main(int Argc, char *Argv[]) {
  if (Argc != 4 && Argc != 5) {
    std::cout << "Usage: ./ssvm model_wasm tensor_file weight_file"
              << std::endl;
    return 0;
  }

  char *cwdstr = getcwd(NULL, 0);
  opendir(cwdstr);

  std::string InputPath(Argv[1]);
  SSVM::VM::Configure Conf(SSVM::VM::Configure::VMType::Wasi);
  SSVM::VM::VM VM(Conf);
  SSVM::VM::WasiEnvironment *Env =
      dynamic_cast<SSVM::VM::WasiEnvironment *>(VM.getEnvironment());
  std::vector<std::string> &CmdArgsVec = Env->getCmdArgs();
  for (int I = 1; I < Argc; I++) {
    CmdArgsVec.push_back(std::string(Argv[I]));
  }
  for (auto It = CmdArgsVec.begin(); It != CmdArgsVec.end(); It++) {
    std::cout << " Args : " << *It << std::endl;
  }
  SSVM::Result Result;
  VM.setPath(InputPath);
  VM.execute();
  Result = VM.getResult();
  return 0;
}
