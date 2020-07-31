// SPDX-License-Identifier: Apache-2.0
#include "validator/validator.h"
#include "common/ast/module.h"
#include "support/log.h"

#include <string>
#include <unordered_set>

namespace SSVM {
namespace Validator {

/// Validate Module. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::Module &Mod) {
  /// https://webassembly.github.io/spec/core/valid/modules.html
  Checker.reset(true);

  /// Register type definitions into FormChecker.
  if (Mod.getTypeSection()) {
    for (auto &Type : Mod.getTypeSection()->getContent()) {
      Checker.addType(*Type.get());
    }
  }

  /// Validate and register import section into FormChecker.
  if (Mod.getImportSection() != nullptr) {
    if (auto Res = validate(*Mod.getImportSection()); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(Mod.getImportSection()->NodeAttr);
      LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
      return Unexpect(Res);
    }
  }

  /// Validate function section and register functions into FormChecker.
  if (Mod.getFunctionSection() != nullptr) {
    if (auto Res = validate(*Mod.getFunctionSection()); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(Mod.getFunctionSection()->NodeAttr);
      LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
      return Unexpect(Res);
    }
  }

  /// Validate table section and register tables into FormChecker.
  if (Mod.getTableSection() != nullptr) {
    if (auto Res = validate(*Mod.getTableSection()); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(Mod.getTableSection()->NodeAttr);
      LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
      return Unexpect(Res);
    }
  }

  /// Validate memory section and register memories into FormChecker.
  if (Mod.getMemorySection() != nullptr) {
    if (auto Res = validate(*Mod.getMemorySection()); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(Mod.getMemorySection()->NodeAttr);
      LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
      return Unexpect(Res);
    }
  }

  /// Validate global section and register globals into FormChecker.
  if (Mod.getGlobalSection() != nullptr) {
    if (auto Res = validate(*Mod.getGlobalSection()); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(Mod.getGlobalSection()->NodeAttr);
      LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
      return Unexpect(Res);
    }
  }

  /// Validate element section which initialize tables.
  if (Mod.getElementSection() != nullptr) {
    if (auto Res = validate(*Mod.getElementSection()); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(Mod.getElementSection()->NodeAttr);
      LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
      return Unexpect(Res);
    }
  }

  /// Validate code section and expressions.
  if (Mod.getCodeSection() != nullptr) {
    if (auto Res = validate(*Mod.getCodeSection()); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(Mod.getCodeSection()->NodeAttr);
      LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
      return Unexpect(Res);
    }
  }

  /// Validate data section which initialize memories.
  if (Mod.getDataSection() != nullptr) {
    if (auto Res = validate(*Mod.getDataSection()); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(Mod.getDataSection()->NodeAttr);
      LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
      return Unexpect(Res);
    }
  }

  /// Validate start section.
  if (Mod.getStartSection() != nullptr) {
    if (auto Res = validate(*Mod.getStartSection()); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(Mod.getStartSection()->NodeAttr);
      LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
      return Unexpect(Res);
    }
  }

  /// Validate export section.
  if (Mod.getExportSection() != nullptr) {
    if (auto Res = validate(*Mod.getExportSection()); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(Mod.getExportSection()->NodeAttr);
      LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
      return Unexpect(Res);
    }
  }

  /// In current version, memory and table must be <= 1.
  if (Checker.getTables().size() > 1) {
    LOG(ERROR) << ErrCode::MultiTables;
    LOG(ERROR) << ErrInfo::InfoInstanceBound(ExternalType::Table,
                                             Checker.getTables().size(), 1);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(ErrCode::MultiTables);
  }
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
  if (auto Res = validate(*Tab.getLimit()); !Res) {
    return Unexpect(Res);
  }
  return {};
}

/// Validate Memory type. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::MemoryType &Mem) {
  /// Validate memory limits.
  const auto *Lim = Mem.getLimit();
  if (auto Res = validate(*Lim); !Res) {
    return Unexpect(Res);
  }
  if (Lim->getMin() > LIMIT_MEMORYTYPE ||
      (Lim->hasMax() && Lim->getMax() > LIMIT_MEMORYTYPE)) {
    LOG(ERROR) << ErrCode::InvalidMemPages;
    LOG(ERROR) << ErrInfo::InfoLimit(Lim->hasMax(), Lim->getMin(),
                                     Lim->getMax());
    return Unexpect(ErrCode::InvalidMemPages);
  }
  return {};
}

/// Validate Global segment. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::GlobalSegment &GlobSeg) {
  /// Check global initialization is a const expression.
  if (auto Res = validateConstExpr(
          GlobSeg.getInstrs(),
          std::array{GlobSeg.getGlobalType()->getValueType()});
      !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Expression);
    return Unexpect(Res);
  }
  return {};
}

