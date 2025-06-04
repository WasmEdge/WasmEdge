// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadCoreSortIndex(
    AST::Component::SortIndex<AST::Component::CoreSort> &SortIdx) {
  // core:sortidx ::= sort:<core:sort> idx:<u32> => (sort idx)
  EXPECTED_TRY(loadCoreSort(SortIdx.getSort()));
  EXPECTED_TRY(SortIdx.getSortIdx(), FMgr.readU32().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Sort);
  }));
  return {};
}

Expect<void> Loader::loadCoreSort(AST::Component::CoreSort &Sort) {
  // core:sort ::= 0x00 => func
  //             | 0x01 => table
  //             | 0x02 => memory
  //             | 0x03 => global
  //             | 0x10 => type
  //             | 0x11 => module
  //             | 0x12 => instance
  EXPECTED_TRY(auto Flag, FMgr.readByte().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Sort);
  }));
  switch (Flag) {
  case 0x00:
  case 0x01:
  case 0x02:
  case 0x03:
  case 0x10:
  case 0x11:
  case 0x12:
    Sort = static_cast<AST::Component::CoreSort>(Flag);
    return {};
  default:
    return logLoadError(ErrCode::Value::MalformedSort, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_Sort);
  }
}

Expect<void> Loader::loadSortIndex(
    AST::Component::SortIndex<AST::Component::Sort> &SortIdx) {
  // sortidx ::= sort:<sort> idx:<u32> => (sort idx)
  EXPECTED_TRY(loadSort(SortIdx.getSort()));
  EXPECTED_TRY(SortIdx.getSortIdx(), FMgr.readU32().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Sort);
  }));
  return {};
}

Expect<void> Loader::loadSort(AST::Component::Sort &Sort) {
  // sort ::= 0x00 cs:<core:sort> => core cs
  //        | 0x01                => func
  //        | 0x02                => value 🪙
  //        | 0x03                => type
  //        | 0x04                => component
  //        | 0x05                => instance
  EXPECTED_TRY(auto Flag, FMgr.readByte().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Sort);
  }));
  switch (Flag) {
  case 0x00:
    return loadCoreSort(Sort.emplace<AST::Component::CoreSort>());
  case 0x01:
  case 0x02:
  case 0x03:
  case 0x04:
  case 0x05:
    Sort = static_cast<AST::Component::SortCase>(Flag);
    return {};
  default:
    return logLoadError(ErrCode::Value::MalformedSort, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_Sort);
  }
}

} // namespace Loader
} // namespace WasmEdge
