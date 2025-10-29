// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

// Load binary and decode HeapType. See "include/loader/loader.h".
Expect<ValType> Loader::loadHeapType(TypeCode TC, ASTNodeAttr From) {
  // Read the heap type code.
  auto Val = FMgr.readS33();
  if (unlikely(!Val)) {
    return logLoadError(Val.error(), FMgr.getLastOffset(), From);
  }

  // The error code is different when the reference-types proposal turned off.
  ErrCode::Value FailCode = Conf.hasProposal(Proposal::ReferenceTypes)
                                ? ErrCode::Value::MalformedRefType
                                : ErrCode::Value::MalformedElemType;

  if (*Val < 0) {
    if (*Val < -64) {
      // For checking the invalid s33 value which is larger than 1 byte.
      return logLoadError(FailCode, FMgr.getLastOffset(), From);
    }
    TypeCode HTCode =
        static_cast<TypeCode>(static_cast<uint8_t>((*Val) & INT64_C(0x7F)));
    switch (HTCode) {
    case TypeCode::ExternRef:
      // For the ref.func instruction, the immediate changed to store the heap
      // type directly instead of the reference type after applying the
      // typed function reference proposal. Therefore the reference-types
      // proposal should be checked here.
      if (!Conf.hasProposal(Proposal::ReferenceTypes)) {
        return logNeedProposal(FailCode, Proposal::ReferenceTypes,
                               FMgr.getLastOffset(), From);
      }
      [[fallthrough]];
    case TypeCode::FuncRef:
      if (!Conf.hasProposal(Proposal::ReferenceTypes) &&
          !Conf.hasProposal(Proposal::BulkMemoryOperations)) {
        return logNeedProposal(FailCode, Proposal::ReferenceTypes,
                               FMgr.getLastOffset(), From);
      }
      return ValType(TC, HTCode);
    case TypeCode::NullFuncRef:
    case TypeCode::NullExternRef:
    case TypeCode::NullRef:
    case TypeCode::AnyRef:
    case TypeCode::EqRef:
    case TypeCode::I31Ref:
    case TypeCode::StructRef:
    case TypeCode::ArrayRef:
      if (!Conf.hasProposal(Proposal::GC)) {
        return logNeedProposal(FailCode, Proposal::GC, FMgr.getLastOffset(),
                               From);
      }
      return ValType(TC, HTCode);
    case TypeCode::NullExnRef:
    case TypeCode::ExnRef:
      if (!Conf.hasProposal(Proposal::ExceptionHandling)) {
        return logNeedProposal(ErrCode::Value::MalformedValType,
                               Proposal::ExceptionHandling,
                               FMgr.getLastOffset(), From);
      }
      return ValType(TC, HTCode);
    default:
      return logLoadError(FailCode, FMgr.getLastOffset(), From);
    }
  } else {
    // Type index case. Legal if the function reference proposal is enabled.
    if (!Conf.hasProposal(Proposal::FunctionReferences)) {
      return logNeedProposal(FailCode, Proposal::FunctionReferences,
                             FMgr.getLastOffset(), From);
    }
    return ValType(TC, static_cast<uint32_t>(*Val));
  }
}

// Load binary and decode RefType. See "include/loader/loader.h".
Expect<ValType> Loader::loadRefType(ASTNodeAttr From) {
  // Peek the reftype code.
  auto B = FMgr.peekByte();
  if (unlikely(!B)) {
    return logLoadError(B.error(), FMgr.getLastOffset(), From);
  }

  // The error code is different when the reference-types proposal turned off.
  ErrCode::Value FailCode = Conf.hasProposal(Proposal::ReferenceTypes)
                                ? ErrCode::Value::MalformedRefType
                                : ErrCode::Value::MalformedElemType;

  // Check the first byte for reference type cases.
  TypeCode Code = static_cast<TypeCode>(*B);
  switch (Code) {
  case TypeCode::ExternRef:
    if (!Conf.hasProposal(Proposal::ReferenceTypes)) {
      return logNeedProposal(FailCode, Proposal::ReferenceTypes,
                             FMgr.getOffset(), From);
    }
    [[fallthrough]];
  case TypeCode::FuncRef:
    // The FuncRef (0x70) is always allowed in the RefType even if the
    // reference-types proposal not enabled.
    FMgr.readByte();
    return ValType(Code);
  case TypeCode::Ref:
  case TypeCode::RefNull: {
    if (!Conf.hasProposal(Proposal::FunctionReferences)) {
      return logNeedProposal(FailCode, Proposal::FunctionReferences,
                             FMgr.getOffset(), From);
    }
    FMgr.readByte();
    return loadHeapType(Code, From);
  }
  default:
    return loadHeapType(TypeCode::RefNull, From);
  }
}

