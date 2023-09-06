// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "loader/loader.h"

#include <cstdint>

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadLimit(AST::Limit &Lim) {
  Expect<uint64_t> Min, Max;
  // Read limit.
  if (auto Res = FMgr.readByte()) {
    auto LimitType = static_cast<AST::Limit::LimitType>(*Res);
    switch (LimitType) {
    case AST::Limit::LimitType::HasMin:
    case AST::Limit::LimitType::HasMinMax:
    case AST::Limit::LimitType::Shared:
      Lim.setType(LimitType);
      goto handle32;
    case AST::Limit::LimitType::SharedNoMax:
      if (Conf.hasProposal(Proposal::Threads)) {
        return logLoadError(ErrCode::Value::SharedMemoryNoMax,
                            FMgr.getLastOffset(), ASTNodeAttr::Type_Limit);
      } else {
        return logLoadError(ErrCode::Value::IntegerTooLarge,
                            FMgr.getLastOffset(), ASTNodeAttr::Type_Limit);
      }
    case AST::Limit::LimitType::I64HasMin:
    case AST::Limit::LimitType::I64HasMinMax:
    case AST::Limit::LimitType::I64SharedNoMax:
    case AST::Limit::LimitType::I64Shared:
      Lim.setType(LimitType);
      goto handle64;
    default:
      if (*Res == 0x80 || *Res == 0x81) {
        // LEB128 cases will fail.
        return logLoadError(ErrCode::Value::IntegerTooLong,
                            FMgr.getLastOffset(), ASTNodeAttr::Type_Limit);
      } else {
        return logLoadError(ErrCode::Value::IntegerTooLarge,
                            FMgr.getLastOffset(), ASTNodeAttr::Type_Limit);
      }
    }
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Limit);
  }

handle32:
  // Read min and max number.
  if (Min = FMgr.readU32(); !Min) {
    return logLoadError(Min.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Limit);
  }
  Max = Min;
  if (Lim.hasMax() && (Max = FMgr.readU32()) && !Max) {
    return logLoadError(Max.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Limit);
  }
  goto setup;
handle64:
  // Read min and max number.
  if (Min = FMgr.readU64(); !Min) {
    return logLoadError(Min.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Limit);
  }
  Max = Min;
  if (Lim.hasMax() && (Max = FMgr.readU64()) && !Max) {
    return logLoadError(Max.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Limit);
  }
  goto setup;
setup:
  Lim.setMin(*Min);
  Lim.setMax(*Max);
  return {};
}

// Load binary to construct FunctionType node. See "include/loader/loader.h".
Expect<void> Loader::loadType(AST::FunctionType &FuncType) {
  uint32_t VecCnt = 0;

  // Read function type (0x60).
  if (auto Res = FMgr.readByte()) {
    if (*Res != 0x60U) {
      return logLoadError(ErrCode::Value::IntegerTooLong, FMgr.getLastOffset(),
                          ASTNodeAttr::Type_Function);
    }
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Function);
  }

  // Read vector of parameter types.
  if (auto Res = FMgr.readU32()) {
    VecCnt = *Res;
    if (VecCnt / 2 > FMgr.getRemainSize()) {
      return logLoadError(ErrCode::Value::IntegerTooLong, FMgr.getLastOffset(),
                          ASTNodeAttr::Type_Function);
    }
    FuncType.getParamTypes().clear();
    FuncType.getParamTypes().reserve(VecCnt);
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Function);
  }
  for (uint32_t I = 0; I < VecCnt; ++I) {
    if (auto Res = FMgr.readByte()) {
      ValType Type = static_cast<ValType>(*Res);
      if (auto Check = checkValTypeProposals(Type, FMgr.getLastOffset(),
                                             ASTNodeAttr::Type_Function);
          !Check) {
        return Unexpect(Check);
      }
      FuncType.getParamTypes().push_back(Type);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Type_Function);
    }
  }

  // Read vector of result types.
  if (auto Res = FMgr.readU32()) {
    VecCnt = *Res;
    if (VecCnt / 2 > FMgr.getRemainSize()) {
      return logLoadError(ErrCode::Value::IntegerTooLong, FMgr.getLastOffset(),
                          ASTNodeAttr::Type_Function);
    }
    FuncType.getReturnTypes().clear();
    FuncType.getReturnTypes().reserve(VecCnt);
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Function);
  }
  if (unlikely(!Conf.hasProposal(Proposal::MultiValue)) && VecCnt > 1) {
    return logNeedProposal(ErrCode::Value::MalformedValType,
                           Proposal::MultiValue, FMgr.getLastOffset(),
                           ASTNodeAttr::Type_Function);
  }
  for (uint32_t I = 0; I < VecCnt; ++I) {
    if (auto Res = FMgr.readByte()) {
      ValType Type = static_cast<ValType>(*Res);
      if (auto Check = checkValTypeProposals(Type, FMgr.getLastOffset(),
                                             ASTNodeAttr::Type_Function);
          !Check) {
        return Unexpect(Check);
      }
      FuncType.getReturnTypes().push_back(Type);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Type_Function);
    }
  }
  return {};
}

// Load binary to construct MemoryType node. See "include/loader/loader.h".
Expect<void> Loader::loadType(AST::MemoryType &MemType) {
  // Read limit.
  if (auto Res = loadLimit(MemType.getLimit()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Memory));
    return Unexpect(Res);
  }
  return {};
}

// Load binary to construct TableType node. See "include/loader/loader.h".
Expect<void> Loader::loadType(AST::TableType &TabType) {
  // Read reference type.
  if (auto Res = FMgr.readByte()) {
    TabType.setRefType(static_cast<RefType>(*Res));
    if (auto Check =
            checkRefTypeProposals(TabType.getRefType(), FMgr.getLastOffset(),
                                  ASTNodeAttr::Type_Table);
        !Check) {
      return Unexpect(Check);
    }
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Table);
  }

  // Read limit.
  if (auto Res = loadLimit(TabType.getLimit()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Table));
    return Unexpect(Res);
  }
  return {};
}

// Load binary to construct GlobalType node. See "include/loader/loader.h".
Expect<void> Loader::loadType(AST::GlobalType &GlobType) {
  // Read value type.
  if (auto Res = FMgr.readByte()) {
    GlobType.setValType(static_cast<ValType>(*Res));
    if (auto Check =
            checkValTypeProposals(GlobType.getValType(), FMgr.getLastOffset(),
                                  ASTNodeAttr::Type_Global);
        !Check) {
      return Unexpect(Check);
    }
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Global);
  }

  // Read mutability.
  if (auto Res = FMgr.readByte()) {
    GlobType.setValMut(static_cast<ValMut>(*Res));
    switch (GlobType.getValMut()) {
    case ValMut::Const:
    case ValMut::Var:
      break;
    default:
      return logLoadError(ErrCode::Value::InvalidMut, FMgr.getLastOffset(),
                          ASTNodeAttr::Type_Global);
    }
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Global);
  }
  return {};
}

} // namespace Loader
} // namespace WasmEdge
