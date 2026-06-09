// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "common/configure.h"
#include "loader/loader.h"
#include "validator/validator.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <variant>

int main(int Argc, char *Argv[]) {
  if (Argc != 2) {
    std::cerr << "Usage: " << Argv[0] << " <wasm-file>" << std::endl;
    return EXIT_FAILURE;
  }

  std::filesystem::path WasmPath(Argv[1]);

  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Component);

  WasmEdge::Loader::Loader Loader(Conf);
  auto ParseResult = Loader.parseWasmUnit(WasmPath);
  if (!ParseResult) {
    std::cerr << "Parse failed: " << WasmPath << std::endl;
    return EXIT_FAILURE;
  }

  auto &Unit = *ParseResult;
  if (auto *Comp =
          std::get_if<std::unique_ptr<WasmEdge::AST::Component::Component>>(
              &Unit)) {
    WasmEdge::Validator::Validator Validator(Conf);
    auto ValidResult = Validator.validate(**Comp);
    if (!ValidResult) {
      std::cerr << "Validation failed: " << WasmPath << std::endl;
      return EXIT_FAILURE;
    }
  } else if (auto *Mod =
                 std::get_if<std::unique_ptr<WasmEdge::AST::Module>>(&Unit)) {
    WasmEdge::Validator::Validator Validator(Conf);
    auto ValidResult = Validator.validate(**Mod);
    if (!ValidResult) {
      std::cerr << "Validation failed: " << WasmPath << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
