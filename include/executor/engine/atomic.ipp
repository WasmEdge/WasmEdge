#include "executor/executor.h"
#include "runtime/instance/memory.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

template <typename T>
TypeT<T> Executor::runAtomicWaitOp(Runtime::StackManager &StackMgr,
                                   Runtime::Instance::MemoryInstance &MemInst,
                                   const AST::Instruction &Instr) {

  const uint32_t BitWidth = sizeof(T) * 8;
  ValVariant Address = StackMgr.pop();
  ValVariant Val = StackMgr.pop();
  [[maybe_unused]] ValVariant Timeout = StackMgr.pop();

  StackMgr.push(Address);
  runLoadOp<T>(StackMgr, MemInst, Instr, BitWidth);
  ValVariant &Loaded = StackMgr.getTop();

  if (Loaded.get<T>() == Val.get<T>()) {
    Loaded.emplace<T>(static_cast<T>(0));
  } else {
    Loaded.emplace<T>(static_cast<T>(1));
  }
  // Wait will be supported in C++20
  // TODO: Implement Timeout
  return {};
}

template <typename T, typename I>
TypeT<T> Executor::runAtomicLoadOp(Runtime::StackManager &StackMgr,
                                   Runtime::Instance::MemoryInstance &MemInst,
                                   const AST::Instruction &Instr) {

  const uint32_t BitWidth = sizeof(I) * 8;
  ValVariant &RawAddress = StackMgr.getTop();
  uint32_t Address = RawAddress.get<uint32_t>();
  if ((Address & ((BitWidth >> 3U) - 1)) != 0) {
    spdlog::error(ErrCode::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::UnalignedAtomicAccess);
  }

  // make sure the address no OOB with size I
  I *RawPointer = MemInst.getPointer<I *>(Address);
  if (!RawPointer) {
    spdlog::error(ErrCode::MemoryOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  auto *AtomicObj =
      static_cast<std::atomic<I> *>(reinterpret_cast<void *>(RawPointer));

  I Return = AtomicObj->load();
  RawAddress.emplace<T>(static_cast<T>(Return));
  return {};
}

template <typename T, typename I>
TypeT<T> Executor::runAtomicStoreOp(Runtime::StackManager &StackMgr,
                                    Runtime::Instance::MemoryInstance &MemInst,
                                    const AST::Instruction &Instr) {

  const uint32_t BitWidth = sizeof(I) * 8;
  ValVariant RawValue = StackMgr.pop();
  ValVariant RawAddress = StackMgr.pop();
  uint32_t Address = RawAddress.get<uint32_t>();
  if ((Address & ((BitWidth >> 3U) - 1)) != 0) {
    spdlog::error(ErrCode::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::UnalignedAtomicAccess);
  }

  // make sure the address no OOB with size I
  I *RawPointer = MemInst.getPointer<I *>(Address);
  if (!RawPointer) {
    spdlog::error(ErrCode::MemoryOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  auto *AtomicObj =
      static_cast<std::atomic<I> *>(reinterpret_cast<void *>(RawPointer));
  I Value = static_cast<I>(RawValue.get<T>());

  AtomicObj->store(Value);
  return {};
}

template <typename T, typename I>
TypeT<T> Executor::runAtomicAddOp(Runtime::StackManager &StackMgr,
                                  Runtime::Instance::MemoryInstance &MemInst,
                                  const AST::Instruction &Instr) {
  const uint32_t BitWidth = sizeof(I) * 8;
  ValVariant RawValue = StackMgr.pop();
  ValVariant &RawAddress = StackMgr.getTop();
  uint32_t Address = RawAddress.get<uint32_t>();
  if ((Address & ((BitWidth >> 3U) - 1)) != 0) {
    spdlog::error(ErrCode::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::UnalignedAtomicAccess);
  }

  // make sure the address no OOB with size I
  I *RawPointer = MemInst.getPointer<I *>(Address);
  if (!RawPointer) {
    spdlog::error(ErrCode::MemoryOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  auto *AtomicObj =
      static_cast<std::atomic<I> *>(reinterpret_cast<void *>(RawPointer));
  I Value = static_cast<I>(RawValue.get<T>());

  I Return = AtomicObj->fetch_add(Value);
  RawAddress.emplace<T>(static_cast<T>(Return));
  return {};
}

template <typename T, typename I>
TypeT<T> Executor::runAtomicSubOp(Runtime::StackManager &StackMgr,
                                  Runtime::Instance::MemoryInstance &MemInst,
                                  const AST::Instruction &Instr) {
  const uint32_t BitWidth = sizeof(I) * 8;
  ValVariant RawValue = StackMgr.pop();
  ValVariant &RawAddress = StackMgr.getTop();
  uint32_t Address = RawAddress.get<uint32_t>();
  if ((Address & ((BitWidth >> 3U) - 1)) != 0) {
    spdlog::error(ErrCode::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::UnalignedAtomicAccess);
  }

  // make sure the address no OOB with size I
  I *RawPointer = MemInst.getPointer<I *>(Address);
  if (!RawPointer) {
    spdlog::error(ErrCode::MemoryOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  auto *AtomicObj =
      static_cast<std::atomic<I> *>(reinterpret_cast<void *>(RawPointer));
  I Value = static_cast<I>(RawValue.get<T>());

  I Return = AtomicObj->fetch_sub(Value);
  RawAddress.emplace<T>(static_cast<T>(Return));
  return {};
}

template <typename T, typename I>
TypeT<T> Executor::runAtomicOrOp(Runtime::StackManager &StackMgr,
                                 Runtime::Instance::MemoryInstance &MemInst,
                                 const AST::Instruction &Instr) {
  const uint32_t BitWidth = sizeof(I) * 8;
  ValVariant RawValue = StackMgr.pop();
  ValVariant &RawAddress = StackMgr.getTop();
  uint32_t Address = RawAddress.get<uint32_t>();
  if ((Address & ((BitWidth >> 3U) - 1)) != 0) {
    spdlog::error(ErrCode::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::UnalignedAtomicAccess);
  }

  // make sure the address no OOB with size I
  I *RawPointer = MemInst.getPointer<I *>(Address);
  if (!RawPointer) {
    spdlog::error(ErrCode::MemoryOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  auto *AtomicObj =
      static_cast<std::atomic<I> *>(reinterpret_cast<void *>(RawPointer));
  I Value = static_cast<I>(RawValue.get<T>());

  I Return = AtomicObj->fetch_or(Value);
  RawAddress.emplace<T>(static_cast<T>(Return));
  return {};
}

template <typename T, typename I>
TypeT<T> Executor::runAtomicAndOp(Runtime::StackManager &StackMgr,
                                  Runtime::Instance::MemoryInstance &MemInst,
                                  const AST::Instruction &Instr) {
  const uint32_t BitWidth = sizeof(I) * 8;
  ValVariant RawValue = StackMgr.pop();
  ValVariant &RawAddress = StackMgr.getTop();
  uint32_t Address = RawAddress.get<uint32_t>();
  if ((Address & ((BitWidth >> 3U) - 1)) != 0) {
    spdlog::error(ErrCode::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::UnalignedAtomicAccess);
  }

  // make sure the address no OOB with size I
  I *RawPointer = MemInst.getPointer<I *>(Address);
  if (!RawPointer) {
    spdlog::error(ErrCode::MemoryOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  auto *AtomicObj =
      static_cast<std::atomic<I> *>(reinterpret_cast<void *>(RawPointer));
  I Value = static_cast<I>(RawValue.get<T>());

  I Return = AtomicObj->fetch_and(Value);
  RawAddress.emplace<T>(static_cast<T>(Return));
  return {};
}

template <typename T, typename I>
TypeT<T> Executor::runAtomicXorOp(Runtime::StackManager &StackMgr,
                                  Runtime::Instance::MemoryInstance &MemInst,
                                  const AST::Instruction &Instr) {
  const uint32_t BitWidth = sizeof(I) * 8;
  ValVariant RawValue = StackMgr.pop();
  ValVariant &RawAddress = StackMgr.getTop();
  uint32_t Address = RawAddress.get<uint32_t>();
  if ((Address & ((BitWidth >> 3U) - 1)) != 0) {
    spdlog::error(ErrCode::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::UnalignedAtomicAccess);
  }

  // make sure the address no OOB with size I
  I *RawPointer = MemInst.getPointer<I *>(Address);
  if (!RawPointer) {
    spdlog::error(ErrCode::MemoryOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  auto *AtomicObj =
      static_cast<std::atomic<I> *>(reinterpret_cast<void *>(RawPointer));
  I Value = static_cast<I>(RawValue.get<T>());

  I Return = AtomicObj->fetch_xor(Value);
  RawAddress.emplace<T>(static_cast<T>(Return));
  return {};
}

template <typename T, typename I>
TypeT<T>
Executor::runAtomicExchangeOp(Runtime::StackManager &StackMgr,
                              Runtime::Instance::MemoryInstance &MemInst,
                              const AST::Instruction &Instr) {

  const uint32_t BitWidth = sizeof(I) * 8;
  ValVariant RawValue = StackMgr.pop();
  ValVariant &RawAddress = StackMgr.getTop();
  uint32_t Address = RawAddress.get<uint32_t>();
  if ((Address & ((BitWidth >> 3U) - 1)) != 0) {
    spdlog::error(ErrCode::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::UnalignedAtomicAccess);
  }

  // make sure the address no OOB with size I
  I *RawPointer = MemInst.getPointer<I *>(Address);
  if (!RawPointer) {
    spdlog::error(ErrCode::MemoryOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  auto *AtomicObj =
      static_cast<std::atomic<I> *>(reinterpret_cast<void *>(RawPointer));
  I Value = static_cast<I>(RawValue.get<T>());

  I Return = AtomicObj->exchange(Value);
  RawAddress.emplace<T>(static_cast<T>(Return));
  return {};
}

template <typename T, typename I>
TypeT<T>
Executor::runAtomicCompareExchangeOp(Runtime::StackManager &StackMgr,
                                     Runtime::Instance::MemoryInstance &MemInst,
                                     const AST::Instruction &Instr) {

  const uint32_t BitWidth = sizeof(I) * 8;
  ValVariant RawReplacement = StackMgr.pop();
  ValVariant RawExpected = StackMgr.pop();
  ValVariant &RawAddress = StackMgr.getTop();
  uint32_t Address = RawAddress.get<uint32_t>();
  if ((Address & ((BitWidth >> 3U) - 1)) != 0) {
    spdlog::error(ErrCode::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::UnalignedAtomicAccess);
  }

  // make sure the address no OOB with size I
  I *RawPointer = MemInst.getPointer<I *>(Address);
  if (!RawPointer) {
    spdlog::error(ErrCode::MemoryOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  auto *AtomicObj =
      static_cast<std::atomic<I> *>(reinterpret_cast<void *>(RawPointer));
  I Replacement = static_cast<I>(RawReplacement.get<T>());
  I Expected = static_cast<I>(RawExpected.get<T>());

  AtomicObj->compare_exchange_strong(Expected, Replacement);
  RawAddress.emplace<T>(static_cast<T>(Expected));
  return {};
}

} // namespace Executor
} // namespace WasmEdge