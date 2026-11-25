// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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

  if (Address % sizeof(uint32_t) != 0) {
    spdlog::error(ErrCode::Value::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::UnalignedAtomicAccess);
  }

  uint64_t Count = extractAddr(RawCount, AddressType::I32);
  EXPECTED_TRY(
      auto Total,
      atomicNotify(MemInst, Address, Count).map_error([&Instr](auto E) {
        spdlog::error(E);
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return E;
      }));
  RawAddress = emplaceAddr(Total, AddressType::I32);
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
  if (!MemInst.checkAccessBound(Address, sizeof(uint32_t))) {
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }

  auto &WaiterMapMtx = MemInst.getWaiterMapMutex();
  auto &WaiterMap = MemInst.getWaiterMap();
  std::unique_lock<std::mutex> Locker(WaiterMapMtx);
  uint64_t Total = 0;
  using WaitState = Runtime::Instance::MemoryInstance::Waiter::WaitState;
  auto Queue = WaiterMap.find(Address);
  if (Queue == WaiterMap.end()) {
    return Total;
  }
  for (auto Iterator = Queue->second.begin();
       Total < Count && Iterator != Queue->second.end(); ++Iterator) {
    if (Iterator->wake(WaitState::Notified)) {
      ++Total;
    }
  }
  return Total;
}

} // namespace Executor
} // namespace WasmEdge
