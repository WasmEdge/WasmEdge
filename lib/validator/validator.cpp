// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "validator/validator.h"

#include "ast/section.h"
#include "common/errinfo.h"
#include "common/hash.h"

#include <numeric>
#include <string>
#include <unordered_set>

using namespace std::literals;

namespace WasmEdge {
namespace Validator {

namespace {

static constexpr uint32_t MaxSubtypeDepth = 63;

Expect<void> calculateSubtypeDepthRecursiveHelper(
    uint32_t Index, uint32_t Depth, std::unordered_set<uint32_t> &VisitedSet,
    const FormChecker &Checker, uint32_t TypeIdx) {
  if (VisitedSet.count(Index)) {
    spdlog::error(ErrCode::Value::InvalidSubType);
    spdlog::error("    Cycle detected in subtype hierarchy for type {}."sv,
                  Index);
    return Unexpect(ErrCode::Value::InvalidSubType);
  }

  if (Depth >= MaxSubtypeDepth) {
    spdlog::error(ErrCode::Value::InvalidSubType);
    spdlog::error(
        "    subtype depth for Type section's {}th signature exceeded "
        "the limits of {}"sv,
        TypeIdx, MaxSubtypeDepth);
    return Unexpect(ErrCode::Value::InvalidSubType);
  }

  VisitedSet.insert(Index);
  const auto &TypeVec = Checker.getTypes();
  const auto &Type = *TypeVec[Index];
  for (const auto SuperIdx : Type.getSuperTypeIndices()) {
    EXPECTED_TRY(calculateSubtypeDepthRecursiveHelper(
                     SuperIdx, Depth + 1, VisitedSet, Checker, TypeIdx)
                     .map_error([&](auto E) {
                       spdlog::error("    When checking super type index {}."sv,
                                     SuperIdx);
                       return E;
                     }));
  }
  return {};
}

Expect<void> calculateSubtypeDepth(uint32_t TypeIdx,
                                   const FormChecker &Checker) {
  std::unordered_set<uint32_t> VisitedNodes;
  return calculateSubtypeDepthRecursiveHelper(TypeIdx, 0, VisitedNodes, Checker,
                                              TypeIdx);
}

} // namespace

// Validate Module. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::Module &Mod) {
  // https://webassembly.github.io/spec/core/valid/modules.html
  Checker.reset(true);

  // Validate and register type section.

  EXPECTED_TRY(validate(Mod.getTypeSection()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Type));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return E;
  }));

  // Validate and register import section into FormChecker.
  EXPECTED_TRY(validate(Mod.getImportSection()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Import));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return E;
  }));

  // Validate function section and register functions into FormChecker.
  EXPECTED_TRY(validate(Mod.getFunctionSection()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Function));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return E;
  }));

  // Validate table section and register tables into FormChecker.
  EXPECTED_TRY(validate(Mod.getTableSection()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Table));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return E;
  }));

  // Validate memory section and register memories into FormChecker.
  EXPECTED_TRY(validate(Mod.getMemorySection()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Memory));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return E;
  }));

  // Validate global section and register globals into FormChecker.
  EXPECTED_TRY(validate(Mod.getGlobalSection()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Global));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return E;
  }));

  // Validate tag section and register tags into FormChecker.
  EXPECTED_TRY(validate(Mod.getTagSection()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Tag));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return E;
  }));

  // Validate export section.
  EXPECTED_TRY(validate(Mod.getExportSection()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Export));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return E;
  }));

  // Validate start section.
  EXPECTED_TRY(validate(Mod.getStartSection()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Start));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return E;
  }));

  // Validate element section which initialize tables.
  EXPECTED_TRY(validate(Mod.getElementSection()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Element));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return E;
  }));

  // Validate data section which initialize memories.
  EXPECTED_TRY(validate(Mod.getDataSection()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Data));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return E;
  }));

  // Validate code section and expressions.
  EXPECTED_TRY(validate(Mod.getCodeSection()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Code));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return E;
  }));

  // Multiple tables is for the ReferenceTypes proposal.
  if (Checker.getTables().size() > 1 &&
      !Conf.hasProposal(Proposal::ReferenceTypes)) {
    spdlog::error(ErrCode::Value::MultiTables);
    spdlog::error(ErrInfo::InfoProposal(Proposal::ReferenceTypes));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(ErrCode::Value::MultiTables);
  }

  // Multiple memories is for the MultiMemories proposal.
  if (Checker.getMemories() > 1 && !Conf.hasProposal(Proposal::MultiMemories)) {
    spdlog::error(ErrCode::Value::MultiMemories);
    spdlog::error(ErrInfo::InfoProposal(Proposal::MultiMemories));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(ErrCode::Value::MultiMemories);
  }

  // Set the validated flag.
  const_cast<AST::Module &>(Mod).setIsValidated();
  return {};
}

