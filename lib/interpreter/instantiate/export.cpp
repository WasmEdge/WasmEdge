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
    std::string_view ExtName = ExpDesc->getExternalName();
    const uint32_t ExtIdx = ExpDesc->getExternalIndex();
    auto Symbol = ExpDesc->getSymbol();

    /// Add the name of instances module.
    switch (ExtType) {
    case ExternalType::Function:
      ModInst.exportFunction(ExtName, ExtIdx);
      if (Symbol) {
        uint32_t Addr = *ModInst.getFuncAddr(ExtIdx);
        auto *Inst = *StoreMgr.getFunction(Addr);
        Inst->setSymbol(
            std::move(Symbol)
                .cast<Runtime::Instance::FunctionInstance::CompiledFunction>());
      }
      break;
    case ExternalType::Global:
      ModInst.exportGlobal(ExtName, ExtIdx);
      if (Symbol) {
        uint32_t Addr = *ModInst.getGlobalAddr(ExtIdx);
        auto *Inst = *StoreMgr.getGlobal(Addr);
        Inst->setSymbol(std::move(Symbol).cast<ValVariant>());
      }
      break;
    case ExternalType::Memory:
      ModInst.exportMemory(ExtName, ExtIdx);
      if (Symbol) {
        uint32_t Addr = *ModInst.getMemAddr(ExtIdx);
        auto *Inst = *StoreMgr.getMemory(Addr);
        Inst->setSymbol(std::move(Symbol).cast<uint8_t *>());
      }
      break;
    case ExternalType::Table:
      ModInst.exportTable(ExtName, ExtIdx);
      if (Symbol) {
        uint32_t Addr = *ModInst.getTableAddr(ExtIdx);
        auto *Inst = *StoreMgr.getTable(Addr);
        Inst->setSymbol(std::move(Symbol).cast<uint32_t>());
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
