#include "executor/engine/atomic_helper.h"
#include "executor/executor.h"
#include "runtime/instance/memory.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

template <typename T>
TypeT<T> Executor::runAtomicWaitOp(Runtime::StackManager &StackMgr,
                                   Runtime::Instance::MemoryInstance &MemInst,
                                   const AST::Instruction &Instr,
                                   const uint32_t BitWidth) {
  detail::atomicLock();
  ValVariant Address = StackMgr.pop();
  ValVariant Val = StackMgr.pop();
  ValVariant Timeout = StackMgr.pop();

  StackMgr.push(Address);
  runLoadOp<T>(StackMgr, MemInst, Instr, BitWidth);
  ValVariant &Loaded = StackMgr.getTop();

  if (Loaded.get<T>() == Val.get<T>()) {
    StackMgr.push(ValVariant(0));
  } else {
    StackMgr.push(ValVariant(1));
  }
  // TODO: Implement Timeout
  detail::atomicUnlock();
  return {};
}

template <typename T>
TypeT<T> Executor::runAtomicLoadOp(Runtime::StackManager &StackMgr,
                                   Runtime::Instance::MemoryInstance &MemInst,
                                   const AST::Instruction &Instr,
                                   const uint32_t BitWidth) {
  detail::atomicLock();
  ValVariant &Address = StackMgr.getTop();
  if ((Address.get<uint32_t>() & ((BitWidth >> 3U) - 1)) != 0) {
    spdlog::error(ErrCode::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    detail::atomicUnlock();
    return Unexpect(ErrCode::UnalignedAtomicAccess);
  }
  runLoadOp<T>(StackMgr, MemInst, Instr, BitWidth);
  detail::atomicUnlock();
  return {};
}

template <typename T>
TypeT<T> Executor::runAtomicStoreOp(Runtime::StackManager &StackMgr,
                                    Runtime::Instance::MemoryInstance &MemInst,
                                    const AST::Instruction &Instr,
                                    const uint32_t BitWidth) {
  detail::atomicLock();
  ValVariant &Address = StackMgr.getBottomN(2);
  if ((Address.get<uint32_t>() & ((BitWidth >> 3U) - 1)) != 0) {
    spdlog::error(ErrCode::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    detail::atomicUnlock();
    return Unexpect(ErrCode::UnalignedAtomicAccess);
  }
  typedef typename std::make_unsigned<T>::type UT;
  runStoreOp<UT>(StackMgr, MemInst, Instr, BitWidth);
  detail::atomicUnlock();
  return {};
}

template <typename T>
TypeT<T> Executor::runAtomicAddOp(Runtime::StackManager &StackMgr,
                                  Runtime::Instance::MemoryInstance &MemInst,
                                  const AST::Instruction &Instr,
                                  const uint32_t BitWidth) {
  detail::atomicLock();
  ValVariant RHS = StackMgr.pop();
  ValVariant Address = StackMgr.getTop();
  if ((Address.get<uint32_t>() & ((BitWidth >> 3U) - 1)) != 0) {
    spdlog::error(ErrCode::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    detail::atomicUnlock();
    return Unexpect(ErrCode::UnalignedAtomicAccess);
  }
  runLoadOp<T>(StackMgr, MemInst, Instr, BitWidth);

  typedef typename std::make_unsigned<T>::type UT;
  ValVariant Loaded = StackMgr.getTop();
  ValVariant &Val = StackMgr.getTop();
  runAddOp<UT>(Val, RHS);

  ValVariant Result = StackMgr.pop();
  StackMgr.push(Address);
  StackMgr.push(Result);
  runStoreOp<UT>(StackMgr, MemInst, Instr, BitWidth);

  StackMgr.push(Loaded);
  detail::atomicUnlock();
  return {};
}

template <typename T>
TypeT<T> Executor::runAtomicSubOp(Runtime::StackManager &StackMgr,
                                  Runtime::Instance::MemoryInstance &MemInst,
                                  const AST::Instruction &Instr,
                                  const uint32_t BitWidth) {
  detail::atomicLock();
  ValVariant RHS = StackMgr.pop();
  ValVariant Address = StackMgr.getTop();
  if ((Address.get<uint32_t>() & ((BitWidth >> 3U) - 1)) != 0) {
    spdlog::error(ErrCode::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    detail::atomicUnlock();
    return Unexpect(ErrCode::UnalignedAtomicAccess);
  }
  runLoadOp<T>(StackMgr, MemInst, Instr, BitWidth);

  typedef typename std::make_unsigned<T>::type UT;
  ValVariant Loaded = StackMgr.getTop();
  ValVariant &Val = StackMgr.getTop();
  runSubOp<UT>(Val, RHS);

  ValVariant Result = StackMgr.pop();
  StackMgr.push(Address);
  StackMgr.push(Result);
  runStoreOp<UT>(StackMgr, MemInst, Instr, BitWidth);

  StackMgr.push(Loaded);
  detail::atomicUnlock();
  return {};
}

template <typename T>
TypeT<T> Executor::runAtomicOrOp(Runtime::StackManager &StackMgr,
                                 Runtime::Instance::MemoryInstance &MemInst,
                                 const AST::Instruction &Instr,
                                 const uint32_t BitWidth) {
  detail::atomicLock();
  ValVariant RHS = StackMgr.pop();
  ValVariant Address = StackMgr.getTop();
  if ((Address.get<uint32_t>() & ((BitWidth >> 3U) - 1)) != 0) {
    spdlog::error(ErrCode::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    detail::atomicUnlock();
    return Unexpect(ErrCode::UnalignedAtomicAccess);
  }
  runLoadOp<T>(StackMgr, MemInst, Instr, BitWidth);

  typedef typename std::make_unsigned<T>::type UT;
  ValVariant Loaded = StackMgr.getTop();
  ValVariant &Val = StackMgr.getTop();
  runOrOp<UT>(Val, RHS);

  ValVariant Result = StackMgr.pop();
  StackMgr.push(Address);
  StackMgr.push(Result);
  runStoreOp<UT>(StackMgr, MemInst, Instr, BitWidth);

  StackMgr.push(Loaded);
  detail::atomicUnlock();
  return {};
}

template <typename T>
TypeT<T> Executor::runAtomicAndOp(Runtime::StackManager &StackMgr,
                                  Runtime::Instance::MemoryInstance &MemInst,
                                  const AST::Instruction &Instr,
                                  const uint32_t BitWidth) {
  detail::atomicLock();
  ValVariant RHS = StackMgr.pop();
  ValVariant Address = StackMgr.getTop();
  if ((Address.get<uint32_t>() & ((BitWidth >> 3U) - 1)) != 0) {
    spdlog::error(ErrCode::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    detail::atomicUnlock();
    return Unexpect(ErrCode::UnalignedAtomicAccess);
  }
  runLoadOp<T>(StackMgr, MemInst, Instr, BitWidth);

  typedef typename std::make_unsigned<T>::type UT;
  ValVariant Loaded = StackMgr.getTop();
  ValVariant &Val = StackMgr.getTop();
  runAndOp<UT>(Val, RHS);

  ValVariant Result = StackMgr.pop();
  StackMgr.push(Address);
  StackMgr.push(Result);
  runStoreOp<UT>(StackMgr, MemInst, Instr, BitWidth);

  StackMgr.push(Loaded);
  detail::atomicUnlock();
  return {};
}

template <typename T>
TypeT<T> Executor::runAtomicXorOp(Runtime::StackManager &StackMgr,
                                  Runtime::Instance::MemoryInstance &MemInst,
                                  const AST::Instruction &Instr,
                                  const uint32_t BitWidth) {
  detail::atomicLock();
  ValVariant RHS = StackMgr.pop();
  ValVariant Address = StackMgr.getTop();
  if ((Address.get<uint32_t>() & ((BitWidth >> 3U) - 1)) != 0) {
    spdlog::error(ErrCode::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    detail::atomicUnlock();
    return Unexpect(ErrCode::UnalignedAtomicAccess);
  }
  runLoadOp<T>(StackMgr, MemInst, Instr, BitWidth);

  typedef typename std::make_unsigned<T>::type UT;
  ValVariant Loaded = StackMgr.getTop();
  ValVariant &Val = StackMgr.getTop();
  runXorOp<UT>(Val, RHS);

  ValVariant Result = StackMgr.pop();
  StackMgr.push(Address);
  StackMgr.push(Result);
  runStoreOp<UT>(StackMgr, MemInst, Instr, BitWidth);

  StackMgr.push(Loaded);
  detail::atomicUnlock();
  return {};
}

template <typename T>
TypeT<T> Executor::runAtomicExchangeOp(
    Runtime::StackManager &StackMgr, Runtime::Instance::MemoryInstance &MemInst,
    const AST::Instruction &Instr, const uint32_t BitWidth) {
  detail::atomicLock();
  ValVariant RHS = StackMgr.pop();
  ValVariant Address = StackMgr.getTop();
  if ((Address.get<uint32_t>() & ((BitWidth >> 3U) - 1)) != 0) {
    spdlog::error(ErrCode::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    detail::atomicUnlock();
    return Unexpect(ErrCode::UnalignedAtomicAccess);
  }
  runLoadOp<T>(StackMgr, MemInst, Instr, BitWidth);

  typedef typename std::make_unsigned<T>::type UT;
  ValVariant Loaded = StackMgr.pop();
  StackMgr.push(Address);
  StackMgr.push(RHS);
  runStoreOp<UT>(StackMgr, MemInst, Instr, BitWidth);

  StackMgr.push(Loaded);
  detail::atomicUnlock();
  return {};
}

template <typename T>
TypeT<T> Executor::runAtomicCompareExchangeOp(
    Runtime::StackManager &StackMgr, Runtime::Instance::MemoryInstance &MemInst,
    const AST::Instruction &Instr, const uint32_t BitWidth) {
  detail::atomicLock();
  ValVariant Val = StackMgr.pop();
  ValVariant Cmp = StackMgr.pop();
  ValVariant Address = StackMgr.getTop();
  if ((Address.get<uint32_t>() & ((BitWidth >> 3U) - 1)) != 0) {
    spdlog::error(ErrCode::UnalignedAtomicAccess);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    detail::atomicUnlock();
    return Unexpect(ErrCode::UnalignedAtomicAccess);
  }
  runLoadOp<T>(StackMgr, MemInst, Instr, BitWidth);
  ValVariant Loaded = StackMgr.pop();

  typedef typename std::make_unsigned<T>::type UT;
  if (Loaded.get<T>() == Cmp.get<T>()) {
    StackMgr.push(Address);
    StackMgr.push(Val);
    runStoreOp<UT>(StackMgr, MemInst, Instr, BitWidth);
  }

  StackMgr.push(Loaded);
  detail::atomicUnlock();
  return {};
}

} // namespace Executor
} // namespace WasmEdge