// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC
#include "loader/loader.h"

#include <cstdint>

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadSort(AST::Sort &Sort) {
  // sort ::= 0x00 cs:<core:sort>   => core cs
  //        | 0x01                  => func
  //        | 0x02                  => value 🪙
  //        | 0x03                  => type
  //        | 0x04                  => component
  //        | 0x05                  => instance
  auto Res = FMgr.readU32();
  if (!Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sort));
    return Unexpect(Res);
  }
  switch (*Res) {
  case 0x00:
    if (auto Res2 = loadCoreSort(Sort); !Res2) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sort));
      return Unexpect(Res2);
    }
    break;
  case 0x01:
    Sort = AST::Sort::Func;
    break;
  case 0x02:
    Sort = AST::Sort::Value;
    break;
  case 0x03:
    Sort = AST::Sort::Type;
    break;
  case 0x04:
    Sort = AST::Sort::Component;
    break;
  case 0x05:
    Sort = AST::Sort::Instance;
    break;
  default:
    return logLoadError(ErrCode::Value::MalformedSort, FMgr.getLastOffset(),
                        ASTNodeAttr::Sort);
  }

  return {};
}

Expect<void> Loader::loadCoreSort(AST::Sort &Sort) {
  // core:sort ::= 0x00     => func
  //           | 0x01       => table
  //           | 0x02       => memory
  //           | 0x03       => global
  //           | 0x10       => type
  //           | 0x11       => module
  //           | 0x12       => instance
  auto Res = FMgr.readU32();
  if (!Res) {
    return Unexpect(Res);
  }
  switch (*Res) {
  case 0x00:
    Sort = AST::Sort::CoreFunc;
    break;
  case 0x01:
    Sort = AST::Sort::CoreFunc;
    break;
  case 0x02:
    Sort = AST::Sort::CoreTable;
    break;
  case 0x03:
    Sort = AST::Sort::CoreMemory;
    break;
  case 0x10:
    Sort = AST::Sort::CoreGlobal;
    break;
  case 0x11:
    Sort = AST::Sort::CoreType;
    break;
  case 0x12:
    Sort = AST::Sort::CoreInstance;
    break;
  default:
    return logLoadError(ErrCode::Value::MalformedSort, FMgr.getLastOffset(),
                        ASTNodeAttr::Sort);
  }

  return {};
}

Expect<void> Loader::loadSortIndex(AST::SortIndex &SortIdx) {
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
Expect<void> Loader::loadCoreSortIndex(AST::SortIndex &SortIdx) {
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