/// Validate Element segment. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ElementSegment &ElemSeg) {
  /// Check table index and reference type in context.
  const auto &TableVec = Checker.getTables();
  const auto &FuncVec = Checker.getFunctions();
  if (ElemSeg.getIdx() >= TableVec.size()) {
    LOG(ERROR) << ErrCode::InvalidTableIdx;
    LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Table,
                                           ElemSeg.getIdx(), TableVec.size());
    return Unexpect(ErrCode::InvalidTableIdx);
  }
  if (TableVec[ElemSeg.getIdx()] != RefType::FuncRef) {
    LOG(ERROR) << ErrCode::InvalidTableIdx;
    return Unexpect(ErrCode::InvalidTableIdx);
  }
  /// Check function indices exist in context.
  for (auto &Idx : ElemSeg.getFuncIdxes()) {
    if (Idx >= FuncVec.size()) {
      LOG(ERROR) << ErrCode::InvalidFuncIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Function,
                                             Idx, FuncVec.size());
      return Unexpect(ErrCode::InvalidFuncIdx);
    }
  }
  /// Check table initialization is const expression.
  if (auto Res =
          validateConstExpr(ElemSeg.getInstrs(), std::array{ValType::I32});
      !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Expression);
    return Unexpect(Res);
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
  return {};
}

/// Validate Import description. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ImportDesc &ImpDesc) {
  switch (ImpDesc.getExternalType()) {
  case ExternalType::Function:
    if (auto TId = ImpDesc.getExternalContent<uint32_t>()) {
      /// Types must exist in context.
      if (*(*TId) >= Checker.getTypes().size()) {
        LOG(ERROR) << ErrCode::InvalidFuncTypeIdx;
        LOG(ERROR) << ErrInfo::InfoForbidIndex(
            ErrInfo::IndexCategory::FunctionType, *(*TId),
            Checker.getTypes().size());
        return Unexpect(ErrCode::InvalidFuncTypeIdx);
      }
      Checker.addFunc(*(*TId), true);
    } else {
      LOG(ERROR) << ErrCode::InvalidFuncIdx;
      return Unexpect(ErrCode::InvalidFuncIdx);
    }
    break;
  case ExternalType::Table:
    if (auto TabType = ImpDesc.getExternalContent<AST::TableType>()) {
      /// Table type must be valid.
      if (auto Res = validate(*(*TabType))) {
        Checker.addTable(*(*TabType));
      } else {
        LOG(ERROR) << ErrInfo::InfoAST((*TabType)->NodeAttr);
        return Unexpect(Res);
      }
    } else {
      LOG(ERROR) << ErrCode::InvalidTableIdx;
      return Unexpect(ErrCode::InvalidTableIdx);
    }
    break;
  case ExternalType::Memory:
    if (auto MemType = ImpDesc.getExternalContent<AST::MemoryType>()) {
      /// Memory type must be valid.
      if (auto Res = validate(*(*MemType))) {
        Checker.addMemory(*(*MemType));
      } else {
        LOG(ERROR) << ErrInfo::InfoAST((*MemType)->NodeAttr);
        return Unexpect(Res);
      }
    } else {
      LOG(ERROR) << ErrCode::InvalidMemoryIdx;
      return Unexpect(ErrCode::InvalidMemoryIdx);
    }
    break;
  case ExternalType::Global:
    if (auto GlobType = ImpDesc.getExternalContent<AST::GlobalType>()) {
      /// Global type always is valid.
      Checker.addGlobal(*(*GlobType), true);
    } else {
      LOG(ERROR) << ErrCode::InvalidGlobalIdx;
      return Unexpect(ErrCode::InvalidGlobalIdx);
    }
    break;
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
    break;
  case ExternalType::Table:
    if (Id >= Checker.getTables().size()) {
      LOG(ERROR) << ErrCode::InvalidTableIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Table, Id,
                                             Checker.getTables().size());
      return Unexpect(ErrCode::InvalidTableIdx);
    }
    break;
  case ExternalType::Memory:
    if (Id >= Checker.getMemories().size()) {
      LOG(ERROR) << ErrCode::InvalidMemoryIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Memory, Id,
                                             Checker.getMemories().size());
      return Unexpect(ErrCode::InvalidMemoryIdx);
    }
    break;
  case ExternalType::Global:
    if (Id >= Checker.getGlobals().size()) {
      LOG(ERROR) << ErrCode::InvalidGlobalIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Global, Id,
                                             Checker.getGlobals().size());
      return Unexpect(ErrCode::InvalidGlobalIdx);
    }
    break;
  default:
    break;
  }
  return {};
}

