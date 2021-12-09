#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

Expect<void> Executor::runAtomicNofityOp(
    Runtime::StackManager &StackMgr,
    [[maybe_unused]] Runtime::Instance::MemoryInstance &MemInst,
    [[maybe_unused]] const AST::Instruction &Instr) {

  ValVariant RawAddress = StackMgr.pop();
  ValVariant &RawCount = StackMgr.getTop();
  [[maybe_unused]] uint32_t Address = RawAddress.get<uint32_t>();
  [[maybe_unused]] int32_t Count = RawCount.get<int32_t>();

  // Notify will be supported in C++20
  return {};
}

Expect<void> Executor::runMemoryFenceOp() {
  std::atomic_thread_fence(std::memory_order_release);
  return {};
}

} // namespace Executor
} // namespace WasmEdge