// Validate Sub type. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::SubType &Type) {
  const auto &TypeVec = Checker.getTypes();
  const auto &CompType = Type.getCompositeType();

  // Check the validation of the composite type.
  if (CompType.isFunc()) {
    const auto &FType = CompType.getFuncType();
    for (auto &PType : FType.getParamTypes()) {
      EXPECTED_TRY(Checker.validate(PType).map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Function));
        return E;
      }));
    }
    if (unlikely(!Conf.hasProposal(Proposal::MultiValue)) &&
        FType.getReturnTypes().size() > 1) {
      spdlog::error(ErrCode::Value::InvalidResultArity);
      spdlog::error(ErrInfo::InfoProposal(Proposal::MultiValue));
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Function));
      return Unexpect(ErrCode::Value::InvalidResultArity);
    }
    for (auto &RType : FType.getReturnTypes()) {
      EXPECTED_TRY(Checker.validate(RType).map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Function));
        return E;
      }));
    }
  } else {
    const auto &FTypes = CompType.getFieldTypes();
    for (auto &FieldType : FTypes) {
      EXPECTED_TRY(Checker.validate(FieldType.getStorageType()));
    }
  }

  // In current version, the length of type index vector will be <= 1.
  if (Type.getSuperTypeIndices().size() > 1) {
    spdlog::error(ErrCode::Value::InvalidSubType);
    spdlog::error("    Accepts only one super type currently."sv);
    return Unexpect(ErrCode::Value::InvalidSubType);
  }

  for (const auto &Index : Type.getSuperTypeIndices()) {
    if (unlikely(Index >= TypeVec.size())) {
      spdlog::error(ErrCode::Value::InvalidSubType);
      spdlog::error(
          ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::DefinedType, Index,
                                   static_cast<uint32_t>(TypeVec.size())));
      return Unexpect(ErrCode::Value::InvalidSubType);
    }

    if (auto Res = calculateSubtypeDepth(Index, Checker); !Res) {
      spdlog::error("    When checking subtype hierarchy of super type {}."sv,
                    Index);
      return Unexpect(Res.error());
    }

    if (TypeVec[Index]->isFinal()) {
      spdlog::error(ErrCode::Value::InvalidSubType);
      spdlog::error("    Super type should not be final."sv);
      return Unexpect(ErrCode::Value::InvalidSubType);
    }
    auto &SuperType = TypeVec[Index]->getCompositeType();
    if (!AST::TypeMatcher::matchType(Checker.getTypes(), SuperType, CompType)) {
      spdlog::error(ErrCode::Value::InvalidSubType);
      spdlog::error("    Super type not matched."sv);
      return Unexpect(ErrCode::Value::InvalidSubType);
    }
  }
  return {};
}

