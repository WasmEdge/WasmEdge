// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "validator/validator.h"

#include "common/errinfo.h"
#include "common/log.h"

#include <array>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace WasmEdge {
namespace Validator {

// Validate Module. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::Module &Mod) {
  // https://webassembly.github.io/spec/core/valid/modules.html
  Checker.reset(true);

  // Register type definitions into FormChecker.
  for (auto &Type : Mod.getTypeSection().getContent()) {
    Checker.addType(Type);
  }

  // Validate and register import section into FormChecker.
  if (auto Res = validate(Mod.getImportSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Import));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

  // Validate function section and register functions into FormChecker.
  if (auto Res = validate(Mod.getFunctionSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Function));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

  // Validate table section and register tables into FormChecker.
  if (auto Res = validate(Mod.getTableSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Table));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

  // Validate memory section and register memories into FormChecker.
  if (auto Res = validate(Mod.getMemorySection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Memory));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

  // Validate global section and register globals into FormChecker.
  if (auto Res = validate(Mod.getGlobalSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Global));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

  // Validate export section.
  if (auto Res = validate(Mod.getExportSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Export));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

  // Validate start section.
  if (auto Res = validate(Mod.getStartSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Start));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

  // Validate element section which initialize tables.
  if (auto Res = validate(Mod.getElementSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Element));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

  // Validate data section which initialize memories.
  if (auto Res = validate(Mod.getDataSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Data));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

  // Validate code section and expressions.
  if (auto Res = validate(Mod.getCodeSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Code));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

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

// Validate Limit type. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::Limit &Lim) {
  if (Lim.hasMax() && Lim.getMin() > Lim.getMax()) {
    spdlog::error(ErrCode::Value::InvalidLimit);
    spdlog::error(ErrInfo::InfoLimit(Lim.hasMax(), Lim.getMin(), Lim.getMax()));
    return Unexpect(ErrCode::Value::InvalidLimit);
  }
  return {};
}

// Validate Table type. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::TableType &Tab) {
  // Validate table limits.
  if (auto Res = validate(Tab.getLimit()); !Res) {
    return Unexpect(Res);
  }
  return {};
}

// Validate Memory type. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::MemoryType &Mem) {
  // Validate memory limits.
  const auto &Lim = Mem.getLimit();
  if (auto Res = validate(Lim); !Res) {
    return Unexpect(Res);
  }
  if (Lim.getMin() > LIMIT_MEMORYTYPE ||
      (Lim.hasMax() && Lim.getMax() > LIMIT_MEMORYTYPE)) {
    spdlog::error(ErrCode::Value::InvalidMemPages);
    spdlog::error(ErrInfo::InfoLimit(Lim.hasMax(), Lim.getMin(), Lim.getMax()));
    return Unexpect(ErrCode::Value::InvalidMemPages);
  }
  return {};
}

// Validate Global segment. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::GlobalSegment &GlobSeg) {
  // Check global initialization is a const expression.
  if (auto Res = validateConstExpr(GlobSeg.getExpr().getInstrs(),
                                   {GlobSeg.getGlobalType().getValType()});
      !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
    return Unexpect(Res);
  }
  return {};
}

// Validate Element segment. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ElementSegment &ElemSeg) {
  // Check initialization expressions are const expressions.
  for (auto &Expr : ElemSeg.getInitExprs()) {
    if (auto Res = validateConstExpr(Expr.getInstrs(),
                                     {ToValType(ElemSeg.getRefType())});
        !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
      return Unexpect(Res);
    }
  }

  // Passive and declarative cases are always valid with reference type.
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
    if (TableVec[ElemSeg.getIdx()] != ElemSeg.getRefType()) {
      // Reference type not matched.
      spdlog::error(ErrCode::Value::TypeCheckFailed);
      spdlog::error(ErrInfo::InfoMismatch(
          static_cast<ValType>(TableVec[ElemSeg.getIdx()]),
          static_cast<ValType>(ElemSeg.getRefType())));
      return Unexpect(ErrCode::Value::TypeCheckFailed);
    }
    // Check table initialization is a const expression.
    if (auto Res =
            validateConstExpr(ElemSeg.getExpr().getInstrs(), {ValType::I32});
        !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
      return Unexpect(Res);
    }
  }
  return {};
}

// Validate Code segment. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::CodeSegment &CodeSeg,
                                 const uint32_t TypeIdx) {
  // Reset stack in FormChecker.
  Checker.reset();
  // Add parameters into this frame.
  for (auto Val : Checker.getTypes()[TypeIdx].first) {
    Checker.addLocal(Val);
  }
  // Add locals into this frame.
  for (auto Val : CodeSeg.getLocals()) {
    for (uint32_t Cnt = 0; Cnt < Val.first; ++Cnt) {
      Checker.addLocal(Val.second);
    }
  }
  // Validate function body expression.
  if (auto Res = Checker.validate(CodeSeg.getExpr().getInstrs(),
                                  Checker.getTypes()[TypeIdx].second);
      !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
    return Unexpect(Res);
  }
  return {};
}

// Validate Data segment. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::DataSegment &DataSeg) {
  if (DataSeg.getMode() == AST::DataSegment::DataMode::Active) {
    // Check memory index in context.
    const auto &MemNum = Checker.getMemories();
    if (DataSeg.getIdx() >= MemNum) {
      spdlog::error(ErrCode::Value::InvalidMemoryIdx);
      spdlog::error(ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Memory,
                                             DataSeg.getIdx(), MemNum));
      return Unexpect(ErrCode::Value::InvalidMemoryIdx);
    }
    // Check memory initialization is a const expression.
    if (auto Res =
            validateConstExpr(DataSeg.getExpr().getInstrs(), {ValType::I32});
        !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
      return Unexpect(Res);
    }
  }
  // Passive case is always valid.
  return {};
}

