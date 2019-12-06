// SPDX-License-Identifier: Apache-2.0
#include "helper.h"
#include "onncenv.h"
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

  /// Insert helper host functions.
  SSVM::VM::ONNCEnvironment ONNCEnv;
  auto FuncONNCTimeStart =
      std::make_unique<SSVM::Executor::ONNCTimeStart>(ONNCEnv);
  auto FuncONNCTimeStop =
      std::make_unique<SSVM::Executor::ONNCTimeStop>(ONNCEnv);
  auto FuncONNCTimeClear =
      std::make_unique<SSVM::Executor::ONNCTimeClear>(ONNCEnv);
  VM.setHostFunction(FuncONNCTimeStart, "QITC", "QITC_time_start");
  VM.setHostFunction(FuncONNCTimeStart, "QITC", "QITC_time_stop");
  VM.setHostFunction(FuncONNCTimeStart, "QITC", "QITC_time_clear");

  VM.setPath(InputPath);
  VM.execute();
  Result = VM.getResult();
  return Result.getErrCode();
}
