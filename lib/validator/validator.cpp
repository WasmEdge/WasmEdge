#include "validator/validator.h"

#include "ast/module.h"
#include "vm/common.h"

#include <algorithm>
#include <iostream>

namespace SSVM {
namespace Validator {

ErrCode Validator::validate(const AST::Limit *limit, unsigned int K) {
  bool cond1 = limit->getMin() <= K;
  bool cond2 = true;

  if (limit->hasMax()) {
    cond2 = limit->getMax() <= K && limit->getMin() <= limit->getMax();
  }

  if (cond1 && cond2)
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

  size_t TotoalFunctions = FuncSec->getContent().size();

  for (size_t id = 0; id < TotoalFunctions; ++id) {
    auto tid = FuncSec->getContent().at(id);

    if (tid >= TypeSec->getContent().size())
      return ErrCode::Invalid;

    vm.addfunc(TypeSec->getContent().at(tid).get());
  }

  for (size_t id = 0; id < TotoalFunctions; ++id) {
    auto tid = FuncSec->getContent().at(id);

    if (validate(CodeSec->getContent().at(id).get(),
                 TypeSec->getContent().at(tid).get()) != ErrCode::Success)
      return ErrCode::Invalid;
  }
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::CodeSegment *CodeSeg,
                            AST::FunctionType *Func) {
  vm.reset();
  int idx = 0;

  for (auto val : Func->getParamTypes()) {
    vm.addloacl(idx++, val);
  }

  for (auto val : CodeSeg->getLocals()) {
    for (unsigned int cnt = 0; cnt < val.first; ++cnt)
      vm.addloacl(idx++, val.second);
  }
  return vm.validate(CodeSeg->getInstrs(), Func->getReturnTypes());
}

ErrCode Validator::validate(AST::MemorySection *MemSec) {
  if (!MemSec)
    return ErrCode::Success;

  for (auto &mem : MemSec->getContent())
    if (validate(mem.get()) != ErrCode::Success)
      return ErrCode::Invalid;

  return ErrCode::Success;
}

ErrCode Validator::validate(AST::TableSection *TabSec) {
  if (!TabSec)
    return ErrCode::Success;

  for (auto &tab : TabSec->getContent()) {
    if (validate(tab.get()) != ErrCode::Success)
      return ErrCode::Invalid;

    switch (tab->getElementType()) {
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

ErrCode Validator::validate(AST::GlobalSection *GloSec) {
  if (!GloSec)
    return ErrCode::Success;

  for (auto &val : GloSec->getContent())
    if (validate(val.get()) != ErrCode::Success) {
      return ErrCode::Invalid;
    } else {
      vm.addglobal(*val.get()->getGlobalType());
    }

  return ErrCode::Success;
}

ErrCode Validator::validate(AST::GlobalSegment *) {
  // TODO: Check GloSeg->getInstrs(); is a const expr
  std::cerr << "...GlobalSegment check are ignored (unimplemented)"
            << std::endl;
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ElementSegment *EleSeg) {
  // In the current version of WebAssembly, at most one table is allowed in a
  // module. Consequently, the only valid tableidx is 0
  if (EleSeg->getIdx() != 0)
    return ErrCode::Invalid;

  // TODO check EleSeg->getInstrs(); is const expr
  std::cerr << "...ElementSegment check are ignored (unimplemented)"
            << std::endl;
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ElementSection *EleSec) {
  if (!EleSec)
    return ErrCode::Success;

  for (auto &element : EleSec->getContent())
    if (validate(element.get()) != ErrCode::Success)
      return ErrCode::Invalid;
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::StartSection *StartSec) {
  if (!StartSec)
    return ErrCode::Success;

  auto fid = StartSec->getContent();

  if (fid >= vm.getFunctions().size())
    return ErrCode::Invalid;

  auto &type = vm.getFunctions().at(fid);
  if (type.first.size() != 0 || type.second.size() != 0)
    return ErrCode::Invalid;

  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ExportSection *ExportSec) {
  if (!ExportSec)
    return ErrCode::Success;

  for (auto &exportedc : ExportSec->getContent())
    if (validate(exportedc.get()) != ErrCode::Success)
      return ErrCode::Invalid;
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ExportDesc *ExportDesc) {
  auto id = ExportDesc->getExternalIndex();

  switch (ExportDesc->getExternalType()) {
  case AST::Desc::ExternalType::Function:
    if (id >= vm.getFunctions().size())
      return ErrCode::Invalid;
    break;
  case AST::Desc::ExternalType::Global:
    if (id >= vm.getGlobals().size())
      return ErrCode::Invalid;
    break;
  case AST::Desc::ExternalType::Memory:
  case AST::Desc::ExternalType::Table:
    std::cerr << "...ExportDesc check are ignored (unimplemented)" << std::endl;
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

  for (auto &import : ImportSec->getContent())
    if (validate(import.get(), TypeSec) != ErrCode::Success)
      return ErrCode::Invalid;
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ImportDesc *Import,
                            AST::TypeSection *TypeSec) {
  switch (Import->getExternalType()) {
  case SSVM::AST::Desc::ExternalType::Function:
    unsigned int *tid;
    if (Import->getExternalContent(tid) != SSVM::Executor::ErrCode::Success)
      return ErrCode::Invalid;
    vm.addfunc(TypeSec->getContent().at(*tid).get());
    break;
  case SSVM::AST::Desc::ExternalType::Global:
    AST::GlobalType *global;
    if (Import->getExternalContent(global) != SSVM::Executor::ErrCode::Success)
      return ErrCode::Invalid;
    vm.addglobal(*global);
    break;
  default:
    std::cerr << "ImportDesc check are ignored (unimplemented type:"
              << (int)Import->getExternalType() << ")" << std::endl;
  }

  return ErrCode::Success;
}

ErrCode Validator::validate(std::unique_ptr<AST::Module> &Mod) {
  std::cout << "start Validator" << std::endl;
  reset();

  // Every import defines an index in the respective index space. In each index
  // space, the indices of imports go before the first index of any definition
  // contained in the module itself.
  if (validate((*Mod).getImportSection(), (*Mod).getTypeSection()) !=
      ErrCode::Success)
    return ErrCode::Invalid;

  if ((*Mod).getTypeSection())
    for (auto &type : (*Mod).getTypeSection()->getContent())
      vm.addtype(type.get());

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

  std::cout << "Validator OK" << std::endl;
  return ErrCode::Success;
}

void Validator::reset() { vm.reset(true); }

} // namespace Validator
} // namespace SSVM