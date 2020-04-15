// SPDX-License-Identifier: Apache-2.0
#include "common/value.h"
#include "expvm/configure.h"
#include "expvm/vm.h"
#include "host/wasi/wasimodule.h"
#include "support/filesystem.h"
#include <cstdlib>
#include <iostream>

int main(int Argc, char *Argv[]) {
  if (Argc < 2) {
    /// Arg0: ./ssvmr
    /// Arg1: so file
    /// Arg2...: inputs
    std::cout << "Usage: ./ssvmr wasm_so.so [args...]" << std::endl;
    return 0;
  }

  std::string InputPath = std::filesystem::absolute(Argv[1]).string();
  SSVM::ExpVM::Configure Conf;
  Conf.addVMType(SSVM::ExpVM::Configure::VMType::Wasi);
  SSVM::ExpVM::VM VM(Conf);

  SSVM::Host::WasiModule *WasiMod = dynamic_cast<SSVM::Host::WasiModule *>(
      VM.getImportModule(SSVM::ExpVM::Configure::VMType::Wasi));

  std::vector<std::string> &CmdArgsVec = WasiMod->getEnv().getCmdArgs();
  for (int I = 1; I < Argc; I++) {
    CmdArgsVec.push_back(std::string(Argv[I]));
  }
  for (auto It = CmdArgsVec.begin(); It != CmdArgsVec.end(); It++) {
    std::cout << " Args : " << *It << std::endl;
  }

  if (auto Result = VM.runWasmFile(InputPath, "_start")) {
    return EXIT_SUCCESS;
  } else {
    std::cout << "Failed. Error code : "
              << static_cast<uint32_t>(Result.error()) << '\n';
    return EXIT_FAILURE;
  }
}