// Validate Limit type. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::Limit &Lim) {
  if (Lim.hasMax() && Lim.getMin() > Lim.getMax()) {
    spdlog::error(ErrCode::Value::InvalidLimit);
    spdlog::error(ErrInfo::InfoLimit(Lim.hasMax(), Lim.getMin(), Lim.getMax()));
    return Unexpect(ErrCode::Value::InvalidLimit);
  }
  if (Lim.isShared() && unlikely(!Lim.hasMax())) {
    spdlog::error(ErrCode::Value::SharedMemoryNoMax);
    return Unexpect(ErrCode::Value::SharedMemoryNoMax);
  }
  return {};
}

// Validate Table type. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::TableType &Tab) {
  // Validate value type.
  EXPECTED_TRY(Checker.validate(Tab.getRefType()));
  // Validate table limits.
  return validate(Tab.getLimit()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Limit));
    return E;
  });
}

// Validate Memory type. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::MemoryType &Mem) {
  // Validate memory limits.
  const auto &Lim = Mem.getLimit();
  EXPECTED_TRY(validate(Lim).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Limit));
    return E;
  }));
  if (Lim.getMin() > LIMIT_MEMORYTYPE ||
      (Lim.hasMax() && Lim.getMax() > LIMIT_MEMORYTYPE)) {
    spdlog::error(ErrCode::Value::InvalidMemPages);
    spdlog::error(ErrInfo::InfoLimit(Lim.hasMax(), Lim.getMin(), Lim.getMax()));
    return Unexpect(ErrCode::Value::InvalidMemPages);
  }
  return {};
}

// Validate Global type. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::GlobalType &Glob) {
  // Validate value type.
  return Checker.validate(Glob.getValType());
}

// Validate Table segment. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::TableSegment &TabSeg) {
  if (TabSeg.getExpr().getInstrs().size() > 0) {
    // Check ref initialization is a const expression.
    EXPECTED_TRY(
        validateConstExpr(TabSeg.getExpr().getInstrs(),
                          {ValType(TabSeg.getTableType().getRefType())})
            .map_error([](auto E) {
              spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
              return E;
            }));
  } else {
    // No init expression. Check the reference type is nullable.
    if (!TabSeg.getTableType().getRefType().isNullableRefType()) {
      spdlog::error(ErrCode::Value::TypeCheckFailed);
      spdlog::error(ErrInfo::InfoMismatch(
          ValType(TypeCode::RefNull,
                  TabSeg.getTableType().getRefType().getHeapTypeCode(),
                  TabSeg.getTableType().getRefType().getTypeIndex()),
          TabSeg.getTableType().getRefType()));
      return Unexpect(ErrCode::Value::TypeCheckFailed);
    }
  }
  // Validate table type.
  return validate(TabSeg.getTableType()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Table));
    return E;
  });
}

// Validate Global segment. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::GlobalSegment &GlobSeg) {
  // Check global initialization is a const expression.
  EXPECTED_TRY(validateConstExpr(GlobSeg.getExpr().getInstrs(),
                                 {GlobSeg.getGlobalType().getValType()})
                   .map_error([](auto E) {
                     spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
                     return E;
                   }));
  // Validate global type.
  return validate(GlobSeg.getGlobalType()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Global));
    return E;
  });
}

