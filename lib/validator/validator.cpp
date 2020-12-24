// SPDX-License-Identifier: Apache-2.0
#include "validator/validator.h"
#include "ast/module.h"
#include "common/log.h"

#include <string>
#include <unordered_set>

namespace SSVM {
namespace Validator {

/// Validate Module. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::Module &Mod) {
  /// https://webassembly.github.io/spec/core/valid/modules.html
  Checker.reset(true);

  /// Register type definitions into FormChecker.
  for (auto &Type : Mod.getTypeSection().getContent()) {
    Checker.addType(Type);
  }

  /// Validate and register import section into FormChecker.
  if (auto Res = validate(Mod.getImportSection()); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(Mod.getImportSection().NodeAttr);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(Res);
  }

  /// Validate function section and register functions into FormChecker.
  if (auto Res = validate(Mod.getFunctionSection()); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(Mod.getFunctionSection().NodeAttr);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(Res);
  }

  /// Validate table section and register tables into FormChecker.
  if (auto Res = validate(Mod.getTableSection()); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(Mod.getTableSection().NodeAttr);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(Res);
  }

  /// Validate memory section and register memories into FormChecker.
  if (auto Res = validate(Mod.getMemorySection()); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(Mod.getMemorySection().NodeAttr);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(Res);
  }

  /// Validate global section and register globals into FormChecker.
  if (auto Res = validate(Mod.getGlobalSection()); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(Mod.getGlobalSection().NodeAttr);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(Res);
  }

  /// Validate export section.
  if (auto Res = validate(Mod.getExportSection()); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(Mod.getExportSection().NodeAttr);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(Res);
  }

  /// Validate start section.
  if (auto Res = validate(Mod.getStartSection()); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(Mod.getStartSection().NodeAttr);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(Res);
  }

  /// Validate element section which initialize tables.
  if (auto Res = validate(Mod.getElementSection()); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(Mod.getElementSection().NodeAttr);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(Res);
  }

  /// Validate data section which initialize memories.
  if (auto Res = validate(Mod.getDataSection()); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(Mod.getDataSection().NodeAttr);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(Res);
  }

  /// Validate code section and expressions.
  if (auto Res = validate(Mod.getCodeSection()); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(Mod.getCodeSection().NodeAttr);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(Res);
  }

  /// Multiple tables is for ReferenceTypes proposal.
  if (Checker.getTables().size() > 1 &&
      !PConf.hasProposal(Proposal::ReferenceTypes)) {
    LOG(ERROR) << ErrCode::MultiTables;
    LOG(ERROR) << ErrInfo::InfoProposal(Proposal::ReferenceTypes);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(ErrCode::MultiTables);
  }

  /// In current version, memory must be <= 1.
  if (Checker.getMemories().size() > 1) {
    LOG(ERROR) << ErrCode::MultiMemories;
    LOG(ERROR) << ErrInfo::InfoInstanceBound(ExternalType::Memory,
                                             Checker.getMemories().size(), 1);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(ErrCode::MultiMemories);
  }
  return {};
}

/// Validate Limit type. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::Limit &Lim) {
  if (Lim.hasMax() && Lim.getMin() > Lim.getMax()) {
    LOG(ERROR) << ErrCode::InvalidLimit;
    LOG(ERROR) << ErrInfo::InfoLimit(Lim.hasMax(), Lim.getMin(), Lim.getMax());
    return Unexpect(ErrCode::InvalidLimit);
  }
  return {};
}

/// Validate Table type. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::TableType &Tab) {
  /// Validate table limits.
  if (auto Res = validate(Tab.getLimit()); !Res) {
    return Unexpect(Res);
  }
  return {};
}

/// Validate Memory type. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::MemoryType &Mem) {
  /// Validate memory limits.
  const auto &Lim = Mem.getLimit();
  if (auto Res = validate(Lim); !Res) {
    return Unexpect(Res);
  }
  if (Lim.getMin() > LIMIT_MEMORYTYPE ||
      (Lim.hasMax() && Lim.getMax() > LIMIT_MEMORYTYPE)) {
    LOG(ERROR) << ErrCode::InvalidMemPages;
    LOG(ERROR) << ErrInfo::InfoLimit(Lim.hasMax(), Lim.getMin(), Lim.getMax());
    return Unexpect(ErrCode::InvalidMemPages);
  }
  return {};
}

