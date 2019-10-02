#include "ast/section.h"
#include "executor/executor.h"
#include "executor/instance/function.h"
#include "executor/instance/global.h"
#include "executor/instance/memory.h"
#include "executor/instance/module.h"
#include "executor/instance/table.h"

namespace SSVM {
namespace Executor {

/// Instantiate imports. See "include/executor/executor.h".
ErrCode Executor::instantiate(AST::ImportSection *ImportSec) {
  if (ImportSec == nullptr) {
    return ErrCode::Success;
  }
  ErrCode Status = ErrCode::Success;

  /// Iterate and instantiate import descriptions.
  auto &ImpDescs = ImportSec->getContent();
  for (auto ImpDesc = ImpDescs.begin(); ImpDesc != ImpDescs.end(); ImpDesc++) {
    /// Get data from import description.
    auto ExtType = (*ImpDesc)->getExternalType();
    const std::string &ModName = (*ImpDesc)->getModuleName();
    const std::string &ExtName = (*ImpDesc)->getExternalName();

    /// Add the imports into module istance.
    switch (ExtType) {
    case AST::Desc::ExternalType::Function: /// Function type index
    {
      /// Find the function instance in Store.
      Instance::FunctionInstance *FuncInst = nullptr;
      if ((Status = StoreMgr.findFunction(ModName, ExtName, FuncInst)) !=
          ErrCode::Success) {
        return Status;
      }
      /// Get the function type index in module.
      unsigned int *TypeIdx = nullptr;
      if ((Status = (*ImpDesc)->getExternalContent(TypeIdx)) !=
          ErrCode::Success) {
        return Status;
      }
      /// TODO: Do import matching.
      /// Set the function address to module instance.
      if ((Status = ModInst->addFuncAddr(FuncInst->Addr)) != ErrCode::Success) {
        return Status;
      }
      break;
    }
    case AST::Desc::ExternalType::Table: /// Table type
    {
      /// Find the table instance in Store.
      Instance::TableInstance *TabInst = nullptr;
      if ((Status = StoreMgr.findTable(ModName, ExtName, TabInst)) !=
          ErrCode::Success) {
        return Status;
      }
      /// TODO: Do import matching.
      /// Set the memory address to module instance.
      if ((Status = ModInst->addTableAddr(TabInst->Addr)) != ErrCode::Success) {
        return Status;
      }
      break;
    }
    case AST::Desc::ExternalType::Memory: /// Memory type
    {
      /// Find the memory instance in Store.
      Instance::MemoryInstance *MemInst = nullptr;
      if ((Status = StoreMgr.findMemory(ModName, ExtName, MemInst)) !=
          ErrCode::Success) {
        return Status;
      }
      /// TODO: Do import matching.
      /// Set the memory address to module instance.
      if ((Status = ModInst->addMemAddr(MemInst->Addr)) != ErrCode::Success) {
        return Status;
      }
      break;
    }
    case AST::Desc::ExternalType::Global: /// Global type
    {
      /// Find the global instance in Store.
      Instance::GlobalInstance *GlobInst = nullptr;
      if ((Status = StoreMgr.findGlobal(ModName, ExtName, GlobInst)) !=
          ErrCode::Success) {
        return Status;
      }
      /// TODO: Do import matching.
      /// Set the global address to module instance.
      if ((Status = ModInst->addGlobalAddr(GlobInst->Addr)) !=
          ErrCode::Success) {
        return Status;
      }
      break;
    }
    default:
      break;
    }
  }
  return Status;
}

} // namespace Executor
} // namespace SSVM