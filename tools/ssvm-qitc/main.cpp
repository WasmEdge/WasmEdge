// SPDX-License-Identifier: Apache-2.0
#include "easyloggingpp/easylogging++.h"
#include "helper.h"
#include "vm/configure.h"
#include "vm/result.h"
#include "vm/vm.h"
#include <iostream>

INITIALIZE_EASYLOGGINGPP

int main(int Argc, char *Argv[]) {
  START_EASYLOGGINGPP(Argc, Argv);
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
  SSVM::VM::WasiEnvironment *Env = VM.getEnvironment<SSVM::VM::WasiEnvironment>(
      SSVM::VM::Configure::VMType::Wasi);
  std::vector<std::string> &CmdArgsVec = Env->getCmdArgs();
  for (int I = 1; I < Argc; I++) {
    CmdArgsVec.push_back(std::string(Argv[I]));
  }
  for (auto It = CmdArgsVec.begin(); It != CmdArgsVec.end(); It++) {
    std::cout << " Args : " << *It << std::endl;
  }
  SSVM::VM::Result Result;

/// Insert helper host functions.
#ifdef ONNC_WASM
  VM.setHostFunction(std::make_unique<SSVM::Executor::QITCTimerStart>());
  VM.setHostFunction(std::make_unique<SSVM::Executor::QITCTimerStop>());
  VM.setHostFunction(std::make_unique<SSVM::Executor::QITCTimerClear>());
#endif

  VM.setPath(InputPath);
  VM.execute("_start");
  Result = VM.getResult();
  return Result.getErrCode();
}