/// Validate Global segment. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::GlobalSegment &GlobSeg) {
  /// Check global initialization is a const expression.
  if (auto Res =
          validateConstExpr(GlobSeg.getInstrs(),
                            std::array{GlobSeg.getGlobalType().getValueType()});
      !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Expression);
    return Unexpect(Res);
  }
  return {};
}

/// Validate Element segment. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ElementSegment &ElemSeg) {
  /// Check initialization expressions are const expressions.
  for (auto &Expr : ElemSeg.getInitExprs()) {
    if (auto Res = validateConstExpr(
            Expr.getInstrs(), std::array{ToValType(ElemSeg.getRefType())});
        !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Expression);
      return Unexpect(Res);
    }
  }

  /// Passive and declarative cases are always valid with reference type.
  if (ElemSeg.getMode() == AST::ElementSegment::ElemMode::Active) {
    /// Check table index and reference type in context.
    const auto &TableVec = Checker.getTables();
    if (ElemSeg.getIdx() >= TableVec.size()) {
      LOG(ERROR) << ErrCode::InvalidTableIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Table,
                                             ElemSeg.getIdx(), TableVec.size());
      return Unexpect(ErrCode::InvalidTableIdx);
    }
    if (TableVec[ElemSeg.getIdx()] != ElemSeg.getRefType()) {
      /// Reference type not matched.
      LOG(ERROR) << ErrCode::InvalidTableIdx;
      return Unexpect(ErrCode::InvalidTableIdx);
    }
    /// Check table initialization is a const expression.
    if (auto Res =
            validateConstExpr(ElemSeg.getInstrs(), std::array{ValType::I32});
        !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Expression);
      return Unexpect(Res);
    }
  }
  return {};
}

/// Validate Code segment. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::CodeSegment &CodeSeg,
                                 const uint32_t TypeIdx) {
  /// Reset stack in FormChecker.
  Checker.reset();
  /// Add parameters into this frame.
  for (auto Val : Checker.getTypes()[TypeIdx].first) {
    Checker.addLocal(Val);
  }
  /// Add locals into this frame.
  for (auto Val : CodeSeg.getLocals()) {
    for (uint32_t Cnt = 0; Cnt < Val.first; ++Cnt) {
      Checker.addLocal(Val.second);
    }
  }
  /// Validate function body expression.
  if (auto Res = Checker.validate(CodeSeg.getInstrs(),
                                  Checker.getTypes()[TypeIdx].second);
      !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Expression);
    return Unexpect(Res);
  }
  return {};
}

/// Validate Data segment. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::DataSegment &DataSeg) {
  if (DataSeg.getMode() == AST::DataSegment::DataMode::Active) {
    /// Check memory index in context.
    const auto &MemVec = Checker.getMemories();
    if (DataSeg.getIdx() >= MemVec.size()) {
      LOG(ERROR) << ErrCode::InvalidMemoryIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Memory,
                                             DataSeg.getIdx(), MemVec.size());
      return Unexpect(ErrCode::InvalidMemoryIdx);
    }
    /// Check memory initialization is a const expression.
    if (auto Res =
            validateConstExpr(DataSeg.getInstrs(), std::array{ValType::I32});
        !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Expression);
      return Unexpect(Res);
    }
  }
  /// Passive case is always valid.
  return {};
}

/// Validate Import description. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ImportDesc &ImpDesc) {
  switch (ImpDesc.getExternalType()) {
  /// External type and the external content are ensured to be matched in
  /// loader phase.
  case ExternalType::Function: {
    const auto TId = ImpDesc.getExternalFuncTypeIdx();
    /// Function type index must exist in context.
    if (TId >= Checker.getTypes().size()) {
      LOG(ERROR) << ErrCode::InvalidFuncTypeIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::FunctionType, TId, Checker.getTypes().size());
      return Unexpect(ErrCode::InvalidFuncTypeIdx);
    }
    Checker.addRef(Checker.getFunctions().size());
    Checker.addFunc(TId, true);
    return {};
  }
  case ExternalType::Table: {
    const auto &TabType = ImpDesc.getExternalTableType();
    /// Table type must be valid.
    if (auto Res = validate(TabType); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(TabType.NodeAttr);
      return Unexpect(Res);
    }
    Checker.addTable(TabType);
    return {};
  }
  case ExternalType::Memory: {
    const auto &MemType = ImpDesc.getExternalMemoryType();
    /// Memory type must be valid.
    if (auto Res = validate(MemType); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(MemType.NodeAttr);
      return Unexpect(Res);
    }
    Checker.addMemory(MemType);
    return {};
  }
  case ExternalType::Global:
    /// Global type always is valid.
    Checker.addGlobal(ImpDesc.getExternalGlobalType(), true);
    return {};
  default:
    break;
  }
  return {};
}

