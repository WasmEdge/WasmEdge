// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

Expect<void>
Executor::runAtomicNotifyOp(Runtime::StackManager &StackMgr,
                            Runtime::Instance::MemoryInstance &MemInst,
                            const AST::Instruction &Instr) noexcept {
  auto [RawCount, RawAddress] = StackMgr.popsPeekTop<ValVariant, ValVariant>();
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t Address = extractAddr(RawAddress, AddrType);
  EXPECTED_TRY(checkOffsetOverflow(MemInst, Instr, Address, sizeof(uint32_t)));
  Address += Instr.getMemoryOffset();
  // notify's atomic access is always 4-byte (it does not widen for memory64),
  // and the count operand and woken-count result are both i32 regardless of
  // address type; treating them as i64 under memory64 would mis-type the stack.
  if (Address % sizeof(uint32_t) != 0) {
    spdlog::error(ErrCode::Value::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::UnalignedAtomicAccess);
  }

  const uint32_t Count = RawCount.get<uint32_t>();
  EXPECTED_TRY(
      auto Total,
      atomicNotify(MemInst, Address, Count).map_error([&Instr](auto E) {
        spdlog::error(E);
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return E;
      }));
  StackMgr.emplaceTop(static_cast<uint32_t>(Total));
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

  auto &WaiterMapMtx = MemInst.getWaiterMapMutex();
  auto &WaiterMap = MemInst.getWaiterMap();
  std::unique_lock<std::mutex> Locker(WaiterMapMtx);
  uint64_t Total = 0;
  auto Range = WaiterMap.equal_range(Address);
  for (auto Iterator = Range.first; Total < Count && Iterator != Range.second;
       ++Iterator) {
    {
      std::unique_lock<std::mutex> WaiterLocker(Iterator->second.Mutex);
      Iterator->second.Notified = true;
    }
    Iterator->second.Cond.notify_all();
    ++Total;
  }
  return Total;
}

void Executor::atomicNotifyAll() noexcept {
  if (auto *MemInst = WaitingMemory.load(std::memory_order_acquire)) {
    MemInst->notifyAllWaiters();
  }
}

} // namespace Executor
} // namespace WasmEdge
