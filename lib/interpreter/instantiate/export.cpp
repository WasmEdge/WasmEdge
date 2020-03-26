// SPDX-License-Identifier: Apache-2.0
#include "common/ast/section.h"
#include "common/types.h"
#include "runtime/instance/function.h"
#include "runtime/instance/global.h"
#include "runtime/instance/memory.h"
#include "runtime/instance/module.h"
#include "runtime/instance/table.h"
#include "interpreter/interpreter.h"

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

    /// Add the name of instances module.
    switch (ExtType) {
    case ExternalType::Function:
      ModInst.exportFuncion(ExtName, ExtIdx);
      break;
    case ExternalType::Global:
      ModInst.exportGlobal(ExtName, ExtIdx);
      break;
    case ExternalType::Memory:
      ModInst.exportMemory(ExtName, ExtIdx);
      break;
    case ExternalType::Table:
      ModInst.exportTable(ExtName, ExtIdx);
      break;
    default:
      break;
    }
  }
  return {};
}

} // namespace Interpreter
} // namespace SSVM
