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
      case (uint8_t)HeapTypeCode::Any:
      case (uint8_t)HeapTypeCode::Eq:
      case (uint8_t)HeapTypeCode::I31:
      case (uint8_t)HeapTypeCode::NoFunc:
      case (uint8_t)HeapTypeCode::NoExtern:
      case (uint8_t)HeapTypeCode::Struct:
      case (uint8_t)HeapTypeCode::Array:
      case (uint8_t)HeapTypeCode::None:
        if (!Conf.hasProposal(Proposal::GC)) {
          return logNeedProposal(ErrCode::Value::MalformedRefType, Proposal::GC,
                                 FMgr.getLastOffset(),
                                 ASTNodeAttr::Type_RefType);
        }
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

Expect<void>
Loader::loadRecursiveTypeGroup(std::vector<AST::DefinedType> &Group) {
  u_int8_t OpCode = 0;

  // Read function type (0x60).
  if (auto Res = FMgr.readByte()) {
    OpCode = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Defined);
  }

  if (OpCode != (uint8_t)DefinedTypeOpCode::Func &&
      !Conf.hasProposal(Proposal::GC)) {
    return logNeedProposal(ErrCode::Value::MalformedValType, Proposal::GC,
                           FMgr.getLastOffset(), ASTNodeAttr::Type_Defined);
  }

  switch (OpCode) {
  case (uint8_t)DefinedTypeOpCode::Func: {
    AST::FunctionType FuncType;
    if (auto Res = loadType(FuncType)) {
      Group = {AST::DefinedType(std::move(FuncType))};
      return {};
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Type_Defined);
    }
  }
  case (uint8_t)DefinedTypeOpCode::Struct: {
    if (auto Res = loadStructType()) {
      Group = {AST::DefinedType(std::move(*Res))};
      return {};
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Type_Defined);
    }
  }
  case (uint8_t)DefinedTypeOpCode::Array: {
    if (auto Res = loadArrayType()) {
      Group = {AST::DefinedType(std::move(*Res))};
      return {};
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Type_Defined);
    }
  }
  case (uint8_t)DefinedTypeOpCode::Sub: {
    if (auto Res = loadSubType(false)) {
      Group = {std::move(*Res)};
      return {};
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Type_Defined);
    }
  }
  case (uint8_t)DefinedTypeOpCode::SubFinal: {
    if (auto Res = loadSubType(true)) {
      Group = {std::move(*Res)};
      return {};
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Type_Defined);
    }
  }
  case (uint8_t)DefinedTypeOpCode::Rec: {
    if (auto Res =
            loadVec(Group, [this](AST::DefinedType &Type) -> Expect<void> {
              if (auto Res = loadSubType()) {
                Type = std::move(*Res);
                return {};
              } else {
                return logLoadError(Res.error(), FMgr.getLastOffset(),
                                    ASTNodeAttr::Type_Defined);
              }
            })) {
      return {};
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Type_Defined);
    }
  }
  default: {
    return logLoadError(ErrCode::Value::IntegerTooLong, FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Defined);
  }
  }
}

Expect<AST::ArrayType> Loader::loadArrayType() {
  AST::FieldType FieldType;
  if (auto Res = loadType(FieldType); !Res) {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Array);
  }
  return AST::ArrayType(FieldType);
}

Expect<AST::StructType> Loader::loadStructType() {
  std::vector<AST::FieldType> FieldTypes;

  if (auto Res = loadVec(
          FieldTypes,
          [this](AST::FieldType &FieldType) { return loadType(FieldType); });
      !Res) {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Struct);
  }

  return AST::StructType(std::move(FieldTypes));
}

Expect<AST::DefinedType> Loader::loadSubType() {
  if (auto Res = FMgr.readByte()) {
    switch (*Res) {
    case (Byte)DefinedTypeOpCode::Sub: {
      return loadSubType(false);
    }
    case (Byte)DefinedTypeOpCode::SubFinal: {
      return loadSubType(true);
    }
    default: {
      return logLoadError(ErrCode::Value::MalformedValType,
                          FMgr.getLastOffset(), ASTNodeAttr::Type_Sub);
    }
    }
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Sub);
  }
}

Expect<AST::DefinedType> Loader::loadSubType(bool IsFinal) {
  std::vector<uint32_t> ParentIdxList;
  if (auto Res = loadVec(ParentIdxList,
                         [this](uint32_t &TypeIdx) -> Expect<void> {
                           if (auto Res = FMgr.readS33(); !Res) {
                             return Unexpect(Res.error());
                           } else {
                             TypeIdx = *Res;
                           }
                           return {};
                         });
      !Res) {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Sub);
  }

  if (auto Res = loadStructureType()) {
    return AST::DefinedType(IsFinal, std::move(ParentIdxList), std::move(*Res));
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Sub);
  }
}

Expect<AST::StructureType> Loader::loadStructureType() {
  u_int8_t OpCode = 0;

  // Read function type (0x60).
  if (auto Res = FMgr.readByte()) {
    OpCode = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Structure);
  }

  switch (OpCode) {
  case (uint8_t)DefinedTypeOpCode::Array: {
    if (auto Res = loadArrayType()) {
      return std::move(*Res);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Type_Structure);
    }
  }
  case (uint8_t)DefinedTypeOpCode::Struct: {
    if (auto Res = loadStructType(); !Res) {
      return std::move(*Res);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Type_Structure);
    }
  }
  case (uint8_t)DefinedTypeOpCode::Func: {
    AST::FunctionType FuncType;
    if (auto Res = loadType(FuncType)) {
      return FuncType;
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Type_Structure);
    }
    break;
  }
  default: {
    return logLoadError(ErrCode::Value::IntegerTooLong, FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Structure);
  }
  }
  return {};
}

Expect<void> Loader::loadType(AST::FieldType &FieldType) {
  uint8_t Mutability;
  AST::StorageType Type;

  if (auto Res = loadStorageType()) {
    Type = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Field);
  }

  if (auto Res = FMgr.readByte()) {
    Mutability = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Field);
  }

  FieldType = AST::FieldType(static_cast<ValMut>(Mutability), Type);
  return {};
}

Expect<AST::StorageType> Loader::loadStorageType() {
  Byte OpCode;
  if (auto Res = FMgr.testReadByte()) {
    OpCode = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Storage);
  }
  switch (OpCode) {
  case (Byte)PackedType::I8: {
    // we have tested read one type. Therefore, `readByte` should success;
    *FMgr.readByte();
    return AST::StorageType(PackedType::I8);
  }
  case (Byte)PackedType::I16: {
    // we have tested read one type. Therefore, `readByte` should success;
    *FMgr.readByte();
    return AST::StorageType(PackedType::I16);
  }
  default:
    if (auto Res = loadFullValType()) {
      return AST::StorageType(*Res);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Type_Storage);
    }
  }
}

// Load binary to construct FunctionType node. See "include/loader/loader.h".
Expect<void> Loader::loadType(AST::FunctionType &FuncType) {
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
      Instrs.push_back(AST::Instruction(OpCode::End));
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
