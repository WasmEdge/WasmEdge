// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"
#include "runtime/instance/memory.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

template <typename T, uint32_t BitWidth>
TypeT<T> Executor::runLoadOp(Runtime::StackManager &StackMgr,
                             Runtime::Instance::MemoryInstance &MemInst,
                             const AST::Instruction &Instr) {
  // Calculate EA
  ValVariant Val = StackMgr.peekTop<ValVariant>();
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t EA = extractAddr(Val, AddrType);
  EXPECTED_TRY(checkOffsetOverflow(MemInst, Instr, EA, BitWidth / 8));
  EA += Instr.getMemoryOffset();
  // Value = Mem.Data[EA : N / 8]
  T Buffer;
  return MemInst.loadValue<T, BitWidth / 8>(Buffer, EA)
      .map([&]() { StackMgr.emplaceTop<T>(std::move(Buffer)); })
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
  // Pop the value t.const c and address from the Stack
  auto [C, Val] = StackMgr.pops<T, ValVariant>();

  // Calculate EA = i + offset
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t I = extractAddr(Val, AddrType);
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
  ValVariant Val = StackMgr.peekTop<ValVariant>();
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

        VTIn Value;
        std::memcpy(&Value, &Buffer, 8);

        if constexpr (sizeof(TOut) == 2) {
          StackMgr.emplaceTop(VTOut{Value[0], Value[1], Value[2], Value[3],
                                    Value[4], Value[5], Value[6], Value[7]});
        } else if constexpr (sizeof(TOut) == 4) {
          StackMgr.emplaceTop(VTOut{Value[0], Value[1], Value[2], Value[3]});
        } else if constexpr (sizeof(TOut) == 8) {
          StackMgr.emplaceTop(VTOut{Value[0], Value[1]});
        }
      });
}

template <typename T>
Expect<void>
Executor::runLoadSplatOp(Runtime::StackManager &StackMgr,
                         Runtime::Instance::MemoryInstance &MemInst,
                         const AST::Instruction &Instr) {
  // Calculate EA
  ValVariant Val = StackMgr.peekTop<ValVariant>();
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

        if constexpr (sizeof(T) == 1) {
          StackMgr.emplaceTop(VT{Part, Part, Part, Part, Part, Part, Part, Part,
                                 Part, Part, Part, Part, Part, Part, Part,
                                 Part});
        } else if constexpr (sizeof(T) == 2) {
          StackMgr.emplaceTop(
              VT{Part, Part, Part, Part, Part, Part, Part, Part});
        } else if constexpr (sizeof(T) == 4) {
          StackMgr.emplaceTop(VT{Part, Part, Part, Part});
        } else if constexpr (sizeof(T) == 8) {
          StackMgr.emplaceTop(VT{Part, Part});
        }
      });
}

template <typename T>
Expect<void> Executor::runLoadLaneOp(Runtime::StackManager &StackMgr,
                                     Runtime::Instance::MemoryInstance &MemInst,
                                     const AST::Instruction &Instr) {
  using VT = SIMDArray<T, 16>;
  auto [Result, Val] = StackMgr.popsPeekTop<VT, ValVariant>();
  const uint32_t Lane = Endian::native == Endian::little
                            ? Instr.getMemoryLane()
                            : (16 / sizeof(T)) - 1 - Instr.getMemoryLane();

  // Calculate EA
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
      .map([&, R = Result]() mutable {
        R[Lane] = static_cast<T>(Buffer);
        StackMgr.emplaceTop<VT>(std::move(R));
      });
}

template <typename T>
Expect<void>
Executor::runStoreLaneOp(Runtime::StackManager &StackMgr,
                         Runtime::Instance::MemoryInstance &MemInst,
                         const AST::Instruction &Instr) {
  using VT = SIMDArray<T, 16>;
  using TBuf = std::conditional_t<sizeof(T) < 4, uint32_t, T>;
  auto [V, Val] = StackMgr.pops<VT, ValVariant>();
  const uint32_t Lane = Endian::native == Endian::little
                            ? Instr.getMemoryLane()
                            : (16 / sizeof(T)) - 1 - Instr.getMemoryLane();
  const TBuf C = V[Lane];

  // Calculate EA = i + offset
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t I = extractAddr(Val, AddrType);
  EXPECTED_TRY(checkOffsetOverflow(MemInst, Instr, I, sizeof(T)));
  uint64_t EA = I + Instr.getMemoryOffset();

  // Store value to bytes.
  return MemInst.storeValue<TBuf, sizeof(T)>(C, EA).map_error([&Instr](auto E) {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return E;
  });
}

} // namespace Executor
} // namespace WasmEdge
