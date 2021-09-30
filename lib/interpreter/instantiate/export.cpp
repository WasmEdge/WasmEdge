// SPDX-License-Identifier: Apache-2.0

#include "interpreter/interpreter.h"

namespace WasmEdge {
namespace Interpreter {

/// Instantiate exports. See "include/interpreter/interpreter.h".
Expect<void>
Interpreter::instantiate(Runtime::StoreManager &,
                         Runtime::Instance::ModuleInstance &ModInst,
                         const AST::ExportSection &ExportSec) {
  /// Iterate and istantiate export descriptions.
  for (const auto &ExpDesc : ExportSec.getContent()) {
    /// Get data from export description.
    const auto ExtType = ExpDesc.getExternalType();
    std::string_view ExtName = ExpDesc.getExternalName();
    const uint32_t ExtIdx = ExpDesc.getExternalIndex();

    /// Add the name of instances module.
    switch (ExtType) {
    case ExternalType::Function:
      ModInst.exportFunction(ExtName, ExtIdx);
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
} // namespace WasmEdge
