// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "ast/type.h"
#include "executor/executor.h"

#include <cstdint>
#include <string_view>

namespace WasmEdge {
namespace Executor {

// Instantiate exports. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::ExportSection &ExportSec) {
  // Iterate through the export descriptions and instantiate the exports.
  for (const auto &ExpDesc : ExportSec.getContent()) {
    // Get data from the export description.
    const auto ExtType = ExpDesc.getExternalType();
    std::string_view ExtName = ExpDesc.getExternalName();
    const uint32_t ExtIdx = ExpDesc.getExternalIndex();

    // Export the instance with the name.
    switch (ExtType) {
    case ExternalType::Function:
      ModInst.exportFunction(ExtName, ExtIdx);

      // Set exported malloc and free functions.
      if (ExtName.compare("malloc") == 0) {
        // Verify function signature.
        spdlog::info("Found malloc function in module {0}",
                     ModInst.getModuleName());
        const auto MallocFunc = ModInst.findFuncExports(ExtName);
        const auto MallocFuncType =
            AST::FunctionType({ValType::I32}, {ValType::I32});
        if (MallocFunc->getFuncType() == MallocFuncType) {
          ModInst.setMallocFunction(MallocFunc);
        }
      }
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

} // namespace Executor
} // namespace WasmEdge
