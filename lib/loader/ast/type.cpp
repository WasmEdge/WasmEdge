// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "loader/loader.h"

#include <cstdint>

namespace WasmEdge {
namespace Loader {

// Load binary decode HeapType. See "include/loader/loader.h".
Expect<HeapType> Loader::loadHeapType(ASTNodeAttr From) {
  if (auto Res = FMgr.readS33()) {
    if (*Res < 0) {
      // Type index case.
      HeapTypeCode HTCode = static_cast<HeapTypeCode>(
          static_cast<uint8_t>((*Res) & INT64_C(0x7F)));
      switch (HTCode) {
      case HeapTypeCode::Extern:
        if (!Conf.hasProposal(Proposal::ReferenceTypes)) {
          return logNeedProposal(ErrCode::Value::MalformedRefType,
                                 Proposal::ReferenceTypes, FMgr.getLastOffset(),
                                 From);
        }
        [[fallthrough]];
      case HeapTypeCode::Func:
        // The FuncRef (0x70) is always allowed in the RefType even if the
        // reference-types proposal not enabled.
        return HeapType(HTCode);
      default:
        return logLoadError(Res.error(), FMgr.getLastOffset(), From);
      }
    } else {
      if (!Conf.hasProposal(Proposal::FunctionReferences)) {
        return logNeedProposal(ErrCode::Value::MalformedRefType,
                               Proposal::FunctionReferences,
                               FMgr.getLastOffset(), ASTNodeAttr::Instruction);
      }
      return HeapType(static_cast<uint32_t>(*Res));
    }
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(), From);
  }
}

// Load binary decode ValType. See "include/loader/loader.h".
Expect<ValType> Loader::loadValType(ASTNodeAttr From) {
  if (auto Res = FMgr.readByte()) {
    ValTypeCode Code = static_cast<ValTypeCode>(*Res);
    switch (Code) {
    case ValTypeCode::V128:
      if (!Conf.hasProposal(Proposal::SIMD)) {
        return logNeedProposal(ErrCode::Value::MalformedValType, Proposal::SIMD,
                               FMgr.getLastOffset(), From);
      }
      [[fallthrough]];
    case ValTypeCode::I32:
    case ValTypeCode::I64:
    case ValTypeCode::F32:
    case ValTypeCode::F64:
      return ValType(Code);
    case ValTypeCode::FuncRef:
      if (!Conf.hasProposal(Proposal::ReferenceTypes) &&
          !Conf.hasProposal(Proposal::BulkMemoryOperations)) {
        return logNeedProposal(ErrCode::Value::MalformedElemType,
                               Proposal::ReferenceTypes, FMgr.getLastOffset(),
                               From);
      }
      return ValType(Code);
    case ValTypeCode::ExternRef:
      if (!Conf.hasProposal(Proposal::ReferenceTypes)) {
        return logNeedProposal(ErrCode::Value::MalformedElemType,
                               Proposal::ReferenceTypes, FMgr.getLastOffset(),
                               From);
      }
      return ValType(Code);
    case ValTypeCode::Ref:
    case ValTypeCode::RefNull:
      if (!Conf.hasProposal(Proposal::FunctionReferences)) {
        return logNeedProposal(ErrCode::Value::MalformedValType,
                               Proposal::FunctionReferences,
                               FMgr.getLastOffset(), From);
      }
      if (auto LoadRes = loadHeapType(From)) {
        return RefType(static_cast<RefTypeCode>(Code), *LoadRes);
      } else {
        return Unexpect(LoadRes.error());
      }
    default:
      return logLoadError(ErrCode::Value::MalformedValType,
                          FMgr.getLastOffset(), From);
    }
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(), From);
  }
}

// Load binary decode RefType. See "include/loader/loader.h".
Expect<RefType> Loader::loadRefType(ASTNodeAttr From) {
  if (auto Res = FMgr.readByte()) {
    // The error code is different when the reference-types proposal turned off.
    ErrCode::Value FailCode = Conf.hasProposal(Proposal::ReferenceTypes)
                                  ? ErrCode::Value::MalformedRefType
                                  : ErrCode::Value::MalformedElemType;
    RefTypeCode Code = static_cast<RefTypeCode>(*Res);
    switch (Code) {
    case RefTypeCode::ExternRef:
      if (!Conf.hasProposal(Proposal::ReferenceTypes)) {
        return logNeedProposal(FailCode, Proposal::ReferenceTypes,
                               FMgr.getLastOffset(), From);
      }
      [[fallthrough]];
    case RefTypeCode::FuncRef:
      // The FuncRef (0x70) is always allowed in the RefType even if the
      // reference-types proposal not enabled.
      return RefType(Code);
    case RefTypeCode::Ref:
    case RefTypeCode::RefNull:
      if (!Conf.hasProposal(Proposal::FunctionReferences)) {
        return logNeedProposal(FailCode, Proposal::FunctionReferences,
                               FMgr.getLastOffset(), From);
      }
      if (auto LoadRes = loadHeapType(From)) {
        return RefType(Code, *LoadRes);
      } else {
        return Unexpect(LoadRes.error());
      }
    default:
      return logLoadError(FailCode, FMgr.getLastOffset(), From);
    }
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(), From);
  }
}

// Load binary to construct Limit node. See "include/loader/loader.h".
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
    if (auto Res = loadValType(ASTNodeAttr::Type_Function)) {
      FuncType.getParamTypes().push_back(*Res);
    } else {
      // The AST node information is handled.
      return Unexpect(Res);
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
    if (auto Res = loadValType(ASTNodeAttr::Type_Function)) {
      FuncType.getReturnTypes().push_back(*Res);
    } else {
      // The AST node information is handled.
      return Unexpect(Res);
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
  if (auto Res = loadRefType(ASTNodeAttr::Type_Table)) {
    TabType.setRefType(*Res);
  } else {
    // The AST node information is handled.
    return Unexpect(Res);
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
  if (auto Res = loadValType(ASTNodeAttr::Type_Global)) {
    GlobType.setValType(*Res);
  } else {
    // The AST node information is handled.
    return Unexpect(Res);
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
