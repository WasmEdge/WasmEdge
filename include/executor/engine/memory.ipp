// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/engine/vector_helper.h"
#include "executor/executor.h"
#include "experimental/bit.hpp"
#include "experimental/simd/ext.hpp"
#include "runtime/instance/memory.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

template <typename T, uint32_t BitWidth>
TypeT<T> Executor::runLoadOp(Runtime::StackManager &StackMgr,
                             Runtime::Instance::MemoryInstance &MemInst,
                             const AST::Instruction &Instr) {
  // Calculate EA
  ValVariant &Val = StackMgr.getTop();
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t EA = extractAddr(Val, AddrType);
  EXPECTED_TRY(checkOffsetOverflow(MemInst, Instr, EA, BitWidth / 8));
  EA += Instr.getMemoryOffset();
  // Value = Mem.Data[EA : N / 8]
  return MemInst.loadValue<T, BitWidth / 8>(Val.emplace<T>(), EA)
      .map_error([&Instr](auto E) {
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return E;
      });
}

template <typename T, uint32_t BitWidth>
TypeN<T> Executor::runStoreOp(Runtime::StackManager &StackMgr,
                              Runtime::Instance::MemoryInstance &MemInst,
                              const AST::Instruction &Instr) {
  // Pop the value t.const c from the Stack
  T C = StackMgr.pop().get<T>();

  // Calculate EA = i + offset
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t I = extractAddr(StackMgr.pop(), AddrType);
  EXPECTED_TRY(checkOffsetOverflow(MemInst, Instr, I, BitWidth / 8));
  uint64_t EA = I + Instr.getMemoryOffset();

  // Store value to bytes.
  return MemInst.storeValue<T, BitWidth / 8>(C, EA).map_error([&Instr](auto E) {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return E;
  });
}

template <typename TIn, typename TOut>
Expect<void>
Executor::runLoadExpandOp(Runtime::StackManager &StackMgr,
                          Runtime::Instance::MemoryInstance &MemInst,
                          const AST::Instruction &Instr) {
  static_assert(sizeof(TOut) == sizeof(TIn) * 2);
  // Calculate EA
  ValVariant &Val = StackMgr.getTop();
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t EA = extractAddr(Val, AddrType);
  EXPECTED_TRY(checkOffsetOverflow(MemInst, Instr, EA, 8));
  EA += Instr.getMemoryOffset();

  // Value = Mem.Data[EA : N / 8]
  uint64_t Buffer;
  return MemInst.loadValue<decltype(Buffer), 8>(Buffer, EA)
      .map_error([&Instr](auto E) {
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return E;
      })
      .map([&]() {
        using VTIn = SIMDArray<TIn, 8>;
        using VTOut = SIMDArray<TOut, 16>;

        const auto Value = cxx20::bit_cast<VTIn>(Buffer);

        Val.emplace<VTOut>(VTOut(Value));
      });
}

template <typename T>
Expect<void>
Executor::runLoadSplatOp(Runtime::StackManager &StackMgr,
                         Runtime::Instance::MemoryInstance &MemInst,
                         const AST::Instruction &Instr) {
  // Calculate EA
  ValVariant &Val = StackMgr.getTop();
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t EA = extractAddr(Val, AddrType);
  EXPECTED_TRY(checkOffsetOverflow(MemInst, Instr, EA, sizeof(T)));
  EA += Instr.getMemoryOffset();

  // Value = Mem.Data[EA : N / 8]
  using VT = SIMDArray<T, 16>;
  uint64_t Buffer;
  return MemInst.loadValue<decltype(Buffer), sizeof(T)>(Buffer, EA)
      .map_error([&Instr](auto E) {
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return E;
      })
      .map([&]() {
        const T Part = static_cast<T>(Buffer);

        Val.emplace<VT>(VT(Part));
      });
}

template <typename T>
Expect<void> Executor::runLoadLaneOp(Runtime::StackManager &StackMgr,
                                     Runtime::Instance::MemoryInstance &MemInst,
                                     const AST::Instruction &Instr) {
  using VT = SIMDArray<T, 16>;
  VT Result = StackMgr.pop().get<VT>();
  const auto Lane = detail::lane<T>(Instr.getMemoryLane());
  // Calculate EA
  ValVariant &Val = StackMgr.getTop();
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t EA = extractAddr(Val, AddrType);
  EXPECTED_TRY(checkOffsetOverflow(MemInst, Instr, EA, sizeof(T)));
  EA += Instr.getMemoryOffset();

  // Value = Mem.Data[EA : N / 8]
  uint64_t Buffer;
  return MemInst.loadValue<decltype(Buffer), sizeof(T)>(Buffer, EA)
      .map_error([&Instr](auto E) {
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return E;
      })
      .map([&]() {
        cxx26::simd_ext::replace_lane(Result, Lane, static_cast<T>(Buffer));
        Val.emplace<VT>(Result);
      });
}

template <typename T>
Expect<void>
Executor::runStoreLaneOp(Runtime::StackManager &StackMgr,
                         Runtime::Instance::MemoryInstance &MemInst,
                         const AST::Instruction &Instr) {
  using VT = SIMDArray<T, 16>;
  using TBuf = std::conditional_t<sizeof(T) < 4, uint32_t, T>;
  const auto Lane = detail::lane<T>(Instr.getMemoryLane());
  const TBuf C = StackMgr.pop().get<VT>()[Lane];

  // Calculate EA = i + offset
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t I = extractAddr(StackMgr.pop(), AddrType);
  EXPECTED_TRY(checkOffsetOverflow(MemInst, Instr, I, sizeof(T)));
  uint64_t EA = I + Instr.getMemoryOffset();

  // Store value to bytes.
  return MemInst.storeValue<decltype(C), sizeof(T)>(C, EA).map_error(
      [&Instr](auto E) {
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return E;
      });
}

} // namespace Executor
} // namespace WasmEdge
