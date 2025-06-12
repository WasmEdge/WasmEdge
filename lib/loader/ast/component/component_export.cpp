// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadExport(AST::Component::Export &Ex) {
  // export      ::= en:<exportname'> si:<sortidx> ed?:<externdesc>?
  //               => (export en si ed?)
  // exportname' ::= 0x00 len:<u32> en:<exportname>
  //               => en  (if len = |en|)

  EXPECTED_TRY(loadExternName(Ex.getName()).map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Export);
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
