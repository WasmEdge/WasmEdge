// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadValue(AST::Component::Value &V) {
  // value ::= t:<valtype> len:<core:u32> v:<val(t)> => (value t v)
  //           (where len = ||v||)
  EXPECTED_TRY(loadType(V.getType()).map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Sec_Value);
  }));
  EXPECTED_TRY(uint32_t Len, FMgr.readU32().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Sec_Value);
  }));
  EXPECTED_TRY(V.getData(), FMgr.readBytes(Len).map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Sec_Value);
  }));
  return {};
}

} // namespace Loader
} // namespace WasmEdge
