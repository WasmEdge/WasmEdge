// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadSortIndex(AST::Component::SortIndex &SortIdx,
                                   const bool IsCore) {
  // sortidx      ::= sort:<sort> idx:<u32>      => (sort idx)
  // core:sortidx ::= sort:<core:sort> idx:<u32> => (sort idx)
  if (IsCore) {
    EXPECTED_TRY(loadCoreSort(SortIdx.getSort()));
  } else {
    EXPECTED_TRY(loadSort(SortIdx.getSort()));
  }
  EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Sort);
  }));
  SortIdx.setIdx(Idx);
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
    return loadCoreSort(Sort);
  case 0x01:
  case 0x02:
  case 0x03:
  case 0x04:
  case 0x05:
    Sort.setIsCore(false);
    Sort.setSortType(static_cast<AST::Component::Sort::SortType>(Flag));
    return {};
  default:
    return logLoadError(ErrCode::Value::MalformedSort, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_Sort);
  }
}

Expect<void> Loader::loadCoreSort(AST::Component::Sort &Sort) {
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
    Sort.setIsCore(true);
    Sort.setCoreSortType(static_cast<AST::Component::Sort::CoreSortType>(Flag));
    return {};
  default:
    return logLoadError(ErrCode::Value::MalformedSort, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_Sort);
  }
}

} // namespace Loader
} // namespace WasmEdge
