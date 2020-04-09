// SPDX-License-Identifier: Apache-2.0
#include "common/value.h"
#include "host/wasi/wasimodule.h"
#include "support/filesystem.h"
#include "vm/configure.h"
#include "vm/vm.h"
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
  SSVM::VM::Configure Conf;
  Conf.addVMType(SSVM::VM::Configure::VMType::Wasi);
  SSVM::VM::VM VM(Conf);

  SSVM::Host::WasiModule *WasiMod = dynamic_cast<SSVM::Host::WasiModule *>(
      VM.getImportModule(SSVM::VM::Configure::VMType::Wasi));

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
