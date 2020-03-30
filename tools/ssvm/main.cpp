// SPDX-License-Identifier: Apache-2.0
#include "common/value.h"
#include "expvm/configure.h"
#include "expvm/vm.h"

#include <iostream>

int main(int Argc, char *Argv[]) {
  if (Argc < 3) {
    /// Arg0: ./ssvm
    /// Arg1: wasm file
    /// Arg2: invoke function name
    /// Arg3...: inputs
    std::cout << "Usage: ./ssvm wasm_file.wasm func_name [args...]"
              << std::endl;
    return 0;
  }

  std::string InputPath(Argv[1]);
  SSVM::ExpVM::Configure Conf;
  SSVM::ExpVM::VM VM(Conf);

  /// Parameters and return values.
  std::vector<SSVM::ValVariant> Params, Results;
  uint32_t Err = 0;

  for (int I = 3; I < Argc; I++) {
    Params.push_back(static_cast<uint32_t>(std::stoul(Argv[I])));
  }
  if (auto Res = VM.runWasmFile(InputPath, Argv[2], Params)) {
    Results = *Res;
    for (auto &It : Results) {
      std::cout << " Return value: " << std::get<uint32_t>(It) << std::endl;
    }
  } else {
    Err = static_cast<uint32_t>(Res.error());
    std::cout << " Failed. Code : " << Err << std::endl;
  }

  return Err;
}
