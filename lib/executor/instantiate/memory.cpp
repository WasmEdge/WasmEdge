// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

// Instantiate memory instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::MemorySection &MemSec) {
  // Prepare pointers vector for compiled functions.
  ModInst.MemoryPtrs.resize(ModInst.getMemoryNum() +
                            MemSec.getContent().size());

  // Set the memory pointers of imported memories.
  for (uint32_t I = 0; I < ModInst.getMemoryNum(); ++I) {
#if WASMEDGE_ALLOCATOR_IS_STABLE
    ModInst.MemoryPtrs[I] = (*ModInst.getMemory(I))->getDataPtr();
#else
    ModInst.MemoryPtrs[I] = &(*ModInst.getMemory(I))->getDataPtr();
#endif
  }

  // Iterate through the memory types to instantiate memory instances.
  for (const auto &MemType : MemSec.getContent()) {
    // Create and add the memory instance into the module instance.
    ModInst.addMemory(MemType, Conf.getRuntimeConfigure().getMaxMemoryPage());
    const auto Index = ModInst.getMemoryNum() - 1;
    Runtime::Instance::MemoryInstance *MemInst = *ModInst.getMemory(Index);
    // Set the memory pointers of instantiated memories.
#if WASMEDGE_ALLOCATOR_IS_STABLE
    ModInst.MemoryPtrs[Index] = MemInst->getDataPtr();
#else
    ModInst.MemoryPtrs[Index] = &MemInst->getDataPtr();
#endif
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
