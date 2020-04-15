// SPDX-License-Identifier: Apache-2.0
#include "aot/compiler.h"
#include "loader/loader.h"
#include "support/filesystem.h"
#include <iostream>

int main(int Argc, char *Argv[]) {
  if (Argc != 3) {
    /// Arg0: ./ssvmc
    /// Arg1: wasm file
    /// Arg2: output so file
    std::cout << "Usage: ./ssvmc wasm_file.wasm output_wasm.so" << std::endl;
    return EXIT_SUCCESS;
  }

  std::string InputPath = std::filesystem::absolute(Argv[1]).string();
  std::string OutputPath = std::filesystem::absolute(Argv[2]).string();
  SSVM::Loader::Loader Loader;

  SSVM::Bytes Data;
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

  SSVM::AOT::Compiler Compiler;
  if (auto Res = Compiler.compile(Data, *Module, OutputPath); !Res) {
    const auto Err = static_cast<uint32_t>(Res.error());
    std::cout << "Compile failed. Error code:" << Err << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
