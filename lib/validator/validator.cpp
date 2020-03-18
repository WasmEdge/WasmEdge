// SPDX-License-Identifier: Apache-2.0
#include "validator/validator.h"
#include "common/ast/module.h"

#include <unordered_set>
#include <string>

namespace SSVM {
namespace Validator {

/// Validate Module. See "include/validator/validator.h".
Expect<void> Validator::validate(const std::unique_ptr<AST::Module> &Mod) {
  /// https://webassembly.github.io/spec/core/valid/modules.html
  Checker.reset(true);

  if (Mod == nullptr) {
    return {};
  }

  /// Register type definitions into FormChecker.
  if (Mod->getTypeSection()) {
    for (auto &Type : Mod->getTypeSection()->getContent()) {
      Checker.addType(*Type.get());
    }
  }

  /// Validate and register import section into FormChecker.
  if (auto Res = validate(Mod->getImportSection()); !Res) {
    return Unexpect(Res);
  }

  /// Validate table section and register tables into FormChecker.
  if (auto Res = validate(Mod->getTableSection()); !Res) {
    return Unexpect(Res);
  }

  /// Validate memory section and register memories into FormChecker.
  if (auto Res = validate(Mod->getMemorySection()); !Res) {
    return Unexpect(Res);
  }

  /// Validate global section and register globals into FormChecker.
  if (auto Res = validate(Mod->getGlobalSection()); !Res) {
    return Unexpect(Res);
  }

  /// Validate function section and code section.
  if (auto Res = validate(Mod->getFunctionSection(), Mod->getCodeSection());
      !Res) {
    return Unexpect(Res);
  }

  /// Validate element section which initialize tables.
  if (auto Res = validate(Mod->getElementSection()); !Res) {
    return Unexpect(Res);
  }

  /// Validate data section which initialize memories.
  if (auto Res = validate(Mod->getDataSection()); !Res) {
    return Unexpect(Res);
  }

  /// Validate start section.
  if (auto Res = validate(Mod->getStartSection()); !Res) {
    return Unexpect(Res);
  }

  /// Validate export section.
  if (auto Res = validate(Mod->getExportSection()); !Res) {
    return Unexpect(Res);
  }

  /// In current version, memory and table must be <= 1.
  if (Checker.getMemories().size() > 1 || Checker.getTables().size() > 1) {
    return Unexpect(ErrCode::ValidationFailed);
  }
  return {};
}

/// Validate Limit type. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::Limit &Lim, uint32_t K) {
  bool Cond1 = Lim.getMin() <= K;
  bool Cond2 =
      Lim.hasMax() ? (Lim.getMax() <= K && Lim.getMin() <= Lim.getMax()) : true;
  if (!Cond1 || !Cond2) {
    return Unexpect(ErrCode::ValidationFailed);
  }
  return {};
}

/// Validate Function type. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::FunctionType &Func) {
  /// The restriction to at most one result may be removed in future versions of
  /// WebAssembly.
  if (Func.getReturnTypes().size() > 1) {
    return Unexpect(ErrCode::ValidationFailed);
  }
  return {};
}

/// Validate Table type. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::TableType &Tab) {
  /// Validate table limits.
  return validate(*Tab.getLimit(), LIMIT_TABLETYPE);
}

/// Validate Memory type. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::MemoryType &Mem) {
  /// Validate memory limits.
  return validate(*Mem.getLimit(), LIMIT_MEMORYTYPE);
}

/// Validate Global segment. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::GlobalSegment &GlobSeg) {
  /// Check global initialization is a const expression.
  return validateConstExpr(GlobSeg.getInstrs(),
                           {GlobSeg.getGlobalType()->getValueType()});
}

/// Validate Element segment. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ElementSegment &ElemSeg) {
  /// Check table index and element type in context.
  const auto &TableVec = Checker.getTables();
  const auto &FuncVec = Checker.getFunctions();
  if (ElemSeg.getIdx() >= TableVec.size() ||
      TableVec[ElemSeg.getIdx()] != ElemType::FuncRef) {
    return Unexpect(ErrCode::ValidationFailed);
  }
  /// Check function indices exist in context.
  for (auto &Idx : ElemSeg.getFuncIdxes()) {
    if (Idx >= FuncVec.size()) {
      return Unexpect(ErrCode::ValidationFailed);
    }
  }
  /// Check table initialization is const expression.
  return validateConstExpr(ElemSeg.getInstrs(), {ValType::I32});
}

