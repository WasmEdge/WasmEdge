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
  case (uint8_t)ValType::I32:
    return ValType::I32;
  case (uint8_t)ValType::I64:
    return ValType::I64;
  case (uint8_t)ValType::F32:
    return ValType::F32;
  case (uint8_t)ValType::F64:
    return ValType::F64;
  case (uint8_t)ValType::V128:
    if (!Conf.hasProposal(Proposal::SIMD)) {
      return logNeedProposal(ErrCode::Value::MalformedValType, Proposal::SIMD,
                             FMgr.getLastOffset(), ASTNodeAttr::Type_ValType);
    }
    return ValType::V128;
  case (uint8_t)ValType::ExternRef:
    if (!Conf.hasProposal(Proposal::ReferenceTypes)) {
      return logNeedProposal(ErrCode::Value::MalformedElemType,
                             Proposal::ReferenceTypes, FMgr.getLastOffset(),
                             ASTNodeAttr::Type_ValType);
    }
    return ValType::ExternRef;
  case (uint8_t)ValType::FuncRef:
    if (!Conf.hasProposal(Proposal::ReferenceTypes) &&
        !Conf.hasProposal(Proposal::BulkMemoryOperations)) {
      return logNeedProposal(ErrCode::Value::MalformedElemType,
                             Proposal::ReferenceTypes, FMgr.getLastOffset(),
                             ASTNodeAttr::Type_ValType);
    }
    return ValType::FuncRef;
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
  case (uint8_t)RefType::ExternRef:
    if (!Conf.hasProposal(Proposal::ReferenceTypes)) {
      return logNeedProposal(ErrCode::Value::MalformedElemType,
                             Proposal::ReferenceTypes, FMgr.getLastOffset(),
                             ASTNodeAttr::Type_RefType);
    }
    [[fallthrough]];
  case (uint8_t)RefType::FuncRef:
    return static_cast<RefType>(TypeCode);
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

// Load binary to construct FunctionType node. See "include/loader/loader.h".
Expect<void> Loader::loadType(AST::FunctionType &FuncType) {
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

  auto loadValTypeList = [this](std::vector<FullValType> &ValTypeList) {
    return loadVec(ValTypeList, [this](FullValType &VType) -> Expect<void> {
      if (auto Res = loadFullValType()) {
        VType = *Res;
        return {};
      } else {
        return Unexpect(Res);
      }
    });
  };

  // Read vector of parameter types.
  if (auto Res = loadValTypeList(FuncType.getParamTypes()); !Res) {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Function);
  }

  // Read vector of result types.
  if (auto Res = loadValTypeList(FuncType.getReturnTypes()); !Res) {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Function);
  }
  if (unlikely(!Conf.hasProposal(Proposal::MultiValue)) &&
      FuncType.getReturnTypes().size() > 1) {
    return logNeedProposal(ErrCode::Value::MalformedValType,
                           Proposal::MultiValue, FMgr.getLastOffset(),
                           ASTNodeAttr::Type_Function);
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
