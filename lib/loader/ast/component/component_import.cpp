// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadImport(AST::Component::Import &Im) {
  // import      ::= in:<importname'> ed:<externdesc> => (import in ed)
  // importname' ::= 0x00 len:<u32> in:<importname>   => in  (if len = |in|)

  EXPECTED_TRY(loadExternName(Im.getName(), Im.getImplements())
                   .map_error([this](auto E) {
                     return logLoadError(E, FMgr.getLastOffset(),
                                         ASTNodeAttr::Comp_Import);
                   }));
  EXPECTED_TRY(loadDesc(Im.getDesc()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
    return E;
  }));
  return {};
}

} // namespace Loader
} // namespace WasmEdge
