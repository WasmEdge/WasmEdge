// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include <cstdint>
#include <string_view>

namespace WasmEdge {
namespace Executor {

// Instantiate exports. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::StoreManager &,
                                   Runtime::StackManager &,
                                   Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::ExportSection &ExportSec) {
  // Iterate and istantiate export descriptions.
  for (const auto &ExpDesc : ExportSec.getContent()) {
    // Get data from export description.
    const auto ExtType = ExpDesc.getExternalType();
    std::string_view ExtName = ExpDesc.getExternalName();
    const uint32_t ExtIdx = ExpDesc.getExternalIndex();

    // Add the name of instances module.
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

} // namespace Executor
} // namespace WasmEdge
