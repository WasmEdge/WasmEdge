// SPDX-License-Identifier: Apache-2.0
#include "ast/section.h"
#include "executor/executor.h"
#include "executor/instance/function.h"
#include "executor/instance/global.h"
#include "executor/instance/memory.h"
#include "executor/instance/module.h"
#include "executor/instance/table.h"

namespace SSVM {
namespace Executor {

/// Instantiate export instance. See "include/executor/executor.h".
ErrCode Executor::instantiate(AST::ExportSection *ExportSec) {
  if (ExportSec == nullptr) {
    return ErrCode::Success;
  }
  ErrCode Status = ErrCode::Success;

  /// Iterate and istantiate export descriptions.
  auto &ExpDescs = ExportSec->getContent();
  for (auto ExpDesc = ExpDescs.begin(); ExpDesc != ExpDescs.end(); ExpDesc++) {
    /// Get data from export description.
    auto ExtType = (*ExpDesc)->getExternalType();
    const std::string &ExtName = (*ExpDesc)->getExternalName();
    unsigned int ExtIdx = (*ExpDesc)->getExternalIndex();
    unsigned int EntityAddr = 0;

    /// Add the name of function to function instance.
    switch (ExtType) {
    case AST::Desc::ExternalType::Function: {
      Instance::FunctionInstance *FuncInst = nullptr;
      /// Find function instance.
      if ((Status = ModInst->getFuncAddr(ExtIdx, EntityAddr)) !=
          ErrCode::Success) {
        return Status;
      }
      if ((Status = StoreMgr.getFunction(EntityAddr, FuncInst)) !=
          ErrCode::Success) {
        return Status;
      }
      /// Set function name. TODO: module name
      if ((Status = FuncInst->setNames("", ExtName)) != ErrCode::Success) {
        return Status;
      }
      /// Set start function index.
      if (StartFunc != "" && ExtName == StartFunc) {
        if ((Status = ModInst->setStartIdx(ExtIdx)) != ErrCode::Success) {
          return Status;
        }
      }
      break;
    }
    case AST::Desc::ExternalType::Global: {
      Instance::GlobalInstance *GlobInst = nullptr;
      /// Find global instance.
      if ((Status = ModInst->getGlobalAddr(ExtIdx, EntityAddr)) !=
          ErrCode::Success) {
        return Status;
      }
      if ((Status = StoreMgr.getGlobal(EntityAddr, GlobInst)) !=
          ErrCode::Success) {
        return Status;
      }
      /// Set global name. TODO: module name
      if ((Status = GlobInst->setNames("", ExtName)) != ErrCode::Success) {
        return Status;
      }
      break;
    }
    case AST::Desc::ExternalType::Memory: {
      Instance::MemoryInstance *MemInst = nullptr;
      /// Find memory instance.
      if ((Status = ModInst->getMemAddr(ExtIdx, EntityAddr)) !=
          ErrCode::Success) {
        return Status;
      }
      if ((Status = StoreMgr.getMemory(EntityAddr, MemInst)) !=
          ErrCode::Success) {
        return Status;
      }
      /// Set memory name. TODO: module name
      if ((Status = MemInst->setNames("", ExtName)) != ErrCode::Success) {
        return Status;
      }
      break;
    }
    case AST::Desc::ExternalType::Table: {
      Instance::TableInstance *TabInst = nullptr;
      /// Find table instance.
      if ((Status = ModInst->getTableAddr(ExtIdx, EntityAddr)) !=
          ErrCode::Success) {
        return Status;
      }
      if ((Status = StoreMgr.getTable(EntityAddr, TabInst)) !=
          ErrCode::Success) {
        return Status;
      }
      /// Set table name. TODO: module name
      if ((Status = TabInst->setNames("", ExtName)) != ErrCode::Success) {
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
