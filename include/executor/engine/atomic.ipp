#include "executor/engine/atomic_helper.h"
#include "executor/executor.h"
#include "runtime/instance/memory.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {


template <typename T>
TypeT<T> Executor::runAtomicLoadOp(Runtime::Instance::MemoryInstance &MemInst,
                             const AST::Instruction &Instr,
                             const uint32_t BitWidth) {
  detail::atomicLock();
  runLoadOp<T>(MemInst, Instr, BitWidth);
  detail::atomicUnlock();
  return {};
}

template <typename T>
TypeT<T> Executor::runAtomicStoreOp(Runtime::Instance::MemoryInstance &MemInst,
                              const AST::Instruction &Instr,
                              const uint32_t BitWidth) {
  detail::atomicLock();
  typedef typename std::make_unsigned<T>::type UT;
  runStoreOp<UT>(MemInst, Instr, BitWidth);
  detail::atomicUnlock();
  return {};
}

template <typename T>
TypeT<T> Executor::runAtomicAddOp(Runtime::Instance::MemoryInstance &MemInst,
                              const AST::Instruction &Instr,
                              const uint32_t BitWidth) {
  detail::atomicLock();
  ValVariant RHS = StackMgr.pop();
  runLoadOp<T>(MemInst, Instr, BitWidth);
  ValVariant &Val = StackMgr.getTop();
  typedef typename std::make_unsigned<T>::type UT;
  runAddOp<UT>(Val, RHS);
  runStoreOp<UT>(MemInst, Instr, BitWidth);
  detail::atomicUnlock();
  return {};
}

template <typename T>
TypeT<T> Executor::runAtomicSubOp(Runtime::Instance::MemoryInstance &MemInst,
                              const AST::Instruction &Instr,
                              const uint32_t BitWidth) {
  detail::atomicLock();
  ValVariant RHS = StackMgr.pop();
  runLoadOp<T>(MemInst, Instr, BitWidth);
  ValVariant &Val = StackMgr.getTop();
  typedef typename std::make_unsigned<T>::type UT;
  runSubOp<UT>(Val, RHS);
  runStoreOp<UT>(MemInst, Instr, BitWidth);
  detail::atomicUnlock();
  return {};
}

template <typename T>
TypeT<T> Executor::runAtomicOrOp(Runtime::Instance::MemoryInstance &MemInst,
                              const AST::Instruction &Instr,
                              const uint32_t BitWidth) {
  detail::atomicLock();
  ValVariant RHS = StackMgr.pop();
  runLoadOp<T>(MemInst, Instr, BitWidth);
  ValVariant &Val = StackMgr.getTop();
  typedef typename std::make_unsigned<T>::type UT;
  runOrOp<UT>(Val, RHS);
  runStoreOp<UT>(MemInst, Instr, BitWidth);
  detail::atomicUnlock();
  return {};
}

template <typename T>
TypeT<T> Executor::runAtomicAndOp(Runtime::Instance::MemoryInstance &MemInst,
                              const AST::Instruction &Instr,
                              const uint32_t BitWidth) {
  detail::atomicLock();
  ValVariant RHS = StackMgr.pop();
  runLoadOp<T>(MemInst, Instr, BitWidth);
  ValVariant &Val = StackMgr.getTop();
  typedef typename std::make_unsigned<T>::type UT;
  runAndOp<UT>(Val, RHS);
  runStoreOp<UT>(MemInst, Instr, BitWidth);
  detail::atomicUnlock();
  return {};
}

template <typename T>
TypeT<T> Executor::runAtomicXorOp(Runtime::Instance::MemoryInstance &MemInst,
                              const AST::Instruction &Instr,
                              const uint32_t BitWidth) {
  detail::atomicLock();
  ValVariant RHS = StackMgr.pop();
  runLoadOp<T>(MemInst, Instr, BitWidth);
  ValVariant &Val = StackMgr.getTop();
  typedef typename std::make_unsigned<T>::type UT;
  runXorOp<UT>(Val, RHS);
  runStoreOp<UT>(MemInst, Instr, BitWidth);
  detail::atomicUnlock();
  return {};
}

template <typename T>
TypeT<T> Executor::runAtomicExchangeOp(Runtime::Instance::MemoryInstance &MemInst,
                              const AST::Instruction &Instr,
                              const uint32_t BitWidth) {
  detail::atomicLock();
  runLoadOp<T>(MemInst, Instr, BitWidth);
  ValVariant RHS = StackMgr.pop();
  typedef typename std::make_unsigned<T>::type UT;
  runStoreOp<UT>(MemInst, Instr, BitWidth);
  StackMgr.push(RHS);
  detail::atomicUnlock();
  return {};
}

template <typename T>
TypeT<T> Executor::runAtomicCompareExchangeOp(Runtime::Instance::MemoryInstance &MemInst,
                              const AST::Instruction &Instr,
                              const uint32_t BitWidth) {
  detail::atomicLock();
  ValVariant Expected = StackMgr.pop();
  runLoadOp<T>(MemInst, Instr, BitWidth);
  ValVariant Loaded = StackMgr.pop();
  typedef typename std::make_unsigned<T>::type UT;
  if(Loaded.get<T>() == Expected.get<T>()){
    runStoreOp<UT>(MemInst, Instr, BitWidth);
  }else{
    ValVariant ToStore = StackMgr.pop();
  }
  StackMgr.push(Loaded);
  detail::atomicUnlock();
  return {};
}


} // namespace Executor
} // namespace WasmEdge