/// Validate Code segment. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::CodeSegment &CodeSeg,
                                 const uint32_t TypeIdx) {
  /// Reset stack in FormChecker.
  Checker.reset();
  /// Add parameters into this frame.
  uint32_t Idx = 0;
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
  return Checker.validate(CodeSeg.getInstrs(),
                          Checker.getTypes()[TypeIdx].second);
}

/// Validate Data segment. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::DataSegment &DataSeg) {
  /// Check memory index in context.
  const auto &MemVec = Checker.getMemories();
  if (DataSeg.getIdx() >= MemVec.size()) {
    return Unexpect(ErrCode::ValidationFailed);
  }
  /// Check memory initialization is a const expression.
  return validateConstExpr(DataSeg.getInstrs(), {ValType::I32});
}

/// Validate Import description. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ImportDesc &ImpDesc) {
  switch (ImpDesc.getExternalType()) {
  case AST::Desc::ExternalType::Function:
    if (auto TId = ImpDesc.getExternalContent<uint32_t>()) {
      /// Types must exist in context.
      if (*(*TId) >= Checker.getTypes().size()) {
        return Unexpect(ErrCode::ValidationFailed);
      }
      Checker.addFunc(*(*TId));
    } else {
      return Unexpect(TId);
    }
    break;
  case AST::Desc::ExternalType::Table:
    if (auto TabType = ImpDesc.getExternalContent<AST::TableType>()) {
      /// Table type must be valid.
      if (auto Res = validate(*(*TabType))) {
        Checker.addTable(*(*TabType));
      } else {
        return Unexpect(Res);
      }
    } else {
      return Unexpect(TabType);
    }
    break;
  case AST::Desc::ExternalType::Memory:
    if (auto MemType = ImpDesc.getExternalContent<AST::MemoryType>()) {
      /// Memory type must be valid.
      if (auto Res = validate(*(*MemType))) {
        Checker.addMemory(*(*MemType));
      } else {
        return Unexpect(Res);
      }
    } else {
      return Unexpect(MemType);
    }
    break;
  case AST::Desc::ExternalType::Global:
    if (auto GlobType = ImpDesc.getExternalContent<AST::GlobalType>()) {
      /// Global type always is valid.
      Checker.addGlobal(*(*GlobType), true);
    } else {
      return Unexpect(GlobType);
    }
    break;
  default:
    return Unexpect(ErrCode::ValidationFailed);
  }
  return {};
}

/// Validate Export description. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ExportDesc &ExpDesc) {
  auto Id = ExpDesc.getExternalIndex();
  switch (ExpDesc.getExternalType()) {
  case AST::Desc::ExternalType::Function:
    if (Id >= Checker.getFunctions().size()) {
      return Unexpect(ErrCode::ValidationFailed);
    }
    break;
  case AST::Desc::ExternalType::Table:
    if (Id >= Checker.getTables().size()) {
      return Unexpect(ErrCode::ValidationFailed);
    }
    break;
  case AST::Desc::ExternalType::Memory:
    if (Id >= Checker.getMemories().size()) {
      return Unexpect(ErrCode::ValidationFailed);
    }
    break;
  case AST::Desc::ExternalType::Global:
    if (Id >= Checker.getGlobals().size()) {
      return Unexpect(ErrCode::ValidationFailed);
    }
    break;
  default:
    return Unexpect(ErrCode::ValidationFailed);
  }
  return {};
}

/// Validate Import section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ImportSection *ImportSec) {
  if (!ImportSec) {
    return {};
  }
  for (auto &ImportDesc : ImportSec->getContent()) {
    if (auto Res = validate(*ImportDesc.get()); !Res) {
      return Unexpect(ErrCode::ValidationFailed);
    }
  }
  return {};
}

/// Validate Function section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::FunctionSection *FuncSec,
                                 const AST::CodeSection *CodeSec) {
  if (!FuncSec && !CodeSec) {
    return {};
  }
  if (
      /// Have only function section or code section, failed.
      ((FuncSec && !CodeSec) || (!FuncSec && CodeSec)) ||
      /// Function section length != code section length, failed.
      FuncSec->getContent().size() != CodeSec->getContent().size()) {
    return Unexpect(ErrCode::ValidationFailed);
  }

  const auto &FuncVec = FuncSec->getContent();
  const auto &CodeVec = CodeSec->getContent();
  const auto &TypeVec = Checker.getTypes();

  /// Check if type id of function is valid in context.
  for (size_t Id = 0; Id < FuncVec.size(); ++Id) {
    uint32_t TId = FuncVec[Id];
    if (TId >= TypeVec.size()) {
      return Unexpect(ErrCode::ValidationFailed);
    }
    Checker.addFunc(TId);
  }

  /// Validate function body.
  for (size_t Id = 0; Id < FuncVec.size(); ++Id) {
    uint32_t TId = FuncVec[Id];
    if (auto Res = validate(*CodeVec[Id].get(), TId); !Res) {
      return Unexpect(Res);
    }
  }
  return {};
}

