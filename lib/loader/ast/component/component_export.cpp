// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadExport(AST::Component::Export &Ex) {
  // export ::= na:<nameattributes> si:<sortidx> et?:<externtype>?
  //          => (export na si et?)

  EXPECTED_TRY(loadNameAttributes(Ex.getName(), Ex.getImplements(),
                                  Ex.getExternalIds(), Ex.getVersionSuffixes())
                   .map_error([this](auto E) {
                     return logLoadError(E, FMgr.getLastOffset(),
                                         ASTNodeAttr::Comp_Export);
                   }));
  EXPECTED_TRY(loadSortIndex(Ex.getSortIndex()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
    return E;
  }));
  EXPECTED_TRY(
      Ex.getDesc(),
      loadOption<AST::Component::Export, AST::Component::ExternDesc>(
          [this](AST::Component::ExternDesc &Desc) { return loadDesc(Desc); }));
  return {};
}

} // namespace Loader
} // namespace WasmEdge
