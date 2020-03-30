// SPDX-License-Identifier: Apache-2.0
#include "expvm/configure.h"
#include "expvm/vm.h"
#include "helper.h"
#include "host/wasi/wasimodule.h"
#include "support/log.h"
#include <iostream>

int main(int Argc, char *Argv[]) {
  if (Argc < 2) {
    /// Args pass to wasm config.
    /// Arg0: ./ssvm-qitc
    /// Arg1: wasm file
    /// Other args are pass into ONNC runtime
    std::cout << "Usage: ./ssvm-qitc model_wasm [args...]" << std::endl;
    return 0;
  }

  std::string InputPath(Argv[1]);
  SSVM::ExpVM::Configure Conf;
  Conf.addVMType(SSVM::ExpVM::Configure::VMType::Wasi);
  Conf.addVMType(SSVM::ExpVM::Configure::VMType::ONNC);
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

  /// Insert helper host functions.
  SSVM::Host::QITCModule QITCMod;
  VM.registerModule(QITCMod);
  VM.runWasmFile(InputPath, "_start");
  return 0;
}
