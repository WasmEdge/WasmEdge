#include "validator/validator.h"
#include "ast/module.h"
#include "vm/common.h"

#include <algorithm>
#include <iostream>

namespace SSVM {
namespace Validator {

ErrCode Validator::validate(const AST::Limit *Lim, unsigned int K) {
  bool Cond1 = Lim->getMin() <= K;
  bool Cond2 = true;

  if (Lim->hasMax()) {
    Cond2 = Lim->getMax() <= K && Lim->getMin() <= Lim->getMax();
  }

  if (Cond1 && Cond2)
    return ErrCode::Success;
  return ErrCode::Invalid;
}

ErrCode Validator::validate(AST::FunctionType *Func) {
  /// The restriction to at most one result may be removed in future versions of
  /// WebAssembly.
  if (Func->getReturnTypes().size() > 1)
    return ErrCode::Invalid;
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::TableType *Tab) {
  return validate(Tab->getLimit(), LIMIT_TABLETYPE);
}

ErrCode Validator::validate(AST::MemoryType *Mem) {
  return validate(Mem->getLimit(), LIMIT_MEMORYTYPE);
}

ErrCode Validator::validate(AST::GlobalType *) {
  /// The global type is always valid
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::FunctionSection *FuncSec,
                            AST::CodeSection *CodeSec,
                            AST::TypeSection *TypeSec) {
  if (!FuncSec)
    return ErrCode::Success;

  if (FuncSec->getContent().size() != CodeSec->getContent().size())
    return ErrCode::Invalid;

  size_t TotalFunctions = FuncSec->getContent().size();

  for (size_t Id = 0; Id < TotalFunctions; ++Id) {
    auto TId = FuncSec->getContent().at(Id);

    if (TId >= TypeSec->getContent().size())
      return ErrCode::Invalid;

    VM.addFunc(TypeSec->getContent().at(TId).get());
  }

  for (size_t Id = 0; Id < TotalFunctions; ++Id) {
    auto TId = FuncSec->getContent().at(Id);

    if (validate(CodeSec->getContent().at(Id).get(),
                 TypeSec->getContent().at(TId).get()) != ErrCode::Success)
      return ErrCode::Invalid;
  }
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::CodeSegment *CodeSeg,
                            AST::FunctionType *Func) {
  VM.reset();
  int Idx = 0;

  for (auto Val : Func->getParamTypes()) {
    VM.addLocal(Idx++, Val);
  }

  for (auto Val : CodeSeg->getLocals()) {
    for (unsigned int Cnt = 0; Cnt < Val.first; ++Cnt)
      VM.addLocal(Idx++, Val.second);
  }
  return VM.validate(CodeSeg->getInstrs(), Func->getReturnTypes());
}

ErrCode Validator::validate(AST::MemorySection *MemSec) {
  if (!MemSec)
    return ErrCode::Success;

  for (auto &Mem : MemSec->getContent())
    if (validate(Mem.get()) != ErrCode::Success)
      return ErrCode::Invalid;

  return ErrCode::Success;
}

ErrCode Validator::validate(AST::TableSection *TabSec) {
  if (!TabSec)
    return ErrCode::Success;

  for (auto &Tab : TabSec->getContent()) {
    if (validate(Tab.get()) != ErrCode::Success)
      return ErrCode::Invalid;

    switch (Tab->getElementType()) {
    case AST::ElemType::FuncRef:
      break;
    default:
      // In future versions of WebAssembly, additional element types may be
      // introduced.
      return ErrCode::Invalid;
    }
  }

  return ErrCode::Success;
}

ErrCode Validator::validate(AST::GlobalSection *GlobSec) {
  if (!GlobSec)
    return ErrCode::Success;

  for (auto &Val : GlobSec->getContent())
    if (validate(Val.get()) != ErrCode::Success) {
      return ErrCode::Invalid;
    } else {
      VM.addGlobal(*Val.get()->getGlobalType());
    }

  return ErrCode::Success;
}

ErrCode Validator::validate(AST::GlobalSegment *) {
  /// TODO: Check GlobSeg->getInstrs(); is a const expr
  /// std::cerr << "...GlobalSegment check are ignored (unimplemented)"
  ///           << std::endl;
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ElementSegment *ElemSeg) {
  // In the current version of WebAssembly, at most one table is allowed in a
  // module. Consequently, the only valid tableidx is 0
  if (ElemSeg->getIdx() != 0)
    return ErrCode::Invalid;

  // TODO check ElemSeg->getInstrs(); is const expr
  /// std::cerr << "...ElementSegment check are ignored (unimplemented)"
  ///           << std::endl;
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ElementSection *ElemSec) {
  if (!ElemSec)
    return ErrCode::Success;

  for (auto &Elem : ElemSec->getContent())
    if (validate(Elem.get()) != ErrCode::Success)
      return ErrCode::Invalid;
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::StartSection *StartSec) {
  if (!StartSec)
    return ErrCode::Success;

  auto FId = StartSec->getContent();

  if (FId >= VM.getFunctions().size())
    return ErrCode::Invalid;

  auto &Type = VM.getFunctions().at(FId);
  if (Type.first.size() != 0 || Type.second.size() != 0)
    return ErrCode::Invalid;

  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ExportSection *ExportSec) {
  if (!ExportSec)
    return ErrCode::Success;

  for (auto &ExportDesc : ExportSec->getContent())
    if (validate(ExportDesc.get()) != ErrCode::Success)
      return ErrCode::Invalid;
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ExportDesc *ExportDesc) {
  auto Id = ExportDesc->getExternalIndex();

  switch (ExportDesc->getExternalType()) {
  case AST::Desc::ExternalType::Function:
    if (Id >= VM.getFunctions().size())
      return ErrCode::Invalid;
    break;
  case AST::Desc::ExternalType::Global:
    if (Id >= VM.getGlobals().size())
      return ErrCode::Invalid;
    break;
  case AST::Desc::ExternalType::Memory:
  case AST::Desc::ExternalType::Table:
    /// std::cerr << "...ExportDesc check are ignored (unimplemented)"
    ///           << std::endl;
    break;
  default:
    // unreachable code
    return ErrCode::Invalid;
  }
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ImportSection *ImportSec,
                            AST::TypeSection *TypeSec) {
  if (!ImportSec)
    return ErrCode::Success;

  for (auto &ImportDesc : ImportSec->getContent())
    if (validate(ImportDesc.get(), TypeSec) != ErrCode::Success)
      return ErrCode::Invalid;
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ImportDesc *ImportDesc,
                            AST::TypeSection *TypeSec) {
  switch (ImportDesc->getExternalType()) {
  case SSVM::AST::Desc::ExternalType::Function:
    unsigned int *TId;
    if (ImportDesc->getExternalContent(TId) != SSVM::Executor::ErrCode::Success)
      return ErrCode::Invalid;
    VM.addFunc(TypeSec->getContent().at(*TId).get());
    break;
  case SSVM::AST::Desc::ExternalType::Global:
    AST::GlobalType *GlobType;
    if (ImportDesc->getExternalContent(GlobType) !=
        SSVM::Executor::ErrCode::Success)
      return ErrCode::Invalid;
    VM.addGlobal(*GlobType);
    break;
  default:
    /// std::cerr << "ImportDesc check are ignored (unimplemented type:"
    ///           << (int)ImportDesc->getExternalType() << ")" << std::endl;
    break;
  }

  return ErrCode::Success;
}

ErrCode Validator::validate(std::unique_ptr<AST::Module> &Mod) {
  reset();

  // Every import defines an index in the respective index space. In each index
  // space, the indices of imports go before the first index of any definition
  // contained in the module itself.
  if (validate((*Mod).getImportSection(), (*Mod).getTypeSection()) !=
      ErrCode::Success)
    return ErrCode::Invalid;

  if ((*Mod).getTypeSection())
    for (auto &Type : (*Mod).getTypeSection()->getContent())
      VM.addType(Type.get());

  if (validate((*Mod).getTableSection()) != ErrCode::Success)
    return ErrCode::Invalid;

  // https://webassembly.github.io/spec/core/valid/modules.html
  if (validate((*Mod).getMemorySection()) != ErrCode::Success)
    return ErrCode::Invalid;

  if (validate((*Mod).getGlobalSection()) != ErrCode::Success)
    return ErrCode::Invalid;

  if (validate((*Mod).getElementSection()) != ErrCode::Success)
    return ErrCode::Invalid;

  if (validate((*Mod).getFunctionSection(), (*Mod).getCodeSection(),
               (*Mod).getTypeSection()) != ErrCode::Success)
    return ErrCode::Invalid;

  if (validate((*Mod).getStartSection()) != ErrCode::Success)
    return ErrCode::Invalid;

  if (validate((*Mod).getExportSection()) != ErrCode::Success)
    return ErrCode::Invalid;

  return ErrCode::Success;
}

void Validator::reset() { VM.reset(true); }

} // namespace Validator
} // namespace SSVM