// Validate Import description. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ImportDesc &ImpDesc) {
  switch (ImpDesc.getExternalType()) {
  // External type and the external content are ensured to be matched in
  // loader phase.
  case ExternalType::Function: {
    const auto TId = ImpDesc.getExternalFuncTypeIdx();
    // Function type index must exist in context.
    if (TId >= Checker.getTypes().size()) {
      spdlog::error(ErrCode::Value::InvalidFuncTypeIdx);
      spdlog::error(ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::FunctionType, TId,
          static_cast<uint32_t>(Checker.getTypes().size())));
      return Unexpect(ErrCode::Value::InvalidFuncTypeIdx);
    }
    Checker.addRef(static_cast<uint32_t>(Checker.getFunctions().size()));
    Checker.addFunc(TId, true);
    return {};
  }
  case ExternalType::Table: {
    const auto &TabType = ImpDesc.getExternalTableType();
    // Table type must be valid.
    if (auto Res = validate(TabType); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Table));
      return Unexpect(Res);
    }
    Checker.addTable(TabType);
    return {};
  }
  case ExternalType::Memory: {
    const auto &MemType = ImpDesc.getExternalMemoryType();
    // Memory type must be valid.
    if (auto Res = validate(MemType); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Memory));
      return Unexpect(Res);
    }
    Checker.addMemory(MemType);
    return {};
  }
  case ExternalType::Global:
    // Global type always is valid.
    Checker.addGlobal(ImpDesc.getExternalGlobalType(), true);
    return {};
  default:
    break;
  }
  return {};
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
    break;
  }
  return {};
}

// Validate Import section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ImportSection &ImportSec) {
  for (auto &ImportDesc : ImportSec.getContent()) {
    if (auto Res = validate(ImportDesc); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Desc_Import));
      return Unexpect(Res);
    }
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
      return Unexpect(ErrCode::Value::InvalidFuncTypeIdx);
    }
    Checker.addFunc(TId);
  }
  return {};
}

// Validate Table section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::TableSection &TabSec) {
  for (auto &Tab : TabSec.getContent()) {
    if (auto Res = validate(Tab)) {
      Checker.addTable(Tab);
    } else {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Table));
      return Unexpect(Res);
    }
  }
  return {};
}

// Validate Memory section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::MemorySection &MemSec) {
  for (auto &Mem : MemSec.getContent()) {
    if (auto Res = validate(Mem)) {
      Checker.addMemory(Mem);
    } else {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Memory));
      return Unexpect(Res);
    }
  }
  return {};
}

// Validate Global section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::GlobalSection &GlobSec) {
  for (auto &GlobSeg : GlobSec.getContent()) {
    if (auto Res = validate(GlobSeg)) {
      Checker.addGlobal(GlobSeg.getGlobalType());
    } else {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Global));
      return Unexpect(Res);
    }
  }
  return {};
}

// Validate Element section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ElementSection &ElemSec) {
  for (auto &ElemSeg : ElemSec.getContent()) {
    if (auto Res = validate(ElemSeg)) {
      Checker.addElem(ElemSeg);
    } else {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Element));
      return Unexpect(Res);
    }
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
    if (auto Res = validate(CodeVec[Id], FuncVec[TId]); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Code));
      return Unexpect(Res);
    }
  }
  return {};
}

// Validate Data section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::DataSection &DataSec) {
  for (auto &DataSeg : DataSec.getContent()) {
    if (auto Res = validate(DataSeg)) {
      Checker.addData(DataSeg);
    } else {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Data));
      return Unexpect(Res);
    }
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
    auto &Type = Checker.getTypes()[TId];
    if (Type.first.size() != 0 || Type.second.size() != 0) {
      // Start function signature should be {}->{}
      std::vector<ValType> Params, Returns;
      for (auto &V : Type.first) {
        Params.push_back(Checker.VTypeToAST(V));
      }
      for (auto &V : Type.second) {
        Returns.push_back(Checker.VTypeToAST(V));
      }
      spdlog::error(ErrCode::Value::InvalidStartFunc);
      spdlog::error(ErrInfo::InfoMismatch({}, {}, Params, Returns));
      return Unexpect(ErrCode::Value::InvalidStartFunc);
    }
  }
  return {};
}

// Validate Export section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ExportSection &ExportSec) {
  std::unordered_set<std::string> ExportNames;
  for (auto &ExportDesc : ExportSec.getContent()) {
    auto Result = ExportNames.emplace(ExportDesc.getExternalName());
    if (!Result.second) {
      // Duplicated export name.
      spdlog::error(ErrCode::Value::DupExportName);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Desc_Export));
      return Unexpect(ErrCode::Value::DupExportName);
    }
    if (auto Res = validate(ExportDesc); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Desc_Export));
      return Unexpect(Res);
    }
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
      if (GlobIdx >= Checker.getNumImportGlobals()) {
        spdlog::error(ErrCode::Value::InvalidGlobalIdx);
        spdlog::error(ErrInfo::InfoForbidIndex(
            ErrInfo::IndexCategory::Global, GlobIdx,
            static_cast<uint32_t>(Checker.getNumImportGlobals())));
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
