// SPDX-License-Identifier: Apache-2.0
#include "vm/configure.h"
#include "vm/result.h"
#include "vm/vm.h"

#include <iostream>

int main(int Argc, char *Argv[]) {
  if (Argc < 2) {
    /// Arg0: ./ssvm
    /// Arg1: wasm file
    /// Arg2: start func name
    std::cout << "Usage: ./ssvm wasm_file.wasm [start_func]" << std::endl;
    return 0;
  }

  std::string InputPath(Argv[1]);
  SSVM::VM::Configure Conf(SSVM::VM::Configure::VMType::Wasm);
  if (Argc == 3) {
    Conf.setStartFuncName(Argv[2]);
  }
  SSVM::VM::VM VM(Conf);
  SSVM::Result Result;

  VM.setPath(InputPath);
  VM.execute();
  Result = VM.getResult();
  return Result.getErrCode();
}
