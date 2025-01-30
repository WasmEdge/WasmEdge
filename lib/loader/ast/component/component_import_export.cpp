
// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

using namespace AST::Component;

Expect<void> Loader::loadImport(AST::Component::Import &Im) {
  return Expect<void>{}
      .and_then([&]() { return loadImportName(Im.getName()); })
      .and_then([&]() { return loadExternDesc(Im.getDesc()); })
      .map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Import));
        return E;
      });
}

Expect<void> Loader::loadExport(AST::Component::Export &Ex) {
  return Expect<void>{}
      .and_then([&]() { return loadExportName(Ex.getName()); })
      .and_then([&]() { return loadSortIndex(Ex.getSortIndex()); })
      .and_then([&]() {
        return loadOption<ExternDesc>(
            [this](ExternDesc Desc) { return loadExternDesc(Desc); });
      })
      .map([&](auto Desc) { Ex.getDesc() = std::move(Desc); })
      .map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Export));
        return E;
      });
}

} // namespace Loader
} // namespace WasmEdge