// Load binary and decode ValType. See "include/loader/loader.h".
Expect<ValType> Loader::loadValType(ASTNodeAttr From, bool IsStorageType) {
  // Peek the valtype code.
  auto B = FMgr.peekByte();
  if (unlikely(!B)) {
    return logLoadError(B.error(), FMgr.getLastOffset(), From);
  }

  // Check the first byte for value type cases.
  TypeCode Code = static_cast<TypeCode>(*B);
  switch (Code) {
  case TypeCode::V128:
    if (!Conf.hasProposal(Proposal::SIMD)) {
      return logNeedProposal(ErrCode::Value::MalformedValType, Proposal::SIMD,
                             FMgr.getLastOffset(), From);
    }
    [[fallthrough]];
  case TypeCode::I32:
  case TypeCode::I64:
  case TypeCode::F32:
  case TypeCode::F64:
    FMgr.readByte();
    return ValType(Code);
  case TypeCode::I8:
  case TypeCode::I16:
    // Packed types are for GC proposal. The proposal checking should be handled
    // in the parent scope.
    if (!IsStorageType) {
      return logLoadError(ErrCode::Value::MalformedValType, FMgr.getOffset(),
                          From);
    }
    FMgr.readByte();
    return ValType(Code);
  default:
    return loadRefType(From);
  }
}

Expect<ValMut> Loader::loadMutability(ASTNodeAttr From) {
  // Read the mutability byte.
  auto B = FMgr.readByte();
  if (unlikely(!B)) {
    return logLoadError(B.error(), FMgr.getLastOffset(), From);
  }

  // Check the mutability cases.
  switch (static_cast<ValMut>(*B)) {
  case ValMut::Const:
  case ValMut::Var:
    return static_cast<ValMut>(*B);
  default:
    return logLoadError(ErrCode::Value::InvalidMut, FMgr.getLastOffset(), From);
  }
}

Expect<void> Loader::loadFieldType(AST::FieldType &FType) {
  // The error code logging is handled.
  EXPECTED_TRY(auto Type, loadValType(ASTNodeAttr::Type_Rec, true));
  FType.setStorageType(Type);
  // The error code logging is handled.
  EXPECTED_TRY(auto Mut, loadMutability(ASTNodeAttr::Type_Rec));
  FType.setValMut(Mut);
  return {};
}

Expect<void> Loader::loadCompositeType(AST::CompositeType &CType) {
  // Read the composite type flag.
  EXPECTED_TRY(uint8_t B, FMgr.readByte().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Type_Rec);
  }));

  // Read the compiste type by cases.
  switch (static_cast<TypeCode>(B)) {
  case TypeCode::Array: {
    AST::FieldType FType;
    EXPECTED_TRY(loadFieldType(FType));
    CType.setArrayType(std::move(FType));
    return {};
  }
  case TypeCode::Struct: {
    std::vector<AST::FieldType> FList;
    EXPECTED_TRY(loadVec<AST::SubType>(
        FList, [this](AST::FieldType &FType) -> Expect<void> {
          // The error code logging is handled.
          return loadFieldType(FType);
        }));
    CType.setStructType(std::move(FList));
    return {};
  }
  case TypeCode::Func: {
    AST::FunctionType FuncType;
    EXPECTED_TRY(loadType(FuncType));
    CType.setFunctionType(std::move(FuncType));
    return {};
  }
  default:
    return logLoadError(ErrCode::Value::IntegerTooLong, FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Rec);
  }
}

// Load binary to construct Limit node. See "include/loader/loader.h".
Expect<void> Loader::loadLimit(AST::Limit &Lim) {
  // Read the limit type flag.
  EXPECTED_TRY(uint8_t B, FMgr.readByte().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Type_Limit);
  }));

  // Check the type flag with proposals.
  switch (static_cast<AST::Limit::LimitType>(B)) {
  case AST::Limit::LimitType::SharedNoMax:
  case AST::Limit::LimitType::Shared:
    if (!Conf.hasProposal(Proposal::Threads)) {
      return logLoadError(ErrCode::Value::IntegerTooLarge, FMgr.getLastOffset(),
                          ASTNodeAttr::Type_Limit);
    }
    [[fallthrough]];
  case AST::Limit::LimitType::HasMin:
  case AST::Limit::LimitType::HasMinMax:
    Lim.setType(static_cast<AST::Limit::LimitType>(B));
    break;
  default:
    if (Conf.hasProposal(Proposal::Memory64)) {
      // TODO: MEMORY64 - fully support implementation.
      // Currently add this for passing binary parsing in spec tests.
      return logLoadError(ErrCode::Value::MalformedLimitFlags,
                          FMgr.getLastOffset(), ASTNodeAttr::Type_Limit);
    } else {
      if (B == 0x80 || B == 0x81) {
        // LEB128 cases will fail.
        return logLoadError(ErrCode::Value::IntegerTooLong,
                            FMgr.getLastOffset(), ASTNodeAttr::Type_Limit);
      } else {
        return logLoadError(ErrCode::Value::IntegerTooLarge,
                            FMgr.getLastOffset(), ASTNodeAttr::Type_Limit);
      }
    }
  }

  // Read the min and max number.
  if (Conf.hasProposal(Proposal::Memory64)) {
    // TODO: MEMORY64 - fully support implementation.
    // Currently add this for passing binary parsing in spec tests.
    EXPECTED_TRY(uint64_t MinVal, FMgr.readU64().map_error([this](auto E) {
      return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Type_Limit);
    }));
    Lim.setMin(static_cast<uint32_t>(MinVal));
    if (Lim.hasMax()) {
      EXPECTED_TRY(uint64_t MaxVal, FMgr.readU64().map_error([this](auto E) {
        return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Type_Limit);
      }));
      Lim.setMax(static_cast<uint32_t>(MaxVal));
    } else {
      Lim.setMax(static_cast<uint32_t>(MinVal));
    }
  } else {
    EXPECTED_TRY(uint32_t MinVal, FMgr.readU32().map_error([this](auto E) {
      return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Type_Limit);
    }));
    Lim.setMin(MinVal);
    if (Lim.hasMax()) {
      EXPECTED_TRY(uint32_t MaxVal, FMgr.readU32().map_error([this](auto E) {
        return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Type_Limit);
      }));
      Lim.setMax(MaxVal);
    } else {
      Lim.setMax(MinVal);
    }
  }
  return {};
}

