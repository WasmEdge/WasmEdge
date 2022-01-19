#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

Expect<void> Executor::runAtomicNofityOp(
    [[maybe_unused]] Runtime::Instance::MemoryInstance &MemInst,
    [[maybe_unused]] const AST::Instruction &Instr) {

  detail::atomicLock();
  ValVariant Address = StackMgr.pop();
  ValVariant Count = StackMgr.pop();
  // TODO: Implemnt Notify
  StackMgr.push(ValVariant(0));
  detail::atomicUnlock();
  return {};
}

Expect<void> Executor::runMemoryFenceOp(
    [[maybe_unused]] Runtime::Instance::MemoryInstance &MemInst,
    [[maybe_unused]] const AST::Instruction &Instr) {
  detail::atomicLock();
  // TODO: Implement Fence
  detail::atomicUnlock();
  return {};
}

} // namespace Executor
} // namespace WasmEdge