// Validate Element segment. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ElementSegment &ElemSeg) {
  // Check initialization expressions are const expressions.
  for (auto &Expr : ElemSeg.getInitExprs()) {
    EXPECTED_TRY(
        validateConstExpr(Expr.getInstrs(), {ValType(ElemSeg.getRefType())})
            .map_error([](auto E) {
              spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
              return E;
            }));
  }

  // The reference type should be valid.
  EXPECTED_TRY(Checker.validate(ElemSeg.getRefType()));

  // Passive and declarative cases are valid with the valid reference type.
  if (ElemSeg.getMode() == AST::ElementSegment::ElemMode::Active) {
    // Check table index and reference type in context.
    const auto &TableVec = Checker.getTables();
    if (ElemSeg.getIdx() >= TableVec.size()) {
      spdlog::error(ErrCode::Value::InvalidTableIdx);
      spdlog::error(ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::Table, ElemSeg.getIdx(),
          static_cast<uint32_t>(TableVec.size())));
      return Unexpect(ErrCode::Value::InvalidTableIdx);
    }
    // TODO: Use AST::TypeMatcher::matchType() to match types instead.
    // For the element segments, the RefType may not record the strict type
    // index, and should check the init exprs for the real type index to do type
    // matching. But for the table type, the type index is recorded into the
    // heap type. So it will fail here to do strict type matching. Therefore,
    // only check the FuncRef and ExternRef and the nullable here.
    if (TableVec[ElemSeg.getIdx()].isFuncRefType() !=
            ElemSeg.getRefType().isFuncRefType() ||
        (!TableVec[ElemSeg.getIdx()].isNullableRefType() &&
         ElemSeg.getRefType().isNullableRefType())) {
      // Reference type not matched.
      spdlog::error(ErrCode::Value::TypeCheckFailed);
      spdlog::error(ErrInfo::InfoMismatch(TableVec[ElemSeg.getIdx()],
                                          ElemSeg.getRefType()));
      return Unexpect(ErrCode::Value::TypeCheckFailed);
    }
    // Check table initialization is a const expression.
    return validateConstExpr(ElemSeg.getExpr().getInstrs(),
                             {ValType(TypeCode::I32)})
        .map_error([](auto E) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
          return E;
        });
  }
  return {};
}

// Validate Code segment. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::CodeSegment &CodeSeg,
                                 const uint32_t TypeIdx) {
  // Due to the validation of the function section, the type of index bust be a
  // function type.
  const auto &FuncType =
      Checker.getTypes()[TypeIdx]->getCompositeType().getFuncType();
  // Reset stack in FormChecker.
  Checker.reset();
  // Add parameters into this frame.
  for (auto &Type : FuncType.getParamTypes()) {
    // Local passed as function parameters should be initialized.
    Checker.addLocal(Type, true);
  }
  // Add locals into this frame.
  for (auto Val : CodeSeg.getLocals()) {
    for (uint32_t Cnt = 0; Cnt < Val.first; ++Cnt) {
      // The local value type should be valid.
      EXPECTED_TRY(Checker.validate(Val.second));
      Checker.addLocal(Val.second, false);
    }
  }
  // Validate function body expression.
  return Checker
      .validate(CodeSeg.getExpr().getInstrs(), FuncType.getReturnTypes())
      .map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
        return E;
      });
}

// Validate Data segment. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::DataSegment &DataSeg) {
  switch (DataSeg.getMode()) {
  case AST::DataSegment::DataMode::Active: {
    // Check memory index in context.
    const auto &MemNum = Checker.getMemories();
    if (DataSeg.getIdx() >= MemNum) {
      spdlog::error(ErrCode::Value::InvalidMemoryIdx);
      spdlog::error(ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Memory,
                                             DataSeg.getIdx(), MemNum));
      return Unexpect(ErrCode::Value::InvalidMemoryIdx);
    }
    // Check memory initialization is a const expression.
    return validateConstExpr(DataSeg.getExpr().getInstrs(),
                             {ValType(TypeCode::I32)})
        .map_error([](auto E) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
          return E;
        });
  }
  case AST::DataSegment::DataMode::Passive:
    // Passive case is always valid.
    return {};
  default:
    return {};
  }
}