/// Validate Import section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ImportSection &ImportSec) {
  for (auto &ImportDesc : ImportSec.getContent()) {
    if (auto Res = validate(*ImportDesc.get()); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(ImportDesc->NodeAttr);
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
  for (auto &TabSeg : TabSec.getContent()) {
    if (auto Res = validate(*TabSeg.get())) {
      Checker.addTable(*TabSeg.get());
    } else {
      LOG(ERROR) << ErrInfo::InfoAST(TabSeg->NodeAttr);
      return Unexpect(Res);
    }
  }
  return {};
}

/// Validate Memory section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::MemorySection &MemSec) {
  for (auto &MemSeg : MemSec.getContent()) {
    if (auto Res = validate(*MemSeg.get())) {
      Checker.addMemory(*MemSeg.get());
    } else {
      LOG(ERROR) << ErrInfo::InfoAST(MemSeg->NodeAttr);
      return Unexpect(Res);
    }
  }
  return {};
}

/// Validate Global section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::GlobalSection &GlobSec) {
  for (auto &GlobSeg : GlobSec.getContent()) {
    if (auto Res = validate(*GlobSeg.get())) {
      Checker.addGlobal(*GlobSeg.get()->getGlobalType());
    } else {
      LOG(ERROR) << ErrInfo::InfoAST(GlobSeg->NodeAttr);
      return Unexpect(Res);
    }
  }
  return {};
}

/// Validate Element section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ElementSection &ElemSec) {
  for (auto &ElemSeg : ElemSec.getContent()) {
    if (auto Res = validate(*ElemSeg.get()); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(ElemSeg->NodeAttr);
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
    if (auto Res = validate(*CodeVec[Id].get(), FuncVec[TId]); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(CodeVec[Id]->NodeAttr);
      return Unexpect(Res);
    }
  }
  return {};
}

/// Validate Data section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::DataSection &DataSec) {
  for (auto &Data : DataSec.getContent()) {
    if (auto Res = validate(*Data.get()); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(Data->NodeAttr);
      return Unexpect(Res);
    }
  }
  return {};
}

/// Validate Start section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::StartSection &StartSec) {
  auto FId = StartSec.getContent();
  if (FId >= Checker.getFunctions().size()) {
    LOG(ERROR) << ErrCode::InvalidFuncIdx;
    LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Function,
                                           FId, Checker.getFunctions().size());
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
  return {};
}

/// Validate Export section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ExportSection &ExportSec) {
  std::unordered_set<std::string> ExportNames;
  for (auto &ExportDesc : ExportSec.getContent()) {
    auto Result = ExportNames.emplace(ExportDesc->getExternalName());
    if (!Result.second) {
      /// Duplicated export name.
      LOG(ERROR) << ErrCode::DupExportName;
      LOG(ERROR) << ErrInfo::InfoAST(ExportDesc->NodeAttr);
      return Unexpect(ErrCode::DupExportName);
    }
    if (auto Res = validate(*ExportDesc.get()); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(ExportDesc->NodeAttr);
      return Unexpect(Res);
    }
  }
  return {};
}

/// Validate constant expression. See "include/validator/validator.h".
Expect<void> Validator::validateConstExpr(const AST::InstrVec &Instrs,
                                          Span<const ValType> Returns) {
  for (auto &Instr : Instrs) {
    /// Only these 5 instructions are constant.
    switch (Instr->getOpCode()) {
    case OpCode::Global__get: {
      /// For initialization case, global indices must be imported globals.
      auto GlobInstr = static_cast<AST::VariableInstruction *>(Instr.get());
      auto GlobIdx = GlobInstr->getVariableIndex();
      if (GlobInstr->getVariableIndex() >= Checker.getNumImportGlobals()) {
        LOG(ERROR) << ErrCode::InvalidGlobalIdx;
        LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Global,
                                               GlobIdx,
                                               Checker.getNumImportGlobals());
        LOG(ERROR) << ErrInfo::InfoInstruction(GlobInstr->getOpCode(),
                                               GlobInstr->getOffset());
        return Unexpect(ErrCode::InvalidGlobalIdx);
      }
      if (Checker.getGlobals()[GlobIdx].second != ValMut::Const) {
        LOG(ERROR) << ErrCode::ConstExprRequired;
        LOG(ERROR) << ErrInfo::InfoInstruction(GlobInstr->getOpCode(),
                                               GlobInstr->getOffset());
        return Unexpect(ErrCode::ConstExprRequired);
      }
    }
      // fall through
    case OpCode::I32__const:
    case OpCode::I64__const:
    case OpCode::F32__const:
    case OpCode::F64__const:
      break;
    default:
      LOG(ERROR) << ErrCode::ConstExprRequired;
      LOG(ERROR) << ErrInfo::InfoInstruction(Instr->getOpCode(),
                                             Instr->getOffset());
      return Unexpect(ErrCode::ConstExprRequired);
    }
  }
  /// Validate expression with result types.
  Checker.reset();
  return Checker.validate(Instrs, Returns);
}

} // namespace Validator
} // namespace SSVM
