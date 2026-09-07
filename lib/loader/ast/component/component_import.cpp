// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadImport(AST::Component::Import &Im) {
  // import ::= na:<nameattributes> et:<externtype> => (import na et)

  EXPECTED_TRY(loadNameAttributes(Im.getName(), Im.getImplements(),
                                  Im.getExternalIds(), Im.getVersionSuffixes())
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
