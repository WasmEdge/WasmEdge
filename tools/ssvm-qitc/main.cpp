// SPDX-License-Identifier: Apache-2.0
#include "helper.h"
#include "host/wasi/wasimodule.h"
#include "support/log.h"
#include "vm/configure.h"
#include "vm/vm.h"
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
  SSVM::VM::Configure Conf;
  Conf.addVMType(SSVM::VM::Configure::VMType::Wasi);
  Conf.addVMType(SSVM::VM::Configure::VMType::ONNC);
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

  /// Insert helper host functions.
  SSVM::Host::QITCModule QITCMod;
  VM.registerModule(QITCMod);
  VM.runWasmFile(InputPath, "_start");
  return 0;
}
