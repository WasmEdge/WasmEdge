// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/endian.h"
#include "common/types.h"
#include "executor/executor.h"
#include "runtime/instance/memory.h"
#include <experimental/scope.hpp>

#include <cstdint>

namespace WasmEdge {
namespace Executor {

template <typename T>
Expect<uint32_t>
Executor::atomicAddressGet(Runtime::Instance::MemoryInstance &MemInst,
                           uint32_t Offset,
                           const AST::Instruction &Instr) noexcept {
  // Calculate EA
  EXPECTED_TRY(const uint32_t EA,
               calculateEA<sizeof(T) * 8>(MemInst, Offset, Instr));

  if (EA % sizeof(T) != 0) {
    spdlog::error(ErrCode::Value::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::UnalignedAtomicAccess);
  }

  return EA;
}

template <typename T>
Expect<std::atomic<T> *>
Executor::atomicGet(Runtime::Instance::MemoryInstance &MemInst,
                    uint32_t Address, const AST::Instruction &Instr) noexcept {
  // make sure the address no OOB with size I
  auto *AtomicObj = MemInst.getPointer<std::atomic<T> *>(Address);
  if (!AtomicObj) {
    spdlog::error(ErrCode::Value::MemoryOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  return AtomicObj;
}

template <typename T>
TypeT<T> Executor::runAtomicWaitOp(Runtime::StackManager &StackMgr,
                                   Runtime::Instance::MemoryInstance &MemInst,
                                   const AST::Instruction &Instr) noexcept {
  auto [Timeout, Value, Offset] = StackMgr.pops<int64_t, T, uint32_t>();
  EXPECTED_TRY(auto Address, atomicAddressGet<T>(MemInst, Offset, Instr));
  EXPECTED_TRY(atomicGet<T>(MemInst, Address, Instr));
  return atomicWait<T>(MemInst, Address, Value, Timeout)
      .map_error([&Instr](auto E) {
        spdlog::error(E);
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return E;
      })
      .map([&](uint32_t V) { StackMgr.push(V); });
}

template <typename T, typename I>
TypeT<T> Executor::runAtomicLoadOp(Runtime::StackManager &StackMgr,
                                   Runtime::Instance::MemoryInstance &MemInst,
                                   const AST::Instruction &Instr) noexcept {
  auto Offset = StackMgr.pop<uint32_t>();
  EXPECTED_TRY(auto Address, atomicAddressGet<I>(MemInst, Offset, Instr));
  EXPECTED_TRY(auto *AtomicObj, atomicGet<I>(MemInst, Address, Instr));
  EndianValue<I> Return = AtomicObj->load();
  StackMgr.push(static_cast<T>(Return.le()));
  return {};
}

template <typename T, typename I>
TypeT<T> Executor::runAtomicStoreOp(Runtime::StackManager &StackMgr,
                                    Runtime::Instance::MemoryInstance &MemInst,
                                    const AST::Instruction &Instr) noexcept {
  auto [RawValue, Offset] = StackMgr.pops<T, uint32_t>();
  EXPECTED_TRY(auto Address, atomicAddressGet<I>(MemInst, Offset, Instr));
  EXPECTED_TRY(auto *AtomicObj, atomicGet<I>(MemInst, Address, Instr));
  EndianValue<I> Value = static_cast<I>(RawValue);
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
                                  const AST::Instruction &Instr) noexcept {
  auto [RawValue, Offset] = StackMgr.pops<T, uint32_t>();
  EXPECTED_TRY(auto Address, atomicAddressGet<I>(MemInst, Offset, Instr));
  EXPECTED_TRY(auto *AtomicObj, atomicGet<I>(MemInst, Address, Instr));
  I Value = static_cast<I>(RawValue);
  I Return = runAtomicOp(
      AtomicObj, Value,
      [](std::atomic<I> *Obj, I Val) { return Obj->fetch_add(Val); },
      std::plus<I>{});
  StackMgr.push(static_cast<T>(Return));
  return {};
}

template <typename T, typename I>
TypeT<T> Executor::runAtomicSubOp(Runtime::StackManager &StackMgr,
                                  Runtime::Instance::MemoryInstance &MemInst,
                                  const AST::Instruction &Instr) noexcept {
  auto [RawValue, Offset] = StackMgr.pops<T, uint32_t>();
  EXPECTED_TRY(auto Address, atomicAddressGet<I>(MemInst, Offset, Instr));
  EXPECTED_TRY(auto *AtomicObj, atomicGet<I>(MemInst, Address, Instr));
  I Value = static_cast<I>(RawValue);
  I Return = runAtomicOp(
      AtomicObj, Value,
      [](std::atomic<I> *Obj, I Val) { return Obj->fetch_sub(Val); },
      std::minus<I>{});
  StackMgr.push(static_cast<T>(Return));
  return {};
}

template <typename T, typename I>
TypeT<T> Executor::runAtomicOrOp(Runtime::StackManager &StackMgr,
                                 Runtime::Instance::MemoryInstance &MemInst,
                                 const AST::Instruction &Instr) noexcept {
  auto [RawValue, Offset] = StackMgr.pops<T, uint32_t>();
  EXPECTED_TRY(auto Address, atomicAddressGet<I>(MemInst, Offset, Instr));
  EXPECTED_TRY(auto *AtomicObj, atomicGet<I>(MemInst, Address, Instr));
  EndianValue<I> Value = static_cast<I>(RawValue);
  EndianValue<I> Return = AtomicObj->fetch_or(Value.le());
  StackMgr.push(static_cast<T>(Return.le()));
  return {};
}

template <typename T, typename I>
TypeT<T> Executor::runAtomicAndOp(Runtime::StackManager &StackMgr,
                                  Runtime::Instance::MemoryInstance &MemInst,
                                  const AST::Instruction &Instr) noexcept {
  auto [RawValue, Offset] = StackMgr.pops<T, uint32_t>();
  EXPECTED_TRY(auto Address, atomicAddressGet<I>(MemInst, Offset, Instr));
  EXPECTED_TRY(auto *AtomicObj, atomicGet<I>(MemInst, Address, Instr));
  EndianValue<I> Value = static_cast<I>(RawValue);
  EndianValue<I> Return = AtomicObj->fetch_and(Value.le());
  StackMgr.push(static_cast<T>(Return.le()));
  return {};
}

template <typename T, typename I>
TypeT<T> Executor::runAtomicXorOp(Runtime::StackManager &StackMgr,
                                  Runtime::Instance::MemoryInstance &MemInst,
                                  const AST::Instruction &Instr) noexcept {
  auto [RawValue, Offset] = StackMgr.pops<T, uint32_t>();
  EXPECTED_TRY(auto Address, atomicAddressGet<I>(MemInst, Offset, Instr));
  EXPECTED_TRY(auto *AtomicObj, atomicGet<I>(MemInst, Address, Instr));
  EndianValue<I> Value = static_cast<I>(RawValue);
  EndianValue<I> Return = AtomicObj->fetch_xor(Value.le());
  StackMgr.push(static_cast<T>(Return.le()));
  return {};
}

template <typename T, typename I>
TypeT<T>
Executor::runAtomicExchangeOp(Runtime::StackManager &StackMgr,
                              Runtime::Instance::MemoryInstance &MemInst,
                              const AST::Instruction &Instr) noexcept {
  auto [RawValue, Offset] = StackMgr.pops<T, uint32_t>();
  EXPECTED_TRY(auto Address, atomicAddressGet<I>(MemInst, Offset, Instr));
  EXPECTED_TRY(auto *AtomicObj, atomicGet<I>(MemInst, Address, Instr));
  EndianValue<I> Value = static_cast<I>(RawValue);
  EndianValue<I> Return = AtomicObj->exchange(Value.le());
  StackMgr.push(static_cast<T>(Return.le()));
  return {};
}

template <typename T, typename I>
TypeT<T>
Executor::runAtomicCompareExchangeOp(Runtime::StackManager &StackMgr,
                                     Runtime::Instance::MemoryInstance &MemInst,
                                     const AST::Instruction &Instr) noexcept {
  auto [RawReplacement, RawExpected, Offset] = StackMgr.pops<T, T, uint32_t>();
  EXPECTED_TRY(auto Address, atomicAddressGet<I>(MemInst, Offset, Instr));
  EXPECTED_TRY(auto *AtomicObj, atomicGet<I>(MemInst, Address, Instr));
  EndianValue<I> Replacement = static_cast<I>(RawReplacement);
  I Expected = EndianValue<I>(static_cast<I>(RawExpected)).le();
  AtomicObj->compare_exchange_strong(Expected, Replacement.le());
  StackMgr.push(static_cast<T>(Expected));
  return {};
}

template <typename T>
Expect<uint32_t>
Executor::atomicWait(Runtime::Instance::MemoryInstance &MemInst,
                     uint32_t Address, EndianValue<T> Expected,
                     int64_t Timeout) noexcept {
  // The error message should be handled by the caller, or the AOT mode will
  // produce the duplicated messages.
  if (!MemInst.isShared()) {
    return Unexpect(ErrCode::Value::ExpectSharedMemory);
  }

  auto *AtomicObj = MemInst.getPointer<std::atomic<T> *>(Address);
  if (!AtomicObj) {
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }

  std::optional<std::chrono::time_point<std::chrono::steady_clock>> Until;
  if (Timeout >= 0) {
    Until.emplace(std::chrono::steady_clock::now() +
                  std::chrono::nanoseconds(Timeout));
  }

  if (AtomicObj->load() != Expected.le()) {
    return UINT32_C(1); // NotEqual
  }

  decltype(WaiterMap)::iterator WaiterIterator;
  {
    std::unique_lock<decltype(WaiterMapMutex)> Locker(WaiterMapMutex);
    WaiterIterator = WaiterMap.emplace(Address, &MemInst);
  }

  cxx20::scope_exit ScopeExitHolder([&]() noexcept {
    std::unique_lock<decltype(WaiterMapMutex)> Locker(WaiterMapMutex);
    WaiterMap.erase(WaiterIterator);
  });

  while (true) {
    std::unique_lock<decltype(WaiterIterator->second.Mutex)> Locker(
        WaiterIterator->second.Mutex);
    std::cv_status WaitResult = std::cv_status::no_timeout;
    if (!Until) {
      WaiterIterator->second.Cond.wait(Locker);
    } else {
      WaitResult = WaiterIterator->second.Cond.wait_until(Locker, *Until);
    }
    if (unlikely(StopToken.load(std::memory_order_relaxed) != 0)) {
      return Unexpect(ErrCode::Value::Interrupted);
    }
    if (likely(AtomicObj->load() != Expected.le())) {
      return UINT32_C(0); // ok
    }
    if (WaitResult == std::cv_status::timeout) {
      return UINT32_C(2); // Timed-out
    }
  }
}

} // namespace Executor
} // namespace WasmEdge
