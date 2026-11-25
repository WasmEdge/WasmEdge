// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/endian.h"
#include "common/types.h"
#include "executor/executor.h"
#include "runtime/instance/memory.h"
#include <experimental/scope.hpp>

#include <cstdint>

namespace WasmEdge {
namespace Executor {

template <typename T>
TypeT<T> Executor::runAtomicWaitOp(Runtime::StackManager &StackMgr,
                                   Runtime::Instance::MemoryInstance &MemInst,
                                   const AST::Instruction &Instr) {
  ValVariant RawTimeout = StackMgr.pop();
  ValVariant RawValue = StackMgr.pop();
  ValVariant &RawAddress = StackMgr.getTop();
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t Address = extractAddr(RawAddress, AddrType);
  EXPECTED_TRY(checkOffsetOverflow(MemInst, Instr, Address, sizeof(T)));
  Address += Instr.getMemoryOffset();

  if (Address % sizeof(T) != 0) {
    spdlog::error(ErrCode::Value::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::UnalignedAtomicAccess);
  }

  int64_t Timeout = RawTimeout.get<int64_t>();

  return atomicWait<T>(MemInst, Address, RawValue.get<T>(), Timeout)
      .map_error([&Instr](auto E) {
        spdlog::error(E);
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return E;
      })
      .map([&](auto V) { RawAddress = emplaceAddr(V, AddressType::I32); });
}

template <typename T, typename I>
TypeT<T> Executor::runAtomicLoadOp(Runtime::StackManager &StackMgr,
                                   Runtime::Instance::MemoryInstance &MemInst,
                                   const AST::Instruction &Instr) {
  ValVariant &RawAddress = StackMgr.getTop();
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t Address = extractAddr(RawAddress, AddrType);
  EXPECTED_TRY(checkOffsetOverflow(MemInst, Instr, Address, sizeof(T)));
  Address += Instr.getMemoryOffset();

  if (Address % sizeof(I) != 0) {
    spdlog::error(ErrCode::Value::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::UnalignedAtomicAccess);
  }

  // make sure the address no OOB with size I
  auto *AtomicObj = MemInst.getPointer<std::atomic<I> *>(Address);
  if (!AtomicObj) {
    spdlog::error(ErrCode::Value::MemoryOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }

  EndianValue<I> Return = AtomicObj->load();
  RawAddress.emplace<T>(static_cast<T>(Return.le()));
  return {};
}

template <typename T, typename I>
TypeT<T> Executor::runAtomicStoreOp(Runtime::StackManager &StackMgr,
                                    Runtime::Instance::MemoryInstance &MemInst,
                                    const AST::Instruction &Instr) {
  ValVariant RawValue = StackMgr.pop();
  ValVariant RawAddress = StackMgr.pop();
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t Address = extractAddr(RawAddress, AddrType);
  EXPECTED_TRY(checkOffsetOverflow(MemInst, Instr, Address, sizeof(T)));
  Address += Instr.getMemoryOffset();

  if (Address % sizeof(I) != 0) {
    spdlog::error(ErrCode::Value::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::UnalignedAtomicAccess);
  }

  // make sure the address no OOB with size I
  auto *AtomicObj = MemInst.getPointer<std::atomic<I> *>(Address);
  if (!AtomicObj) {
    spdlog::error(ErrCode::Value::MemoryOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  EndianValue<I> Value = static_cast<I>(RawValue.get<T>());

  AtomicObj->store(Value.le());
  return {};
}

template <typename T, typename AtomicOp, typename BinaryOp>
T runAtomicOp(std::atomic<T> *AtomicObj, T Value, AtomicOp Op [[maybe_unused]],
              BinaryOp BinOp [[maybe_unused]]) {
  if constexpr (Endian::native == Endian::little) {
    return Op(AtomicObj, Value);
  } else {
    EndianValue<T> Return;
    EndianValue<T> Result;
    do {
      Return = AtomicObj->load();
      Result = BinOp(Return.le(), Value);
    } while (!AtomicObj->compare_exchange_weak(Return.raw(), Result.le()));
    return Return.le();
  }
}

template <typename T, typename I>
TypeT<T> Executor::runAtomicAddOp(Runtime::StackManager &StackMgr,
                                  Runtime::Instance::MemoryInstance &MemInst,
                                  const AST::Instruction &Instr) {
  ValVariant RawValue = StackMgr.pop();
  ValVariant &RawAddress = StackMgr.getTop();
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t Address = extractAddr(RawAddress, AddrType);
  EXPECTED_TRY(checkOffsetOverflow(MemInst, Instr, Address, sizeof(T)));
  Address += Instr.getMemoryOffset();

  if (Address % sizeof(I) != 0) {
    spdlog::error(ErrCode::Value::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::UnalignedAtomicAccess);
  }

  // make sure the address no OOB with size I
  auto *AtomicObj = MemInst.getPointer<std::atomic<I> *>(Address);
  if (!AtomicObj) {
    spdlog::error(ErrCode::Value::MemoryOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  I Value = static_cast<I>(RawValue.get<T>());

  I Return = runAtomicOp(
      AtomicObj, Value,
      [](std::atomic<I> *Obj, I Val) { return Obj->fetch_add(Val); },
      std::plus<I>{});
  RawAddress.emplace<T>(static_cast<T>(Return));
  return {};
}

template <typename T, typename I>
TypeT<T> Executor::runAtomicSubOp(Runtime::StackManager &StackMgr,
                                  Runtime::Instance::MemoryInstance &MemInst,
                                  const AST::Instruction &Instr) {
  ValVariant RawValue = StackMgr.pop();
  ValVariant &RawAddress = StackMgr.getTop();
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t Address = extractAddr(RawAddress, AddrType);
  EXPECTED_TRY(checkOffsetOverflow(MemInst, Instr, Address, sizeof(T)));
  Address += Instr.getMemoryOffset();

  if (Address % sizeof(I) != 0) {
    spdlog::error(ErrCode::Value::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::UnalignedAtomicAccess);
  }

  // make sure the address no OOB with size I
  auto *AtomicObj = MemInst.getPointer<std::atomic<I> *>(Address);
  if (!AtomicObj) {
    spdlog::error(ErrCode::Value::MemoryOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  I Value = static_cast<I>(RawValue.get<T>());

  I Return = runAtomicOp(
      AtomicObj, Value,
      [](std::atomic<I> *Obj, I Val) { return Obj->fetch_sub(Val); },
      std::minus<I>{});
  RawAddress.emplace<T>(static_cast<T>(Return));
  return {};
}

template <typename T, typename I>
TypeT<T> Executor::runAtomicOrOp(Runtime::StackManager &StackMgr,
                                 Runtime::Instance::MemoryInstance &MemInst,
                                 const AST::Instruction &Instr) {
  ValVariant RawValue = StackMgr.pop();
  ValVariant &RawAddress = StackMgr.getTop();
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t Address = extractAddr(RawAddress, AddrType);
  EXPECTED_TRY(checkOffsetOverflow(MemInst, Instr, Address, sizeof(T)));
  Address += Instr.getMemoryOffset();

  if (Address % sizeof(I) != 0) {
    spdlog::error(ErrCode::Value::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::UnalignedAtomicAccess);
  }

  // make sure the address no OOB with size I
  auto *AtomicObj = MemInst.getPointer<std::atomic<I> *>(Address);
  if (!AtomicObj) {
    spdlog::error(ErrCode::Value::MemoryOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  EndianValue<I> Value = static_cast<I>(RawValue.get<T>());

  EndianValue<I> Return = AtomicObj->fetch_or(Value.le());
  RawAddress.emplace<T>(static_cast<T>(Return.le()));
  return {};
}

template <typename T, typename I>
TypeT<T> Executor::runAtomicAndOp(Runtime::StackManager &StackMgr,
                                  Runtime::Instance::MemoryInstance &MemInst,
                                  const AST::Instruction &Instr) {
  ValVariant RawValue = StackMgr.pop();
  ValVariant &RawAddress = StackMgr.getTop();
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t Address = extractAddr(RawAddress, AddrType);
  EXPECTED_TRY(checkOffsetOverflow(MemInst, Instr, Address, sizeof(T)));
  Address += Instr.getMemoryOffset();

  if (Address % sizeof(I) != 0) {
    spdlog::error(ErrCode::Value::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::UnalignedAtomicAccess);
  }

  // make sure the address no OOB with size I
  auto *AtomicObj = MemInst.getPointer<std::atomic<I> *>(Address);
  if (!AtomicObj) {
    spdlog::error(ErrCode::Value::MemoryOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  EndianValue<I> Value = static_cast<I>(RawValue.get<T>());

  EndianValue<I> Return = AtomicObj->fetch_and(Value.le());
  RawAddress.emplace<T>(static_cast<T>(Return.le()));
  return {};
}

template <typename T, typename I>
TypeT<T> Executor::runAtomicXorOp(Runtime::StackManager &StackMgr,
                                  Runtime::Instance::MemoryInstance &MemInst,
                                  const AST::Instruction &Instr) {
  ValVariant RawValue = StackMgr.pop();
  ValVariant &RawAddress = StackMgr.getTop();
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t Address = extractAddr(RawAddress, AddrType);
  EXPECTED_TRY(checkOffsetOverflow(MemInst, Instr, Address, sizeof(T)));
  Address += Instr.getMemoryOffset();

  if (Address % sizeof(I) != 0) {
    spdlog::error(ErrCode::Value::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::UnalignedAtomicAccess);
  }

  // make sure the address no OOB with size I
  auto *AtomicObj = MemInst.getPointer<std::atomic<I> *>(Address);
  if (!AtomicObj) {
    spdlog::error(ErrCode::Value::MemoryOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  EndianValue<I> Value = static_cast<I>(RawValue.get<T>());

  EndianValue<I> Return = AtomicObj->fetch_xor(Value.le());
  RawAddress.emplace<T>(static_cast<T>(Return.le()));
  return {};
}

template <typename T, typename I>
TypeT<T>
Executor::runAtomicExchangeOp(Runtime::StackManager &StackMgr,
                              Runtime::Instance::MemoryInstance &MemInst,
                              const AST::Instruction &Instr) {
  ValVariant RawValue = StackMgr.pop();
  ValVariant &RawAddress = StackMgr.getTop();
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t Address = extractAddr(RawAddress, AddrType);
  EXPECTED_TRY(checkOffsetOverflow(MemInst, Instr, Address, sizeof(T)));
  Address += Instr.getMemoryOffset();

  if (Address % sizeof(I) != 0) {
    spdlog::error(ErrCode::Value::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::UnalignedAtomicAccess);
  }

  // make sure the address no OOB with size I
  auto *AtomicObj = MemInst.getPointer<std::atomic<I> *>(Address);
  if (!AtomicObj) {
    spdlog::error(ErrCode::Value::MemoryOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  EndianValue<I> Value = static_cast<I>(RawValue.get<T>());

  EndianValue<I> Return = AtomicObj->exchange(Value.le());
  RawAddress.emplace<T>(static_cast<T>(Return.le()));
  return {};
}

template <typename T, typename I>
TypeT<T>
Executor::runAtomicCompareExchangeOp(Runtime::StackManager &StackMgr,
                                     Runtime::Instance::MemoryInstance &MemInst,
                                     const AST::Instruction &Instr) {
  ValVariant RawReplacement = StackMgr.pop();
  ValVariant RawExpected = StackMgr.pop();
  ValVariant &RawAddress = StackMgr.getTop();
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t Address = extractAddr(RawAddress, AddrType);
  EXPECTED_TRY(checkOffsetOverflow(MemInst, Instr, Address, sizeof(T)));
  Address += Instr.getMemoryOffset();

  if (Address % sizeof(I) != 0) {
    spdlog::error(ErrCode::Value::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::UnalignedAtomicAccess);
  }

  // make sure the address no OOB with size I
  auto *AtomicObj = MemInst.getPointer<std::atomic<I> *>(Address);
  if (!AtomicObj) {
    spdlog::error(ErrCode::Value::MemoryOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  EndianValue<I> Replacement = static_cast<I>(RawReplacement.get<T>());
  I Expected = EndianValue<I>(static_cast<I>(RawExpected.get<T>())).le();

  AtomicObj->compare_exchange_strong(Expected, Replacement.le());
  RawAddress.emplace<T>(static_cast<T>(Expected));
  return {};
}

template <typename T>
Expect<uint64_t>
Executor::atomicWait(Runtime::Instance::MemoryInstance &MemInst,
                     uint64_t Address, EndianValue<T> Expected,
                     int64_t Timeout) noexcept {
  // The error message should be handled by the caller, or the AOT mode will
  // produce the duplicated messages.
  if (!MemInst.isShared()) {
    return Unexpect(ErrCode::Value::ExpectSharedMemory);
  }

  if (auto *AtomicObj = MemInst.getPointer<std::atomic<T> *>(Address);
      !AtomicObj) {
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }

  std::optional<std::chrono::time_point<std::chrono::steady_clock>> Until;
  if (Timeout >= 0) {
    Until.emplace(std::chrono::steady_clock::now() +
                  std::chrono::nanoseconds(Timeout));
  }

  auto *AtomicObj = MemInst.getPointer<std::atomic<T> *>(Address);
  assuming(AtomicObj);

  auto &WaiterMapMtx = MemInst.getWaiterMapMutex();
  auto &WaiterMap = MemInst.getWaiterMap();

  StopState *Run = enclosingRun();
  std::list<Runtime::Instance::MemoryInstance::Waiter>::iterator WaiterIt;
  Runtime::Instance::MemoryInstance::Waiter *Waiter;
  {
    // Compare, enqueue, and attach the waiter to its run under the lock that
    // atomicNotify() takes, so that a notify sees the waiter only after a stop
    // of the run can reach it.
    std::unique_lock<std::mutex> Locker(WaiterMapMtx);
    if (AtomicObj->load() != Expected.le()) {
      return static_cast<uint64_t>(1); // NotEqual
    }
    auto &Queue = WaiterMap[Address];
    WaiterIt = Queue.emplace(Queue.end());
    Waiter = &*WaiterIt;
    if (Run) {
      Run->setWaiter(Waiter);
    }
  }
  cxx20::scope_exit ScopeExitHolder([&]() noexcept {
    if (Run) {
      Run->setWaiter(nullptr);
    }
    std::unique_lock<std::mutex> Locker(WaiterMapMtx);
    auto Queue = WaiterMap.find(Address);
    assuming(Queue != WaiterMap.end());
    Queue->second.erase(WaiterIt);
    if (Queue->second.empty()) {
      WaiterMap.erase(Queue);
    }
  });

  std::unique_lock<std::mutex> Locker(Waiter->Mutex);
  // StopState::requestStop() and StopState::setWaiter() set Interrupted under
  // this same mutex, so the state alone decides the result.
  using WaitState = Runtime::Instance::MemoryInstance::Waiter::WaitState;
  auto Woken = [Waiter]() noexcept {
    return Waiter->State != WaitState::Waiting;
  };
  if (!Until) {
    Waiter->Cond.wait(Locker, Woken);
  } else if (!Waiter->Cond.wait_until(Locker, *Until, Woken)) {
    Waiter->State = WaitState::TimedOut;
  }
  if (Waiter->State == WaitState::Notified) {
    // Woken by memory.atomic.notify — return "ok" per the spec, regardless of
    // the current memory value.
    return static_cast<uint64_t>(0); // ok
  }
  if (unlikely(Waiter->State == WaitState::Interrupted)) {
    return Unexpect(ErrCode::Value::Interrupted);
  }
  return static_cast<uint64_t>(2); // Timed-out
}

} // namespace Executor
} // namespace WasmEdge