// Load binary to construct SubType node. See "include/loader/loader.h".
Expect<void> Loader::loadType(AST::SubType &SType) {
  // Pick the first byte for sub type.
  EXPECTED_TRY(uint8_t B, FMgr.peekByte().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Type_Rec);
  }));

  switch (static_cast<TypeCode>(B)) {
  default:
    // Case: comptype.
    SType.setFinal(true);
    return loadCompositeType(SType.getCompositeType());
  case TypeCode::Sub:
    // Case: 0x50 vec(typeidx) comptype.
    SType.setFinal(false);
    break;
  case TypeCode::SubFinal:
    // Case: 0x4F vec(typeidx) comptype.
    SType.setFinal(true);
    break;
  }
  FMgr.readByte();

  // Read the super type indices.
  auto LoadNum = [this](uint32_t &Idx) -> Expect<void> {
    EXPECTED_TRY(uint32_t Num, FMgr.readU32().map_error([this](auto E) {
      return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Type_Rec);
    }));
    Idx = Num;
    return {};
  };
  EXPECTED_TRY(loadVec<AST::SubType>(SType.getSuperTypeIndices(), LoadNum));

  // Read the composite type.
  return loadCompositeType(SType.getCompositeType());
}

// Load binary to construct FunctionType node. See "include/loader/loader.h".
Expect<void> Loader::loadType(AST::FunctionType &FuncType) {
  // Read type of Func (0x60). Moved into the composite type.
  auto LoadValType = [this](ValType &VT) -> Expect<void> {
    // The error code logging is handled.
    EXPECTED_TRY(VT, loadValType(ASTNodeAttr::Type_Function));
    return {};
  };
  // Read vector of parameter types.
  EXPECTED_TRY(
      loadVec<AST::FunctionType>(FuncType.getParamTypes(), LoadValType));

  // Read vector of result types.
  EXPECTED_TRY(
      loadVec<AST::FunctionType>(FuncType.getReturnTypes(), LoadValType));
  return {};
}

// Load binary to construct MemoryType node. See "include/loader/loader.h".
Expect<void> Loader::loadType(AST::MemoryType &MemType) {
  // Read limit.
  return loadLimit(MemType.getLimit()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Memory));
    return E;
  });
}

// Load binary to construct TableType node. See "include/loader/loader.h".
Expect<void> Loader::loadType(AST::TableType &TabType) {
  // Read reference type.
  // The AST node information is handled.
  EXPECTED_TRY(auto Type, loadRefType(ASTNodeAttr::Type_Table));
  TabType.setRefType(Type);

  // Read limit.
  return loadLimit(TabType.getLimit()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Table));
    return E;
  });
}

// Load binary to construct GlobalType node. See "include/loader/loader.h".
Expect<void> Loader::loadType(AST::GlobalType &GlobType) {
  // Read value type.
  // The AST node information is handled.
  EXPECTED_TRY(auto Type, loadValType(ASTNodeAttr::Type_Global));
  GlobType.setValType(Type);

  // Read mutability.
  // The AST node information is handled.
  EXPECTED_TRY(auto Mut, loadMutability(ASTNodeAttr::Type_Global));
  GlobType.setValMut(Mut);
  return {};
}

// Load binary to construct Tag node. See "include/loader/loader.h".
Expect<void> Loader::loadType(AST::TagType &TgType) {
  EXPECTED_TRY(uint8_t B, FMgr.readByte().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Type_Tag);
  }));

  // The preserved byte for future extension possibility for tag type.
  // It supports only 0x00 currently, which is for exception handling.
  if (unlikely(B != 0x00)) {
    return logLoadError(ErrCode::Value::ExpectedZeroByte, FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Tag);
  }
  EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Type_Tag);
  }));
  TgType.setTypeIdx(Idx);
  return {};
}

} // namespace Loader
} // namespace WasmEdge
