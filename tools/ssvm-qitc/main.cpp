// SPDX-License-Identifier: Apache-2.0
#include "helper.h"
#include "vm/configure.h"
#include "vm/result.h"
#include "vm/vm.h"

#include <dirent.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>

int main(int Argc, char *Argv[]) {
  if (Argc < 2) {
    /// Args pass to wasm config.
    /// Arg0: ./ssvm-qitc
    /// Arg1: wasm file
    /// Other args are pass into ONNC runtime
    std::cout << "Usage: ./ssvm-qitc model_wasm [args...]" << std::endl;
    return 0;
  }

  /// Open dir for WASI environment.
  /// FIXME: Don't move it! Need to refine this after completion of Wasi
  /// functions.
  std::unique_ptr<char, decltype(std::free) *> cwdstr(getcwd(NULL, 0),
                                                      std::free);
  std::unique_ptr<DIR, decltype(closedir) *> dir(opendir(cwdstr.get()),
                                                 closedir);

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
  auto FuncONNCTimeStart = std::make_unique<SSVM::Executor::QITCTimerStart>();
  auto FuncONNCTimeStop = std::make_unique<SSVM::Executor::QITCTimerStop>();
  auto FuncONNCTimeClear = std::make_unique<SSVM::Executor::QITCTimerClear>();
  VM.setHostFunction(FuncONNCTimeStart, "QITC", "QITC_time_start");
  VM.setHostFunction(FuncONNCTimeStop, "QITC", "QITC_time_stop");
  VM.setHostFunction(FuncONNCTimeClear, "QITC", "QITC_time_clear");
#endif

  VM.setPath(InputPath);
  VM.execute("_start");
  Result = VM.getResult();
  return Result.getErrCode();
}
