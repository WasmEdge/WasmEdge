// SPDX-License-Identifier: Apache-2.0
#include "common/value.h"
#include "host/wasi/wasimodule.h"
#include "po/argument_parser.h"
#include "support/filesystem.h"
#include "vm/configure.h"
#include "vm/vm.h"
#include <cstdlib>
#include <iostream>

int main(int Argc, const char *Argv[]) {
  namespace PO = SSVM::PO;
  using namespace std::literals;

  std::ios::sync_with_stdio(false);
  PO::Option<std::string> SoName(PO::Description("Wasm so file"s),
                                 PO::MetaVar("WASM_SO"s));
  PO::List<std::string> Args(PO::Description("Execution arguments"s),
                             PO::MetaVar("ARG"s));

  PO::List<std::string> Dir(
      PO::Description(
          "Binding directories into WASI virtual filesystem. Each directories "
          "can specified as --dir `host_path:guest_path`, where `guest_path` "
          "specifies the path that will correspond to `host_path` for calls "
          "like `fopen` in the guest."s),
      PO::MetaVar("PREOPEN_DIRS"s));

  if (!PO::ArgumentParser()
           .add_option(SoName)
           .add_option(Args)
           .add_option("dir", Dir)
           .parse(Argc, Argv)) {
    return 0;
  }

  std::string InputPath = std::filesystem::absolute(SoName.value()).string();
  SSVM::VM::Configure Conf;
  Conf.addVMType(SSVM::VM::Configure::VMType::Wasi);
  SSVM::VM::VM VM(Conf);

  SSVM::Host::WasiModule *WasiMod = dynamic_cast<SSVM::Host::WasiModule *>(
      VM.getImportModule(SSVM::VM::Configure::VMType::Wasi));

  WasiMod->getEnv().init(Dir.value(), SoName.value(), Args.value(),
                         {} /* Envs */);

  for (const auto &Arg: WasiMod->getEnv().getCmdArgs()) {
    std::cout << " Args : " << Arg << '\n';
  }
  for (auto It = Dir.value().begin(); It != Dir.value().end(); It++) {
    std::cout << " Dir : " << *It << '\n';
  }
  std::cout.flush();

  if (auto Result = VM.runWasmFile(InputPath, "_start")) {
    return EXIT_SUCCESS;
  } else {
    std::cout << "Failed. Error code : "
              << static_cast<uint32_t>(Result.error()) << '\n';
    return EXIT_FAILURE;
  }
}
