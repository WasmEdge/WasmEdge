
// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

using namespace AST::Component;

Expect<void> Loader::loadImport(AST::Component::Import &Im) {
  if (auto Res = loadImportName(Im.getName()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Import));
    return Unexpect(Res);
  }
  if (auto Res = loadExternDesc(Im.getDesc()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Import));
    return Unexpect(Res);
  }
  return {};
}

Expect<void> Loader::loadExport(AST::Component::Export &Ex) {
  if (auto Res = loadExportName(Ex.getName()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Export));
    return Unexpect(Res);
  }
  if (auto Res = loadSortIndex(Ex.getSortIndex()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Export));
    return Unexpect(Res);
  }
  if (auto Res =
          loadOption<ExternDesc>([this](ExternDesc Desc) -> Expect<void> {
            return loadExternDesc(Desc);
          })) {
    Ex.getDesc() = *Res;
  } else {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Export));
    return Unexpect(Res);
  }
  return {};
}

} // namespace Loader
} // namespace WasmEdge