/// Validate Table section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::TableSection *TabSec) {
  if (!TabSec) {
    return {};
  }
  for (auto &Tab : TabSec->getContent()) {
    if (auto Res = validate(*Tab.get())) {
      Checker.addTable(*Tab.get());
    } else {
      return Unexpect(ErrCode::ValidationFailed);
    }
  }
  return {};
}

/// Validate Memory section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::MemorySection *MemSec) {
  if (!MemSec) {
    return {};
  }
  for (auto &Mem : MemSec->getContent()) {
    if (auto Res = validate(*Mem.get())) {
      Checker.addMemory(*Mem.get());
    } else {
      return Unexpect(ErrCode::ValidationFailed);
    }
  }
  return {};
}

/// Validate Global section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::GlobalSection *GlobSec) {
  if (!GlobSec) {
    return {};
  }
  for (auto &Val : GlobSec->getContent()) {
    if (auto Res = validate(*Val.get())) {
      Checker.addGlobal(*Val.get()->getGlobalType());
    } else {
      return Unexpect(Res);
    }
  }
  return {};
}

/// Validate Export section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ExportSection *ExportSec) {
  if (!ExportSec) {
    return {};
  }
  std::unordered_set<std::string> ExportNames;
  for (auto &ExportDesc : ExportSec->getContent()) {
    const auto &Name = ExportDesc->getExternalName();
    if (Name == "" || ExportNames.find(Name) != ExportNames.end()) {
      /// Duplicated export name.
      return Unexpect(ErrCode::ValidationFailed);
    }
    ExportNames.insert(Name);
    if (auto Res = validate(*ExportDesc.get()); !Res) {
      return Unexpect(Res);
    }
  }
  return {};
}

/// Validate Start section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::StartSection *StartSec) {
  if (!StartSec) {
    return {};
  }
  auto FId = StartSec->getContent();
  if (FId >= Checker.getFunctions().size()) {
    return Unexpect(ErrCode::ValidationFailed);
  }
  auto TId = Checker.getFunctions()[FId];
  auto &Type = Checker.getTypes()[TId];
  if (Type.first.size() != 0 || Type.second.size() != 0) {
    return Unexpect(ErrCode::ValidationFailed);
  }
  return {};
}

/// Validate Element section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::ElementSection *ElemSec) {
  if (!ElemSec) {
    return {};
  }
  for (auto &Elem : ElemSec->getContent()) {
    if (auto Res = validate(*Elem.get()); !Res) {
      return Unexpect(Res);
    }
  }
  return {};
}

/// Validate Data section. See "include/validator/validator.h".
Expect<void> Validator::validate(const AST::DataSection *DataSec) {
  if (!DataSec) {
    return {};
  }
  for (auto &Data : DataSec->getContent()) {
    if (auto Res = validate(*Data.get()); !Res) {
      return Unexpect(Res);
    }
  }
  return {};
}

/// Validate constant expression. See "include/validator/validator.h".
Expect<void> Validator::validateConstExpr(const AST::InstrVec &Instrs,
                                          const std::vector<ValType> &Returns,
                                          const bool RestrictGlobal) {
  for (auto &Instr : Instrs) {
    /// Only these 5 instructions are constant.
    switch (Instr->getOpCode()) {
    case OpCode::Global__get:
      /// For global initialization case, global indices must be imported
      /// globals.
      if (RestrictGlobal) {
        auto GlobInstr = static_cast<AST::VariableInstruction *>(Instr.get());
        if (GlobInstr->getVariableIndex() >= Checker.getNumImportGlobals()) {
          return Unexpect(ErrCode::ValidationFailed);
        }
      }
    case OpCode::I32__const:
    case OpCode::I64__const:
    case OpCode::F32__const:
    case OpCode::F64__const:
      break;
    default:
      return Unexpect(ErrCode::ValidationFailed);
    }
  }
  /// Validate expression with result types.
  Checker.reset();
  return Checker.validate(Instrs, Returns);
}

} // namespace Validator
} // namespace SSVM