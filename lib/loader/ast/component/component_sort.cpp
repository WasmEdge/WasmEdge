// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#include "loader/loader.h"

#include <cstdint>

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
  auto RTag = FMgr.readByte();
  if (!RTag) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sort));
    return Unexpect(RTag);
  }
  switch (*RTag) {
  case 0x00:
    if (auto Res = loadCoreSort(Sort.emplace<CoreSort>()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sort));
      return Unexpect(Res);
    }
    break;
  case 0x01:
  case 0x02:
  case 0x03:
  case 0x04:
  case 0x05:
    Sort = static_cast<SortCase>(*RTag);
    break;
  default:
    return logLoadError(ErrCode::Value::MalformedSort, FMgr.getLastOffset(),
                        ASTNodeAttr::Sort);
  }
  return {};
}

Expect<void> Loader::loadCoreSort(CoreSort &Sort) {
  // core:sort ::= 0x00     => func
  //           | 0x01       => table
  //           | 0x02       => memory
  //           | 0x03       => global
  //           | 0x10       => type
  //           | 0x11       => module
  //           | 0x12       => instance
  auto Res = FMgr.readByte();
  if (!Res) {
    return Unexpect(Res);
  }
  switch (*Res) {
  case 0x00:
  case 0x01:
  case 0x02:
  case 0x03:
  case 0x10:
  case 0x11:
  case 0x12:
    Sort = static_cast<CoreSort>(*Res);
    return {};
  default:
    return logLoadError(ErrCode::Value::MalformedSort, FMgr.getLastOffset(),
                        ASTNodeAttr::Sort);
  }
}

Expect<void> Loader::loadSortIndex(SortIndex<Sort> &SortIdx) {
  if (auto Res = loadSort(SortIdx.getSort()); !Res) {
    return Unexpect(Res);
  }
  if (auto Res = FMgr.readU32(); !Res) {
    return Unexpect(Res);
  } else {
    SortIdx.getSortIdx() = *Res;
  }

  return {};
}
Expect<void> Loader::loadCoreSortIndex(SortIndex<CoreSort> &SortIdx) {
  if (auto Res = loadCoreSort(SortIdx.getSort()); !Res) {
    return Unexpect(Res);
  }
  if (auto Res = FMgr.readU32(); !Res) {
    return Unexpect(Res);
  } else {
    SortIdx.getSortIdx() = *Res;
  }

  return {};
}

} // namespace Loader
} // namespace WasmEdge
