// SPDX-License-Identifier: Apache-2.0

#include "executor/executor.h"
#include "runtime/instance/memory.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

template <typename T>
TypeT<T> Executor::runLoadOp(Runtime::Instance::MemoryInstance &MemInst,
                             const AST::Instruction &Instr,
                             const uint32_t BitWidth) {
  /// Calculate EA
  ValVariant &Val = StackMgr.getTop();
  if (Val.get<uint32_t>() >
      std::numeric_limits<uint32_t>::max() - Instr.getMemoryOffset()) {
    spdlog::error(ErrCode::MemoryOutOfBounds);
    spdlog::error(ErrInfo::InfoBoundary(
        Val.get<uint32_t>() + static_cast<uint64_t>(Instr.getMemoryOffset()),
        BitWidth / 8, MemInst.getBoundIdx()));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  uint32_t EA = Val.get<uint32_t>() + Instr.getMemoryOffset();

  /// Value = Mem.Data[EA : N / 8]
  if (auto Res = MemInst.loadValue(Val.emplace<T>(), EA, BitWidth / 8); !Res) {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(Res);
  }
  return {};
}

template <typename T>
TypeN<T> Executor::runStoreOp(Runtime::Instance::MemoryInstance &MemInst,
                              const AST::Instruction &Instr,
                              const uint32_t BitWidth) {
  /// Pop the value t.const c from the Stack
  T C = StackMgr.pop().get<T>();

  /// Calculate EA = i + offset
  uint32_t I = StackMgr.pop().get<uint32_t>();
  if (I > std::numeric_limits<uint32_t>::max() - Instr.getMemoryOffset()) {
    spdlog::error(ErrCode::MemoryOutOfBounds);
    spdlog::error(ErrInfo::InfoBoundary(
        I + static_cast<uint64_t>(Instr.getMemoryOffset()), BitWidth / 8,
        MemInst.getBoundIdx()));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  uint32_t EA = I + Instr.getMemoryOffset();

  /// Store value to bytes.
  if (auto Res = MemInst.storeValue(C, EA, BitWidth / 8); !Res) {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(Res);
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void>
Executor::runLoadExpandOp(Runtime::Instance::MemoryInstance &MemInst,
                          const AST::Instruction &Instr) {
  static_assert(sizeof(TOut) == sizeof(TIn) * 2);
  /// Calculate EA
  ValVariant &Val = StackMgr.getTop();
  if (Val.get<uint32_t>() >
      std::numeric_limits<uint32_t>::max() - Instr.getMemoryOffset()) {
    spdlog::error(ErrCode::MemoryOutOfBounds);
    spdlog::error(ErrInfo::InfoBoundary(
        Val.get<uint32_t>() + static_cast<uint64_t>(Instr.getMemoryOffset()), 8,
        MemInst.getBoundIdx()));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  uint32_t EA = Val.get<uint32_t>() + Instr.getMemoryOffset();

  /// Value = Mem.Data[EA : N / 8]
  uint64_t Buffer;
  if (auto Res = MemInst.loadValue(Buffer, EA, 8); !Res) {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(Res);
  }

  using VTIn [[gnu::vector_size(8)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;

  VTIn Value;
  std::memcpy(&Value, &Buffer, 8);

  if constexpr (sizeof(TOut) == 2) {
    Val.emplace<VTOut>(VTOut{Value[0], Value[1], Value[2], Value[3], Value[4],
                             Value[5], Value[6], Value[7]});
  } else if constexpr (sizeof(TOut) == 4) {
    Val.emplace<VTOut>(VTOut{Value[0], Value[1], Value[2], Value[3]});
  } else if constexpr (sizeof(TOut) == 8) {
    Val.emplace<VTOut>(VTOut{Value[0], Value[1]});
  }
  return {};
}

template <typename T>
Expect<void>
Executor::runLoadSplatOp(Runtime::Instance::MemoryInstance &MemInst,
                         const AST::Instruction &Instr) {
  /// Calculate EA
  ValVariant &Val = StackMgr.getTop();
  if (Val.get<uint32_t>() >
      std::numeric_limits<uint32_t>::max() - Instr.getMemoryOffset()) {
    spdlog::error(ErrCode::MemoryOutOfBounds);
    spdlog::error(ErrInfo::InfoBoundary(
        Val.get<uint32_t>() + static_cast<uint64_t>(Instr.getMemoryOffset()),
        sizeof(T), MemInst.getBoundIdx()));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  uint32_t EA = Val.get<uint32_t>() + Instr.getMemoryOffset();

  /// Value = Mem.Data[EA : N / 8]
  using VT [[gnu::vector_size(16)]] = T;
  uint64_t Buffer;
  if (auto Res = MemInst.loadValue(Buffer, EA, sizeof(T)); !Res) {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(Res);
  }
  const T Part = static_cast<T>(Buffer);

  if constexpr (sizeof(T) == 1) {
    Val.emplace<VT>(VT{Part, Part, Part, Part, Part, Part, Part, Part, Part,
                       Part, Part, Part, Part, Part, Part, Part});
  } else if constexpr (sizeof(T) == 2) {
    Val.emplace<VT>(VT{Part, Part, Part, Part, Part, Part, Part, Part});
  } else if constexpr (sizeof(T) == 4) {
    Val.emplace<VT>(VT{Part, Part, Part, Part});
  } else if constexpr (sizeof(T) == 8) {
    Val.emplace<VT>(VT{Part, Part});
  }
  return {};
}

template <typename T>
Expect<void> Executor::runLoadLaneOp(Runtime::Instance::MemoryInstance &MemInst,
                                     const AST::Instruction &Instr) {
  using VT [[gnu::vector_size(16)]] = T;
  VT Result = StackMgr.pop().get<VT>();

  /// Calculate EA
  ValVariant &Val = StackMgr.getTop();
  const uint32_t Offset = Val.get<uint32_t>();
  if (Offset > std::numeric_limits<uint32_t>::max() - Instr.getMemoryOffset()) {
    spdlog::error(ErrCode::MemoryOutOfBounds);
    spdlog::error(ErrInfo::InfoBoundary(
        Offset + static_cast<uint64_t>(Instr.getMemoryOffset()), sizeof(T),
        MemInst.getBoundIdx()));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  const uint32_t EA = Offset + Instr.getMemoryOffset();

  /// Value = Mem.Data[EA : N / 8]
  uint64_t Buffer;
  if (auto Res = MemInst.loadValue(Buffer, EA, sizeof(T)); !Res) {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(Res);
  }

  Result[Instr.getMemoryLane()] = static_cast<T>(Buffer);
  Val.emplace<VT>(Result);
  return {};
}

template <typename T>
Expect<void>
Executor::runStoreLaneOp(Runtime::Instance::MemoryInstance &MemInst,
                         const AST::Instruction &Instr) {
  using VT [[gnu::vector_size(16)]] = T;
  using TBuf = std::conditional_t<sizeof(T) < 4, uint32_t, T>;
  const TBuf C = StackMgr.pop().get<VT>()[Instr.getMemoryLane()];

  /// Calculate EA = i + offset
  uint32_t I = StackMgr.pop().get<uint32_t>();
  if (I > std::numeric_limits<uint32_t>::max() - Instr.getMemoryOffset()) {
    spdlog::error(ErrCode::MemoryOutOfBounds);
    spdlog::error(ErrInfo::InfoBoundary(
        I + static_cast<uint64_t>(Instr.getMemoryOffset()), sizeof(T),
        MemInst.getBoundIdx()));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  uint32_t EA = I + Instr.getMemoryOffset();

  /// Store value to bytes.
  if (auto Res = MemInst.storeValue(C, EA, sizeof(T)); !Res) {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(Res);
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
