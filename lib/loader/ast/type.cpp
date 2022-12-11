// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "loader/loader.h"

#include <cstdint>

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadLimit(AST::Limit &Lim) {
  // Read limit.
  if (auto Res = FMgr.readByte()) {

    switch (static_cast<AST::Limit::LimitType>(*Res)) {
    case AST::Limit::LimitType::HasMin:
      Lim.setType(AST::Limit::LimitType::HasMin);
      break;
    case AST::Limit::LimitType::HasMinMax:
      Lim.setType(AST::Limit::LimitType::HasMinMax);
      break;
    case AST::Limit::LimitType::SharedNoMax:
      if (Conf.hasProposal(Proposal::Threads)) {
        return logLoadError(ErrCode::Value::SharedMemoryNoMax,
                            FMgr.getLastOffset(), ASTNodeAttr::Type_Limit);
      } else {
        return logLoadError(ErrCode::Value::IntegerTooLarge,
                            FMgr.getLastOffset(), ASTNodeAttr::Type_Limit);
      }
    case AST::Limit::LimitType::Shared:
      Lim.setType(AST::Limit::LimitType::Shared);
      break;
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

  // Read min and max number.
  if (auto Res = FMgr.readU32()) {
    Lim.setMin(*Res);
    Lim.setMax(*Res);
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Limit);
  }
  if (Lim.hasMax()) {
    if (auto Res = FMgr.readU32()) {
      Lim.setMax(*Res);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Type_Limit);
    }
  }
  return {};
}

Expect<FullValType> Loader::loadFullValType(uint8_t TypeCode) {
  switch (TypeCode) {
  case (uint8_t)NumType::I32:
    return NumType::I32;
  case (uint8_t)NumType::I64:
    return NumType::I64;
  case (uint8_t)NumType::F32:
    return NumType::F32;
  case (uint8_t)NumType::F64:
    return NumType::F64;
  case (uint8_t)NumType::V128:
    if (!Conf.hasProposal(Proposal::SIMD)) {
      return logNeedProposal(ErrCode::Value::MalformedValType, Proposal::SIMD,
                             FMgr.getLastOffset(), ASTNodeAttr::Type_ValType);
    }
    return NumType::V128;
  case (uint8_t)HeapTypeCode::Extern:
    if (!Conf.hasProposal(Proposal::ReferenceTypes)) {
      return logNeedProposal(ErrCode::Value::MalformedElemType,
                             Proposal::ReferenceTypes, FMgr.getLastOffset(),
                             ASTNodeAttr::Type_ValType);
    }
    return FullRefType(HeapTypeCode::Extern);
  case (uint8_t)HeapTypeCode::Func:
    if (!Conf.hasProposal(Proposal::ReferenceTypes) &&
        !Conf.hasProposal(Proposal::BulkMemoryOperations)) {
      return logNeedProposal(ErrCode::Value::MalformedElemType,
                             Proposal::ReferenceTypes, FMgr.getLastOffset(),
                             ASTNodeAttr::Type_ValType);
    }
    return FullRefType(HeapTypeCode::Func);
  case (uint8_t)RefTypeCode::Ref:
  case (uint8_t)RefTypeCode::RefNull:
    if (!Conf.hasProposal(Proposal::FunctionReferences)) {
      return logNeedProposal(ErrCode::Value::MalformedElemType,
                             Proposal::FunctionReferences, FMgr.getLastOffset(),
                             ASTNodeAttr::Type_ValType);
    }
    if (auto Res = loadHeapType()) {
      return FullRefType(static_cast<RefTypeCode>(TypeCode), *Res);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Type_ValType);
    }

  default:
    return logLoadError(ErrCode::Value::MalformedValType, FMgr.getLastOffset(),
                        ASTNodeAttr::Type_ValType);
  }
}

Expect<FullValType> Loader::loadFullValType() {
  if (auto Res = FMgr.readByte()) {
    return loadFullValType(*Res);
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_ValType);
  }
}

