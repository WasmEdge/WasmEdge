// SPDX-License-Identifier: Apache-2.0
#include "compiler/compiler.h"
#include "compiler/hostfunc.h"
#include "compiler/library.h"
#include "vm/result.h"
#include <iostream>
#include <string>
#include <vector>

#ifdef ONNC_WASM
namespace SSVM {
namespace Compiler {
class QITCTimerStart : public HostFunction {
public:
  QITCTimerStart(Library &Lib) : HostFunction(Lib) {}

  void *getFunction() override { return proxy<QITCTimerStart>(); }

  void run() {}
};

class QITCTimerStop : public HostFunction {
public:
  QITCTimerStop(Library &Lib) : HostFunction(Lib) {}

  void *getFunction() override { return proxy<QITCTimerStop>(); }

  void run() {}
};

class QITCTimerClear : public HostFunction {
public:
  QITCTimerClear(Library &Lib) : HostFunction(Lib) {}

  void *getFunction() override { return proxy<QITCTimerClear>(); }

  void run() {}
};
} // namespace Compiler
} // namespace SSVM
#endif

int main(int Argc, char *Argv[]) {
  if (Argc < 2) {
    /// Arg0: ./ssvm
    /// Arg1: wasm file
    /// Arg2...: inputs
    std::cout << "Usage: ./ssvm wasm_file.wasm [args...]" << std::endl;
    return 0;
  }

  std::string InputPath(Argv[1]);
  SSVM::VM::Configure Conf;
  Conf.addVMType(SSVM::VM::Configure::VMType::Wasi);

  SSVM::Compiler::Compiler Compiler(Conf);

  SSVM::VM::WasiEnvironment *Env =
      Compiler.getEnvironment<SSVM::VM::WasiEnvironment>(
          SSVM::VM::Configure::VMType::Wasi);
  std::vector<std::string> &CmdArgsVec = Env->getCmdArgs();
  for (int I = 1; I < Argc; I++) {
    CmdArgsVec.push_back(std::string(Argv[I]));
  }

  Compiler.setPath(InputPath);

  if (Compiler.compile() != SSVM::Compiler::ErrCode::Success) {
    return EXIT_FAILURE;
  }

  auto &Library = Compiler.getLibrary();

#ifdef ONNC_WASM
  {
    using namespace SSVM::Compiler;
    Library.setHostFunction(std::make_unique<QITCTimerStart>(Library), "QITC",
                            "QITC_time_start");
    Library.setHostFunction(std::make_unique<QITCTimerStop>(Library), "QITC",
                            "QITC_time_stop");
    Library.setHostFunction(std::make_unique<QITCTimerClear>(Library), "QITC",
                            "QITC_time_clear");
  }
#endif

  Library.execute();

  return EXIT_SUCCESS;
}
