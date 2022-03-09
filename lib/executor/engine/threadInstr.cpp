#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

Expect<void> Executor::runAtomicNofityOp(
    Runtime::StackManager &StackMgr,
    [[maybe_unused]] Runtime::Instance::MemoryInstance &MemInst,
    [[maybe_unused]] const AST::Instruction &Instr) {

  [[maybe_unused]] ValVariant Address = StackMgr.pop();
  [[maybe_unused]] ValVariant Count = StackMgr.pop();
  // TODO: Implemnt Notify
  StackMgr.push(ValVariant(0));
  return {};
}

Expect<void> Executor::runMemoryFenceOp(
    [[maybe_unused]] Runtime::StackManager &StackMgr,
    [[maybe_unused]] Runtime::Instance::MemoryInstance &MemInst,
    [[maybe_unused]] const AST::Instruction &Instr) {
  std::atomic_thread_fence(std::memory_order_release);
  return {};
}

} // namespace Executor
} // namespace WasmEdge