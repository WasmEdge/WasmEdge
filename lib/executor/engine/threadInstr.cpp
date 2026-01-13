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
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t Address = extractAddr(RawAddress, AddrType);
  EXPECTED_TRY(checkOffsetOverflow(MemInst, Instr, Address, sizeof(uint32_t)));
  Address += Instr.getMemoryOffset();
  uint32_t Align =
      AddrType == AddressType::I32 ? sizeof(uint32_t) : sizeof(uint64_t);

  if (Address % Align != 0) {
    spdlog::error(ErrCode::Value::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::UnalignedAtomicAccess);
  }

  uint64_t Count = extractAddr(RawCount, AddrType);
  EXPECTED_TRY(
      auto Total,
      atomicNotify(MemInst, Address, Count).map_error([&Instr](auto E) {
        spdlog::error(E);
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return E;
      }));
  RawAddress = emplaceAddr(Total, AddrType);
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
