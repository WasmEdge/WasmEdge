// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

using namespace AST::Component;

Expect<void> Loader::loadSort(Sort &Sort) {
  // sort ::= 0x00 cs:<core:sort>   => core cs
  //        | 0x01                  => func
  //        | 0x02                  => value 🪙
  //        | 0x03                  => type
  //        | 0x04                  => component
  //        | 0x05                  => instance
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sort));
    return E;
  };
  EXPECTED_TRY(auto Tag, FMgr.readByte().map_error(ReportError));
  switch (Tag) {
  case 0x00:
    return loadCoreSort(Sort.emplace<CoreSort>()).map_error(ReportError);
  case 0x01:
  case 0x02:
  case 0x03:
  case 0x04:
  case 0x05:
    Sort = static_cast<SortCase>(Tag);
    return {};
  default:
    return logLoadError(ErrCode::Value::MalformedSort, FMgr.getLastOffset(),
                        ASTNodeAttr::Sort);
  }
}

Expect<void> Loader::loadCoreSort(CoreSort &Sort) {
  // core:sort ::= 0x00     => func
  //           | 0x01       => table
  //           | 0x02       => memory
  //           | 0x03       => global
  //           | 0x10       => type
  //           | 0x11       => module
  //           | 0x12       => instance
  EXPECTED_TRY(auto B, FMgr.readByte());
  switch (B) {
  case 0x00:
  case 0x01:
  case 0x02:
  case 0x03:
  case 0x10:
  case 0x11:
  case 0x12:
    Sort = static_cast<CoreSort>(B);
    return {};
  default:
    return logLoadError(ErrCode::Value::MalformedSort, FMgr.getLastOffset(),
                        ASTNodeAttr::Sort);
  }
}

Expect<void> Loader::loadSortIndex(SortIndex<Sort> &SortIdx) {
  EXPECTED_TRY(loadSort(SortIdx.getSort()));
  EXPECTED_TRY(SortIdx.getSortIdx(), FMgr.readU32());

  return {};
}
Expect<void> Loader::loadCoreSortIndex(SortIndex<CoreSort> &SortIdx) {
  EXPECTED_TRY(loadCoreSort(SortIdx.getSort()));
  EXPECTED_TRY(SortIdx.getSortIdx(), FMgr.readU32());

  return {};
}

} // namespace Loader
} // namespace WasmEdge