// Validate Import description. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ImportDesc &ImpDesc) {
  switch (ImpDesc.getExternalType()) {
  // External type and the external content are ensured to be matched in
  // loader phase.
  case ExternalType::Function: {
    const auto TId = ImpDesc.getExternalFuncTypeIdx();
    // Function type index must exist in context and be valid.
    if (TId >= Checker.getTypes().size()) {
      spdlog::error(ErrCode::Value::InvalidFuncTypeIdx);
      spdlog::error(ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::FunctionType, TId,
          static_cast<uint32_t>(Checker.getTypes().size())));
      return Unexpect(ErrCode::Value::InvalidFuncTypeIdx);
    }
    if (!Checker.getTypes()[TId]->getCompositeType().isFunc()) {
      spdlog::error(ErrCode::Value::InvalidFuncTypeIdx);
      spdlog::error("    Defined type index {} is not a function type."sv, TId);
      return Unexpect(ErrCode::Value::InvalidFuncTypeIdx);
    }
    Checker.addRef(static_cast<uint32_t>(Checker.getFunctions().size()));
    Checker.addFunc(TId, true);
    return {};
  }
  case ExternalType::Table: {
    const auto &TabType = ImpDesc.getExternalTableType();
    // Table type must be valid.
    EXPECTED_TRY(validate(TabType).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Table));
      return E;
    }));
    Checker.addTable(TabType);
    return {};
  }
  case ExternalType::Memory: {
    const auto &MemType = ImpDesc.getExternalMemoryType();
    // Memory type must be valid.
    EXPECTED_TRY(validate(MemType).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Memory));
      return E;
    }));
    Checker.addMemory(MemType);
    return {};
  }
  case ExternalType::Tag: {
    const auto &T = ImpDesc.getExternalTagType();
    // Tag type index must exist in context.
    auto TagTypeIdx = T.getTypeIdx();
    if (TagTypeIdx >= Checker.getTypes().size()) {
      spdlog::error(ErrCode::Value::InvalidTagIdx);
      spdlog::error(ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::TagType, TagTypeIdx,
          static_cast<uint32_t>(Checker.getTypes().size())));
      return Unexpect(ErrCode::Value::InvalidTagIdx);
    }
    Checker.addTag(TagTypeIdx);
    return {};
  }
  case ExternalType::Global: {
    const auto &GlobType = ImpDesc.getExternalGlobalType();
    // Global type must be valid.
    EXPECTED_TRY(validate(GlobType).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Global));
      return E;
    }));
    Checker.addGlobal(GlobType, true);
    return {};
  }
  default:
    return {};
  }
}

// Validate Export description. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ExportDesc &ExpDesc) {
  auto Id = ExpDesc.getExternalIndex();
  switch (ExpDesc.getExternalType()) {
  case ExternalType::Function:
    if (Id >= Checker.getFunctions().size()) {
      spdlog::error(ErrCode::Value::InvalidFuncIdx);
      spdlog::error(ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::Function, Id,
          static_cast<uint32_t>(Checker.getFunctions().size())));
      return Unexpect(ErrCode::Value::InvalidFuncIdx);
    }
    Checker.addRef(Id);
    return {};
  case ExternalType::Table:
    if (Id >= Checker.getTables().size()) {
      spdlog::error(ErrCode::Value::InvalidTableIdx);
      spdlog::error(ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::Table, Id,
          static_cast<uint32_t>(Checker.getTables().size())));
      return Unexpect(ErrCode::Value::InvalidTableIdx);
    }
    return {};
  case ExternalType::Memory:
    if (Id >= Checker.getMemories()) {
      spdlog::error(ErrCode::Value::InvalidMemoryIdx);
      spdlog::error(ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Memory, Id,
                                             Checker.getMemories()));
      return Unexpect(ErrCode::Value::InvalidMemoryIdx);
    }
    return {};
  case ExternalType::Tag:
    if (Id >= Checker.getTags().size()) {
      spdlog::error(ErrCode::Value::InvalidTagIdx);
      spdlog::error(ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::Tag, Id,
          static_cast<uint32_t>(Checker.getTags().size())));
      return Unexpect(ErrCode::Value::InvalidTagIdx);
    }
    return {};
  case ExternalType::Global:
    if (Id >= Checker.getGlobals().size()) {
      spdlog::error(ErrCode::Value::InvalidGlobalIdx);
      spdlog::error(ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::Global, Id,
          static_cast<uint32_t>(Checker.getGlobals().size())));
      return Unexpect(ErrCode::Value::InvalidGlobalIdx);
    }
    return {};
  default:
    return {};
  }
}

