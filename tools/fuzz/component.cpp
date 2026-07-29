// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "common/configure.h"
#include "common/span.h"
#include "loader/loader.h"
#include "validator/validator.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <variant>

// Fuzz the WebAssembly Component Model loader + validator on untrusted bytes.
// The Component proposal routes parseWasmUnit() through the component decoder
// (AST::Component) and component_validator.cpp, which the existing core-module
// driver fuzzer never reaches.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Component);

  WasmEdge::Loader::Loader Loader(Conf);
  auto ParseResult =
      Loader.parseWasmUnit(WasmEdge::Span<const uint8_t>(Data, Size));
  if (!ParseResult) {
    return 0;
  }

  auto &Unit = *ParseResult;
  WasmEdge::Validator::Validator Validator(Conf);
  if (auto *Comp =
          std::get_if<std::unique_ptr<WasmEdge::AST::Component::Component>>(
              &Unit)) {
    Validator.validate(**Comp);
  } else if (auto *Mod =
                 std::get_if<std::unique_ptr<WasmEdge::AST::Module>>(&Unit)) {
    Validator.validate(**Mod);
  }

  return 0;
}