/// Validate Export description. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ExportDesc &ExpDesc) {
  auto Id = ExpDesc.getExternalIndex();
  switch (ExpDesc.getExternalType()) {
  case ExternalType::Function:
    if (Id >= Checker.getFunctions().size()) {
      LOG(ERROR) << ErrCode::InvalidFuncIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Function,
                                             Id, Checker.getFunctions().size());
      return Unexpect(ErrCode::InvalidFuncIdx);
    }
    Checker.addRef(Id);
    return {};
  case ExternalType::Table:
    if (Id >= Checker.getTables().size()) {
      LOG(ERROR) << ErrCode::InvalidTableIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Table, Id,
                                             Checker.getTables().size());
      return Unexpect(ErrCode::InvalidTableIdx);
    }
    return {};
  case ExternalType::Memory:
    if (Id >= Checker.getMemories().size()) {
      LOG(ERROR) << ErrCode::InvalidMemoryIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Memory, Id,
                                             Checker.getMemories().size());
      return Unexpect(ErrCode::InvalidMemoryIdx);
    }
    return {};
  case ExternalType::Global:
    if (Id >= Checker.getGlobals().size()) {
      LOG(ERROR) << ErrCode::InvalidGlobalIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Global, Id,
                                             Checker.getGlobals().size());
      return Unexpect(ErrCode::InvalidGlobalIdx);
    }
    return {};
  default:
    break;
  }
  return {};
}

/// Validate Import section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ImportSection &ImportSec) {
  for (auto &ImportDesc : ImportSec.getContent()) {
    if (auto Res = validate(ImportDesc); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(ImportDesc.NodeAttr);
      return Unexpect(Res);
    }
  }
  return {};
}

/// Validate Function section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::FunctionSection &FuncSec) {
  const auto &FuncVec = FuncSec.getContent();
  const auto &TypeVec = Checker.getTypes();

  /// Check if type id of function is valid in context.
  for (auto &TId : FuncVec) {
    if (TId >= TypeVec.size()) {
      LOG(ERROR) << ErrCode::InvalidFuncTypeIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::FunctionType, TId, TypeVec.size());
      return Unexpect(ErrCode::InvalidFuncTypeIdx);
    }
    Checker.addFunc(TId);
  }
  return {};
}

/// Validate Table section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::TableSection &TabSec) {
  for (auto &Tab : TabSec.getContent()) {
    if (auto Res = validate(Tab)) {
      Checker.addTable(Tab);
    } else {
      LOG(ERROR) << ErrInfo::InfoAST(Tab.NodeAttr);
      return Unexpect(Res);
    }
  }
  return {};
}

/// Validate Memory section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::MemorySection &MemSec) {
  for (auto &Mem : MemSec.getContent()) {
    if (auto Res = validate(Mem)) {
      Checker.addMemory(Mem);
    } else {
      LOG(ERROR) << ErrInfo::InfoAST(Mem.NodeAttr);
      return Unexpect(Res);
    }
  }
  return {};
}

/// Validate Global section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::GlobalSection &GlobSec) {
  for (auto &GlobSeg : GlobSec.getContent()) {
    if (auto Res = validate(GlobSeg)) {
      Checker.addGlobal(GlobSeg.getGlobalType());
    } else {
      LOG(ERROR) << ErrInfo::InfoAST(GlobSeg.NodeAttr);
      return Unexpect(Res);
    }
  }
  return {};
}

/// Validate Element section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ElementSection &ElemSec) {
  for (auto &ElemSeg : ElemSec.getContent()) {
    if (auto Res = validate(ElemSeg)) {
      Checker.addElem(ElemSeg);
    } else {
      LOG(ERROR) << ErrInfo::InfoAST(ElemSeg.NodeAttr);
      return Unexpect(Res);
    }
  }
  return {};
}

/// Validate Code section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::CodeSection &CodeSec) {
  const auto &CodeVec = CodeSec.getContent();
  const auto &FuncVec = Checker.getFunctions();

  /// Validate function body.
  for (size_t Id = 0; Id < CodeVec.size(); ++Id) {
    /// Added functions contains imported functions.
    uint32_t TId = Id + Checker.getNumImportFuncs();
    if (TId >= FuncVec.size()) {
      LOG(ERROR) << ErrCode::InvalidFuncIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Function,
                                             TId, FuncVec.size());
      return Unexpect(ErrCode::InvalidFuncIdx);
    }
    if (auto Res = validate(CodeVec[Id], FuncVec[TId]); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(CodeVec[Id].NodeAttr);
      return Unexpect(Res);
    }
  }
  return {};
}