Expect<void> Validator::validate(const AST::TypeSection &TypeSec) {
  const auto STypeList = TypeSec.getContent();
  uint32_t Idx = 0;
  while (Idx < STypeList.size()) {
    const auto &SType = STypeList[Idx];
    if (SType.getRecursiveInfo().has_value()) {
      // Recursive type case. Add types first for referring recursively.
      uint32_t RecSize = SType.getRecursiveInfo()->RecTypeSize;
      for (uint32_t I = Idx; I < Idx + RecSize; I++) {
        Checker.addType(STypeList[I]);
      }
      for (uint32_t I = Idx; I < Idx + RecSize; I++) {
        EXPECTED_TRY(validate(STypeList[I]).map_error([](auto E) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Rec));
          return E;
        }));
      }
      Idx += RecSize;
    } else {
      // SubType case.
      if (Conf.hasProposal(Proposal::GC)) {
        // For the GC proposal, the subtype is seemed as a self-recursive type.
        // Add types first for referring recursively.
        Checker.addType(SType);
        EXPECTED_TRY(validate(*Checker.getTypes().back()));
      } else {
        // Validating first.
        EXPECTED_TRY(validate(SType));
        Checker.addType(SType);
      }
      Idx++;
    }
  }
  return {};
}

// Validate Import section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ImportSection &ImportSec) {
  for (auto &ImportDesc : ImportSec.getContent()) {
    EXPECTED_TRY(validate(ImportDesc).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Desc_Import));
      return E;
    }));
  }
  return {};
}

// Validate Function section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::FunctionSection &FuncSec) {
  const auto &FuncVec = FuncSec.getContent();
  const auto &TypeVec = Checker.getTypes();

  // Check if type id of function is valid in context.
  for (auto &TId : FuncVec) {
    if (TId >= TypeVec.size()) {
      spdlog::error(ErrCode::Value::InvalidFuncTypeIdx);
      spdlog::error(
          ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::FunctionType, TId,
                                   static_cast<uint32_t>(TypeVec.size())));
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Function));
      return Unexpect(ErrCode::Value::InvalidFuncTypeIdx);
    }
    if (!TypeVec[TId]->getCompositeType().isFunc()) {
      spdlog::error(ErrCode::Value::InvalidFuncTypeIdx);
      spdlog::error("    Defined type index {} is not a function type."sv, TId);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Function));
      return Unexpect(ErrCode::Value::InvalidFuncTypeIdx);
    }
    Checker.addFunc(TId);
  }
  return {};
}

// Validate Table section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::TableSection &TabSec) {
  for (auto &Tab : TabSec.getContent()) {
    EXPECTED_TRY(validate(Tab).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Table));
      return E;
    }));
    Checker.addTable(Tab.getTableType());
  }
  return {};
}

// Validate Memory section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::MemorySection &MemSec) {
  for (auto &Mem : MemSec.getContent()) {
    EXPECTED_TRY(validate(Mem).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Memory));
      return E;
    }));
    Checker.addMemory(Mem);
  }
  return {};
}

// Validate Global section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::GlobalSection &GlobSec) {
  for (auto &GlobSeg : GlobSec.getContent()) {
    EXPECTED_TRY(validate(GlobSeg).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Global));
      return E;
    }));
    Checker.addGlobal(GlobSeg.getGlobalType());
  }
  return {};
}

// Validate Element section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ElementSection &ElemSec) {
  for (auto &ElemSeg : ElemSec.getContent()) {
    EXPECTED_TRY(validate(ElemSeg).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Element));
      return E;
    }));
    Checker.addElem(ElemSeg);
  }
  return {};
}

