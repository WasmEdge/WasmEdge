// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

Expect<void>
Executor::runAtomicNotifyOp(Runtime::StackManager &StackMgr,
                            Runtime::Instance::MemoryInstance &MemInst,
                            const AST::Instruction &Instr) {
  ValVariant RawCount = StackMgr.pop();
  ValVariant &RawAddress = StackMgr.getTop();

  uint64_t Address =
      valToIndex(RawAddress, MemInst.getMemoryType().getIdxType());

  if (auto Res = checkOutOfBound<sizeof(uint32_t) * 8>(MemInst, Instr, Address);
      !Res) {
    return Unexpect(Res);
  }

  Address += Instr.getMemoryOffset();

  if (Address % sizeof(uint64_t) != 0) {
    spdlog::error(ErrCode::Value::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::UnalignedAtomicAccess);
  }

  uint64_t Count = valToIndex(RawCount, MemInst.getMemoryType().getIdxType());
  EXPECTED_TRY(
      auto Total,
      atomicNotify(MemInst, Address, Count).map_error([&Instr](auto E) {
        spdlog::error(E);
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return E;
      }));
  RawAddress.emplace<uint64_t>(Total);
  return {};
}

Expect<void> Executor::runMemoryFenceOp() {
  std::atomic_thread_fence(std::memory_order_release);
  return {};
}

Expect<uint64_t>
Executor::atomicNotify(Runtime::Instance::MemoryInstance &MemInst,
                       uint64_t Address, uint64_t Count) noexcept {
  // The error message should be handled by the caller, or the AOT mode will
  // produce the duplicated messages.
  if (auto *AtomicObj = MemInst.getPointer<std::atomic<uint64_t> *>(Address);
      !AtomicObj) {
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }

  std::unique_lock<decltype(WaiterMapMutex)> Locker(WaiterMapMutex);
  uint64_t Total = 0;
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