/// Validate Data section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::DataSection &DataSec) {
  for (auto &DataSeg : DataSec.getContent()) {
    if (auto Res = validate(DataSeg)) {
      Checker.addData(DataSeg);
    } else {
      LOG(ERROR) << ErrInfo::InfoAST(DataSeg.NodeAttr);
      return Unexpect(Res);
    }
  }
  return {};
}

/// Validate Start section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::StartSection &StartSec) {
  if (StartSec.getContent()) {
    auto FId = *StartSec.getContent();
    if (FId >= Checker.getFunctions().size()) {
      LOG(ERROR) << ErrCode::InvalidFuncIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::Function, FId, Checker.getFunctions().size());
      return Unexpect(ErrCode::InvalidFuncIdx);
    }
    auto TId = Checker.getFunctions()[FId];
    auto &Type = Checker.getTypes()[TId];
    if (Type.first.size() != 0 || Type.second.size() != 0) {
      /// Start function signature should be {}->{}
      std::vector<ValType> Params, Returns;
      for (auto &V : Type.first) {
        Params.push_back(Checker.VTypeToAST(V));
      }
      for (auto &V : Type.second) {
        Returns.push_back(Checker.VTypeToAST(V));
      }
      LOG(ERROR) << ErrCode::InvalidStartFunc;
      LOG(ERROR) << ErrInfo::InfoMismatch({}, {}, Params, Returns);
      return Unexpect(ErrCode::InvalidStartFunc);
    }
  }
  return {};
}

/// Validate Export section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ExportSection &ExportSec) {
  std::unordered_set<std::string> ExportNames;
  for (auto &ExportDesc : ExportSec.getContent()) {
    auto Result = ExportNames.emplace(ExportDesc.getExternalName());
    if (!Result.second) {
      /// Duplicated export name.
      LOG(ERROR) << ErrCode::DupExportName;
      LOG(ERROR) << ErrInfo::InfoAST(ExportDesc.NodeAttr);
      return Unexpect(ErrCode::DupExportName);
    }
    if (auto Res = validate(ExportDesc); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(ExportDesc.NodeAttr);
      return Unexpect(Res);
    }
  }
  return {};
}

/// Validate constant expression. See "include/validator/validator.h".
Expect<void> Validator::validateConstExpr(AST::InstrView Instrs,
                                          Span<const ValType> Returns) {
  for (auto &Instr : Instrs) {
    /// Only these 5 instructions are constant.
    switch (Instr.getOpCode()) {
    case OpCode::Global__get: {
      /// For initialization case, global indices must be imported globals.
      auto GlobIdx = Instr.getTargetIndex();
      if (GlobIdx >= Checker.getNumImportGlobals()) {
        LOG(ERROR) << ErrCode::InvalidGlobalIdx;
        LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Global,
                                               GlobIdx,
                                               Checker.getNumImportGlobals());
        LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                               Instr.getOffset());
        return Unexpect(ErrCode::InvalidGlobalIdx);
      }
      if (Checker.getGlobals()[GlobIdx].second != ValMut::Const) {
        LOG(ERROR) << ErrCode::ConstExprRequired;
        LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                               Instr.getOffset());
        return Unexpect(ErrCode::ConstExprRequired);
      }
      break;
    }
    case OpCode::Ref__func: {
      /// When in const expression, add the reference into context.
      auto FuncIdx = Instr.getTargetIndex();
      if (FuncIdx >= Checker.getFunctions().size()) {
        /// Function index out of range.
        LOG(ERROR) << ErrCode::InvalidFuncIdx;
        LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Function,
                                               FuncIdx,
                                               Checker.getFunctions().size());
        LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                               Instr.getOffset());
        return Unexpect(ErrCode::InvalidFuncIdx);
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
    default:
      LOG(ERROR) << ErrCode::ConstExprRequired;
      LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                             Instr.getOffset());
      return Unexpect(ErrCode::ConstExprRequired);
    }
  }
  /// Validate expression with result types.
  Checker.reset();
  return Checker.validate(Instrs, Returns);
}

} // namespace Validator
} // namespace SSVM