// Validate Code section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::CodeSection &CodeSec) {
  const auto &CodeVec = CodeSec.getContent();
  const auto &FuncVec = Checker.getFunctions();

  // Validate function body.
  for (uint32_t Id = 0; Id < static_cast<uint32_t>(CodeVec.size()); ++Id) {
    // Added functions contains imported functions.
    uint32_t TId = Id + static_cast<uint32_t>(Checker.getNumImportFuncs());
    if (TId >= static_cast<uint32_t>(FuncVec.size())) {
      spdlog::error(ErrCode::Value::InvalidFuncIdx);
      spdlog::error(
          ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Function, TId,
                                   static_cast<uint32_t>(FuncVec.size())));
      return Unexpect(ErrCode::Value::InvalidFuncIdx);
    }
    EXPECTED_TRY(validate(CodeVec[Id], FuncVec[TId]).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Code));
      return E;
    }));
  }
  return {};
}

// Validate Data section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::DataSection &DataSec) {
  for (auto &DataSeg : DataSec.getContent()) {
    EXPECTED_TRY(validate(DataSeg).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Data));
      return E;
    }));
    Checker.addData(DataSeg);
  }
  return {};
}

// Validate Start section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::StartSection &StartSec) {
  if (StartSec.getContent()) {
    auto FId = *StartSec.getContent();
    if (FId >= Checker.getFunctions().size()) {
      spdlog::error(ErrCode::Value::InvalidFuncIdx);
      spdlog::error(ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::Function, FId,
          static_cast<uint32_t>(Checker.getFunctions().size())));
      return Unexpect(ErrCode::Value::InvalidFuncIdx);
    }
    auto TId = Checker.getFunctions()[FId];
    assuming(TId < Checker.getTypes().size());
    if (!Checker.getTypes()[TId]->getCompositeType().isFunc()) {
      spdlog::error(ErrCode::Value::InvalidStartFunc);
      spdlog::error("    Defined type index {} is not a function type."sv, TId);
      return Unexpect(ErrCode::Value::InvalidStartFunc);
    }
    auto &Type = Checker.getTypes()[TId]->getCompositeType().getFuncType();
    if (Type.getParamTypes().size() != 0 || Type.getReturnTypes().size() != 0) {
      // Start function signature should be {}->{}
      spdlog::error(ErrCode::Value::InvalidStartFunc);
      spdlog::error(ErrInfo::InfoMismatch({}, {}, Type.getParamTypes(),
                                          Type.getReturnTypes()));
      return Unexpect(ErrCode::Value::InvalidStartFunc);
    }
  }
  return {};
}

// Validate Export section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ExportSection &ExportSec) {
  std::unordered_set<std::string_view, Hash::Hash> ExportNames;
  for (auto &ExportDesc : ExportSec.getContent()) {
    auto Result = ExportNames.emplace(ExportDesc.getExternalName());
    if (!Result.second) {
      // Duplicated export name.
      spdlog::error(ErrCode::Value::DupExportName);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Desc_Export));
      return Unexpect(ErrCode::Value::DupExportName);
    }
    EXPECTED_TRY(validate(ExportDesc).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Desc_Export));
      return E;
    }));
  }
  return {};
}

// Validate Tag section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::TagSection &TagSec) {
  const auto &TagVec = TagSec.getContent();
  const auto &TypeVec = Checker.getTypes();

  // Check if type id of tag is valid in context.
  for (auto &TagType : TagVec) {
    auto TagTypeIdx = TagType.getTypeIdx();
    if (TagTypeIdx >= TypeVec.size()) {
      spdlog::error(ErrCode::Value::InvalidTagIdx);
      spdlog::error(
          ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::TagType, TagTypeIdx,
                                   static_cast<uint32_t>(TypeVec.size())));
      return Unexpect(ErrCode::Value::InvalidTagIdx);
    }
    auto &CompType = TypeVec[TagTypeIdx]->getCompositeType();
    if (!CompType.isFunc()) {
      spdlog::error(ErrCode::Value::InvalidTagIdx);
      spdlog::error("    Defined type index {} is not a function type."sv,
                    TagTypeIdx);
      return Unexpect(ErrCode::Value::InvalidTagIdx);
    }
    if (!CompType.getFuncType().getReturnTypes().empty()) {
      spdlog::error(ErrCode::Value::InvalidTagResultType);
      return Unexpect(ErrCode::Value::InvalidTagResultType);
    }
    Checker.addTag(TagTypeIdx);
  }
  return {};
}

