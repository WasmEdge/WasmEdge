// SPDX-License-Identifier: Apache-2.0
#include "aot/compiler.h"
#include "loader/loader.h"
#include "po/argument_parser.h"
#include "support/filesystem.h"
#include "validator/validator.h"
#include <iostream>

int main(int Argc, const char *Argv[]) {
  namespace PO = SSVM::PO;
  using namespace std::literals;

  std::ios::sync_with_stdio(false);
  SSVM::Log::setErrorLoggingLevel();

  PO::Option<std::string> WasmName(PO::Description("Wasm file"s),
                                   PO::MetaVar("WASM"s));
  PO::Option<std::string> SoName(PO::Description("Wasm so file"s),
                                 PO::MetaVar("WASM_SO"s));

  PO::Option<PO::Toggle> DumpIR(
      PO::Description("Dump LLVM IR to `wasm.ll` and `wasm-opt.ll`."));

  if (!PO::ArgumentParser()
           .add_option(WasmName)
           .add_option(SoName)
           .add_option("dump", DumpIR)
           .parse(Argc, Argv)) {
    return 0;
  }

  std::string InputPath = std::filesystem::absolute(WasmName.value()).string();
  std::string OutputPath = std::filesystem::absolute(SoName.value()).string();
  SSVM::Loader::Loader Loader;

  std::vector<SSVM::Byte> Data;
  if (auto Res = Loader.loadFile(InputPath)) {
    Data = std::move(*Res);
  } else {
    const auto Err = static_cast<uint32_t>(Res.error());
    std::cout << "Load failed. Error code:" << Err << std::endl;
    return EXIT_FAILURE;
  }

  std::unique_ptr<SSVM::AST::Module> Module;
  if (auto Res = Loader.parseModule(Data)) {
    Module = std::move(*Res);
  } else {
    const auto Err = static_cast<uint32_t>(Res.error());
    std::cout << "Load failed. Error code:" << Err << std::endl;
    return EXIT_FAILURE;
  }

  {
    SSVM::Validator::Validator ValidatorEngine;
    if (auto Res = ValidatorEngine.validate(*Module); !Res) {
      const auto Err = static_cast<uint32_t>(Res.error());
      std::cout << "Validate failed. Error code:" << Err << std::endl;
      return EXIT_FAILURE;
    }
  }

  {
    SSVM::AOT::Compiler Compiler;
    if (DumpIR.value()) {
      Compiler.setDumpIR();
    }
    if (auto Res = Compiler.compile(Data, *Module, OutputPath); !Res) {
      const auto Err = static_cast<uint32_t>(Res.error());
      std::cout << "Compile failed. Error code:" << Err << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