Expect<FullRefType> Loader::loadFullRefType() {
  Byte TypeCode;
  if (auto Res = FMgr.readByte()) {
    TypeCode = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_RefType);
  }
  switch (TypeCode) {
  case (uint8_t)HeapTypeCode::Extern:
  case (uint8_t)HeapTypeCode::Func:
    if (!Conf.hasProposal(Proposal::ReferenceTypes)) {
      return logNeedProposal(ErrCode::Value::MalformedElemType,
                             Proposal::ReferenceTypes, FMgr.getLastOffset(),
                             ASTNodeAttr::Type_RefType);
    }
    return static_cast<HeapTypeCode>(TypeCode);
  case (uint8_t)RefTypeCode::Ref:
  case (uint8_t)RefTypeCode::RefNull:
    if (!Conf.hasProposal(Proposal::FunctionReferences)) {
      return logNeedProposal(ErrCode::Value::MalformedElemType,
                             Proposal::FunctionReferences, FMgr.getLastOffset(),
                             ASTNodeAttr::Type_ValType);
    }
    if (auto Res = loadHeapType()) {
      return FullRefType(static_cast<RefTypeCode>(TypeCode), *Res);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Type_ValType);
    }
  default:
    if (Conf.hasProposal(Proposal::ReferenceTypes)) {
      return logLoadError(ErrCode::Value::MalformedRefType,
                          FMgr.getLastOffset(), ASTNodeAttr::Type_RefType);
    } else {
      return logLoadError(ErrCode::Value::MalformedElemType,
                          FMgr.getLastOffset(), ASTNodeAttr::Type_RefType);
    }
  }
}

Expect<HeapType> Loader::loadHeapType() {
  if (auto Res = FMgr.readS33()) {
    if (*Res >= 0) {
      return HeapType((uint32_t)*Res);
    } else {
      if (-*Res >= 0x80) {
        return logLoadError(ErrCode::Value::MalformedRefType,
                            FMgr.getLastOffset(), ASTNodeAttr::Type_RefType);
      }
      uint8_t HTypeCode = 0x80 + *Res;
      switch (HTypeCode) {
      case (uint8_t)HeapTypeCode::Func:
      case (uint8_t)HeapTypeCode::Extern:
        return HeapType(static_cast<HeapTypeCode>(HTypeCode));
      default:
        return logLoadError(ErrCode::Value::MalformedRefType,
                            FMgr.getLastOffset(), ASTNodeAttr::Type_RefType);
      }
    }
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_ValType);
  }
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
    if (auto Res = loadFullValType()) {
      FuncType.getParamTypes().push_back(*Res);
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
    if (auto Res = loadFullValType()) {
      FuncType.getReturnTypes().push_back(*Res);
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
  if (auto Res = loadFullRefType()) {
    TabType.setRefType(*Res);
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

Expect<void> Loader::loadType(AST::Table &Table) {
  if (auto TestRes = FMgr.testReadByte()) {
    if (*TestRes == 0x40) {
      if (!Conf.hasProposal(Proposal::FunctionReferences)) {
        return logNeedProposal(ErrCode::Value::MalformedTable,
                               Proposal::FunctionReferences,
                               FMgr.getLastOffset(), ASTNodeAttr::Type_Table);
      }
      // The first byte has been tested.
      FMgr.readByte();
      if (auto Res = FMgr.readByte()) {
        if (*Res != 0x00) {
          return logLoadError(ErrCode::Value::MalformedTable,
                              FMgr.getLastOffset(), ASTNodeAttr::Type_Table);
        }
      } else {
        return logLoadError(Res.error(), FMgr.getLastOffset(),
                            ASTNodeAttr::Type_Table);
      }
      if (auto Res = loadType(Table.getTableType()); !Res) {
        return logLoadError(Res.error(), FMgr.getLastOffset(),
                            ASTNodeAttr::Type_Table);
      }

      if (auto Res = loadExpression(Table.getInitExpr()); !Res) {
        return logLoadError(Res.error(), FMgr.getLastOffset(),
                            ASTNodeAttr::Type_Table);
      }
      return {};

    } else {
      if (auto Res = loadType(Table.getTableType()); !Res) {
        return logLoadError(Res.error(), FMgr.getLastOffset(),
                            ASTNodeAttr::Type_Table);
      }

      auto &Instrs = Table.getInitExpr().getInstrs();
      Instrs.clear();
      AST::Instruction Instr(OpCode::Ref__null);
      Instr.setHeapType(Table.getTableType().getRefType().getHeapType());
      Instrs.push_back(Instr);
      return {};
    }
  } else {
    return logLoadError(TestRes.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Table);
  }
}

// Load binary to construct GlobalType node. See "include/loader/loader.h".
Expect<void> Loader::loadType(AST::GlobalType &GlobType) {
  // Read value type.
  if (auto Res = loadFullValType()) {
    GlobType.setValType(*Res);
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