// Validate constant expression. See "include/validator/validator.h".
Expect<void> Validator::validateConstExpr(AST::InstrView Instrs,
                                          Span<const ValType> Returns) {
  for (auto &Instr : Instrs) {
    // Only these instructions are accepted.
    switch (Instr.getOpCode()) {
    case OpCode::Global__get: {
      // For initialization case, global indices must be imported globals.
      auto GlobIdx = Instr.getTargetIndex();
      uint32_t ValidGlobalSize = Checker.getNumImportGlobals();
      if (Conf.hasProposal(Proposal::FunctionReferences)) {
        ValidGlobalSize = static_cast<uint32_t>(Checker.getGlobals().size());
      }
      if (GlobIdx >= ValidGlobalSize) {
        spdlog::error(ErrCode::Value::InvalidGlobalIdx);
        spdlog::error(ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Global,
                                               GlobIdx, ValidGlobalSize));
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return Unexpect(ErrCode::Value::InvalidGlobalIdx);
      }
      if (Checker.getGlobals()[GlobIdx].second != ValMut::Const) {
        spdlog::error(ErrCode::Value::ConstExprRequired);
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return Unexpect(ErrCode::Value::ConstExprRequired);
      }
      break;
    }
    case OpCode::Ref__func: {
      // When in const expression, add the reference into context.
      auto FuncIdx = Instr.getTargetIndex();
      if (FuncIdx >= Checker.getFunctions().size()) {
        // Function index out of range.
        spdlog::error(ErrCode::Value::InvalidFuncIdx);
        spdlog::error(ErrInfo::InfoForbidIndex(
            ErrInfo::IndexCategory::Function, FuncIdx,
            static_cast<uint32_t>(Checker.getFunctions().size())));
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return Unexpect(ErrCode::Value::InvalidFuncIdx);
      }
      Checker.addRef(Instr.getTargetIndex());
      break;
    }
    case OpCode::I32__const:
    case OpCode::I64__const:
    case OpCode::F32__const:
    case OpCode::F64__const:
    case OpCode::Ref__null:
    case OpCode::V128__const:
    case OpCode::End:
    case OpCode::Struct__new:
    case OpCode::Struct__new_default:
    case OpCode::Array__new:
    case OpCode::Array__new_default:
    case OpCode::Array__new_fixed:
    case OpCode::Any__convert_extern:
    case OpCode::Extern__convert_any:
    case OpCode::Ref__i31:
      break;

    // For the Extended-const proposal, these instructions are accepted.
    case OpCode::I32__add:
    case OpCode::I32__sub:
    case OpCode::I32__mul:
    case OpCode::I64__add:
    case OpCode::I64__sub:
    case OpCode::I64__mul:
      if (Conf.hasProposal(Proposal::ExtendedConst)) {
        break;
      }
      spdlog::error(ErrCode::Value::ConstExprRequired);
      spdlog::error(ErrInfo::InfoProposal(Proposal::ExtendedConst));
      spdlog::error(
          ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
      return Unexpect(ErrCode::Value::ConstExprRequired);

    default:
      spdlog::error(ErrCode::Value::ConstExprRequired);
      spdlog::error(
          ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
      return Unexpect(ErrCode::Value::ConstExprRequired);
    }
  }
  // Validate expression with result types.
  Checker.reset();
  return Checker.validate(Instrs, Returns);
}

} // namespace Validator
} // namespace WasmEdge
