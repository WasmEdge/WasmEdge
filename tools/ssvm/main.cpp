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
    /// Arg3...: inputs
    std::cout << "Usage: ./ssvm wasm_file.wasm [start_func] [args...]"
              << std::endl;
    return 0;
  }

  std::string InputPath(Argv[1]);
  SSVM::VM::Configure Conf;
  SSVM::VM::VM VM(Conf);
  SSVM::VM::Result Result;

  VM.setPath(InputPath);
  for (int I = 3; I < Argc; I++) {
    VM.appendArgument(static_cast<uint32_t>(atoi(Argv[I])));
  }
  if (Argc >= 3) {
    VM.execute(Argv[2]);
  } else {
    VM.execute();
  }
  std::vector<SSVM::ValVariant> Rets;
  VM.getReturnValue(Rets);
  Result = VM.getResult();

  for (auto It = Rets.begin(); It != Rets.end(); It++) {
    std::cout << " Return value: " << std::get<uint32_t>(*It) << std::endl;
  }
  return Result.getErrCode();
}
