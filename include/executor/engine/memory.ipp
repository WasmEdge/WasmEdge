// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"
#include "runtime/instance/memory.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

template <uint32_t BitWidth>
Expect<uint32_t>
Executor::calculateEA(Runtime::Instance::MemoryInstance &MemInst,
                      uint32_t Offset, const AST::Instruction &Instr) noexcept {
  if (Offset > std::numeric_limits<uint32_t>::max() - Instr.getMemoryOffset()) {
    spdlog::error(ErrCode::Value::MemoryOutOfBounds);
    spdlog::error(ErrInfo::InfoBoundary(
        Offset + static_cast<uint64_t>(Instr.getMemoryOffset()), BitWidth / 8,
        MemInst.getBoundIdx()));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  return Offset + Instr.getMemoryOffset();
}

template <typename T, uint32_t BitWidth>
TypeT<T> Executor::runLoadOp(Runtime::StackManager &StackMgr,
                             Runtime::Instance::MemoryInstance &MemInst,
                             const AST::Instruction &Instr) {
  // Calculate EA
  const uint32_t Offset = StackMgr.peekTop<uint32_t>();
  EXPECTED_TRY(uint32_t EA, calculateEA<BitWidth>(MemInst, Offset, Instr));

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
  // Pop the value t.const c and offset from the Stack
  auto [C, Offset] = StackMgr.pops<T, uint32_t>();

  // Calculate EA
  EXPECTED_TRY(uint32_t EA, calculateEA<BitWidth>(MemInst, Offset, Instr));

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
  uint32_t Offset = StackMgr.peekTop<uint32_t>();
  EXPECTED_TRY(uint32_t EA, calculateEA<64>(MemInst, Offset, Instr));

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
  uint32_t Offset = StackMgr.peekTop<uint32_t>();
  EXPECTED_TRY(uint32_t EA, calculateEA<sizeof(T) * 8>(MemInst, Offset, Instr));

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
  auto [V, Offset] = StackMgr.pops<VT, uint32_t>();
  const uint32_t Lane = Endian::native == Endian::little
                            ? Instr.getMemoryLane()
                            : (16 / sizeof(T)) - 1 - Instr.getMemoryLane();

  // Calculate EA
  EXPECTED_TRY(uint32_t EA, calculateEA<sizeof(T) * 8>(MemInst, Offset, Instr));

  // Value = Mem.Data[EA : N / 8]
  uint64_t Buffer;
  return MemInst.loadValue<decltype(Buffer), sizeof(T)>(Buffer, EA)
      .map_error([&Instr](auto E) {
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return E;
      })
      .map([&, V = V]() {
        auto Result = V;
        Result[Lane] = static_cast<T>(Buffer);
        StackMgr.emplaceTop<VT>(std::move(Result));
      });
}

template <typename T>
Expect<void>
Executor::runStoreLaneOp(Runtime::StackManager &StackMgr,
                         Runtime::Instance::MemoryInstance &MemInst,
                         const AST::Instruction &Instr) {
  using VT = SIMDArray<T, 16>;
  using TBuf = std::conditional_t<sizeof(T) < 4, uint32_t, T>;
  const auto [V, Offset] = StackMgr.pops<VT, uint32_t>();
  const uint32_t Lane = Endian::native == Endian::little
                            ? Instr.getMemoryLane()
                            : (16 / sizeof(T)) - 1 - Instr.getMemoryLane();
  const TBuf C = V[Lane];

  // Calculate EA
  EXPECTED_TRY(uint32_t EA, calculateEA<sizeof(T) * 8>(MemInst, Offset, Instr));

  // Store value to bytes.
  return MemInst.storeValue<TBuf, sizeof(T)>(C, EA).map_error([&Instr](auto E) {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return E;
  });
}

} // namespace Executor
} // namespace WasmEdge
