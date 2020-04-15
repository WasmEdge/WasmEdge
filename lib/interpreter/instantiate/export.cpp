// SPDX-License-Identifier: Apache-2.0
#include "common/ast/section.h"
#include "common/types.h"
#include "interpreter/interpreter.h"
#include "runtime/instance/function.h"
#include "runtime/instance/global.h"
#include "runtime/instance/memory.h"
#include "runtime/instance/module.h"
#include "runtime/instance/table.h"

namespace SSVM {
namespace Interpreter {

/// Instantiate exports. See "include/interpreter/interpreter.h".
Expect<void>
Interpreter::instantiate(Runtime::StoreManager &StoreMgr,
                         Runtime::Instance::ModuleInstance &ModInst,
                         const AST::ExportSection &ExportSec) {
  /// Iterate and istantiate export descriptions.
  for (const auto &ExpDesc : ExportSec.getContent()) {
    /// Get data from export description.
    const auto ExtType = ExpDesc->getExternalType();
    const std::string &ExtName = ExpDesc->getExternalName();
    const uint32_t ExtIdx = ExpDesc->getExternalIndex();
    void *Symbol = ExpDesc->getSymbol();

    /// Add the name of instances module.
    switch (ExtType) {
    case ExternalType::Function:
      ModInst.exportFuncion(ExtName, ExtIdx);
      if (Symbol) {
        if (auto Addr = ModInst.getFuncAddr(ExtIdx)) {
          if (auto Inst = StoreMgr.getFunction(*Addr)) {
            (*Inst)->setSymbol(Symbol);
          } else {
            return Unexpect(Inst);
          }
        } else {
          return Unexpect(Addr);
        }
      }
      break;
    case ExternalType::Global:
      ModInst.exportGlobal(ExtName, ExtIdx);
      if (Symbol) {
        if (auto Addr = ModInst.getGlobalAddr(ExtIdx)) {
          if (auto Inst = StoreMgr.getGlobal(*Addr)) {
            (*Inst)->setSymbol(Symbol);
          } else {
            return Unexpect(Inst);
          }
        } else {
          return Unexpect(Addr);
        }
      }
      break;
    case ExternalType::Memory:
      ModInst.exportMemory(ExtName, ExtIdx);
      if (Symbol) {
        if (auto Addr = ModInst.getMemAddr(ExtIdx)) {
          if (auto Inst = StoreMgr.getMemory(*Addr)) {
            (*Inst)->setSymbol(Symbol);
          } else {
            return Unexpect(Inst);
          }
        } else {
          return Unexpect(Addr);
        }
      }
      break;
    case ExternalType::Table:
      ModInst.exportTable(ExtName, ExtIdx);
      if (Symbol) {
        if (auto Addr = ModInst.getTableAddr(ExtIdx)) {
          if (auto Inst = StoreMgr.getTable(*Addr)) {
            (*Inst)->setSymbol(Symbol);
          } else {
            return Unexpect(Inst);
          }
        } else {
          return Unexpect(Addr);
        }
      }
      break;
    default:
      break;
    }
  }
  return {};
}

} // namespace Interpreter
} // namespace SSVM
