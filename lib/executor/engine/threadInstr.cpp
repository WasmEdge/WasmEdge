// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

Expect<void>
Executor::runAtomicNotifyOp(Runtime::StackManager &StackMgr,
                            Runtime::Instance::MemoryInstance &MemInst,
                            const AST::Instruction &Instr) {
  auto [Count, Offset] = StackMgr.pops<uint32_t, uint32_t>();
  EXPECTED_TRY(const uint32_t EA,
               atomicAddressGet<uint32_t>(MemInst, Offset, Instr));
  EXPECTED_TRY(uint32_t Total,
               atomicNotify(MemInst, EA, Count).map_error([&Instr](auto E) {
                 spdlog::error(E);
                 spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(),
                                                        Instr.getOffset()));
                 return E;
               }));
  StackMgr.push(Total);
  return {};
}

Expect<void> Executor::runMemoryFenceOp() {
  std::atomic_thread_fence(std::memory_order_release);
  return {};
}

Expect<uint32_t>
Executor::atomicNotify(Runtime::Instance::MemoryInstance &MemInst,
                       uint32_t Address, uint32_t Count) noexcept {
  // The error message should be handled by the caller, or the AOT mode will
  // produce the duplicated messages.
  if (auto *AtomicObj = MemInst.getPointer<std::atomic<uint32_t> *>(Address);
      !AtomicObj) {
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }

  std::unique_lock<decltype(WaiterMapMutex)> Locker(WaiterMapMutex);
  uint32_t Total = 0;
  auto Range = WaiterMap.equal_range(Address);
  for (auto Iterator = Range.first; Total < Count && Iterator != Range.second;
       ++Iterator) {
    if (likely(&MemInst == Iterator->second.MemInst)) {
      Iterator->second.Cond.notify_all();
      ++Total;
    }
  }
  return Total;
}

void Executor::atomicNotifyAll() noexcept {
  std::unique_lock<decltype(WaiterMapMutex)> Locker(WaiterMapMutex);
  for (auto Iterator = WaiterMap.begin(); Iterator != WaiterMap.end();
       ++Iterator) {
    Iterator->second.Cond.notify_all();
  }
}

} // namespace Executor
} // namespace WasmEdge
