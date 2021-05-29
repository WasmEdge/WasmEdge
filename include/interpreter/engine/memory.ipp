// SPDX-License-Identifier: Apache-2.0
#include "ast/instruction.h"
#include "common/log.h"
#include "common/value.h"
#include "interpreter/interpreter.h"
#include "runtime/instance/memory.h"

#include <cstdint>

namespace WasmEdge {
namespace Interpreter {

template <typename T>
TypeT<T> Interpreter::runLoadOp(Runtime::Instance::MemoryInstance &MemInst,
                                const AST::Instruction &Instr,
                                const uint32_t BitWidth) {
  /// Calculate EA
  ValVariant &Val = StackMgr.getTop();
  if (retrieveValue<uint32_t>(Val) >
      std::numeric_limits<uint32_t>::max() - Instr.getMemoryOffset()) {
    spdlog::error(ErrCode::MemoryOutOfBounds);
    spdlog::error(ErrInfo::InfoBoundary(
        retrieveValue<uint32_t>(Val) +
            static_cast<uint64_t>(Instr.getMemoryOffset()),
        BitWidth / 8, MemInst.getBoundIdx()));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  uint32_t EA = retrieveValue<uint32_t>(Val) + Instr.getMemoryOffset();

  /// Value = Mem.Data[EA : N / 8]
  if (auto Res = MemInst.loadValue(retrieveValue<T>(Val), EA, BitWidth / 8);
      !Res) {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(Res);
  }
  return {};
}

template <typename T>
TypeN<T> Interpreter::runStoreOp(Runtime::Instance::MemoryInstance &MemInst,
                                 const AST::Instruction &Instr,
                                 const uint32_t BitWidth) {
  /// Pop the value t.const c from the Stack
  T C = retrieveValue<T>(StackMgr.pop());

  /// Calculate EA = i + offset
  uint32_t I = retrieveValue<uint32_t>(StackMgr.pop());
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
Interpreter::runLoadExpandOp(Runtime::Instance::MemoryInstance &MemInst,
                             const AST::Instruction &Instr) {
  static_assert(sizeof(TOut) == sizeof(TIn) * 2);
  /// Calculate EA
  ValVariant &Val = StackMgr.getTop();
  if (retrieveValue<uint32_t>(Val) >
      std::numeric_limits<uint32_t>::max() - Instr.getMemoryOffset()) {
    spdlog::error(ErrCode::MemoryOutOfBounds);
    spdlog::error(ErrInfo::InfoBoundary(
        retrieveValue<uint32_t>(Val) +
            static_cast<uint64_t>(Instr.getMemoryOffset()),
        8, MemInst.getBoundIdx()));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  uint32_t EA = retrieveValue<uint32_t>(Val) + Instr.getMemoryOffset();

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
  VTOut &Result = retrieveValue<VTOut>(Val);

  if constexpr (sizeof(TOut) == 2) {
    Result = VTOut{Value[0], Value[1], Value[2], Value[3],
                   Value[4], Value[5], Value[6], Value[7]};
  } else if constexpr (sizeof(TOut) == 4) {
    Result = VTOut{Value[0], Value[1], Value[2], Value[3]};
  } else if constexpr (sizeof(TOut) == 8) {
    Result = VTOut{Value[0], Value[1]};
  }
  return {};
}

template <typename T>
Expect<void>
Interpreter::runLoadSplatOp(Runtime::Instance::MemoryInstance &MemInst,
                            const AST::Instruction &Instr) {
  /// Calculate EA
  ValVariant &Val = StackMgr.getTop();
  if (retrieveValue<uint32_t>(Val) >
      std::numeric_limits<uint32_t>::max() - Instr.getMemoryOffset()) {
    spdlog::error(ErrCode::MemoryOutOfBounds);
    spdlog::error(ErrInfo::InfoBoundary(
        retrieveValue<uint32_t>(Val) +
            static_cast<uint64_t>(Instr.getMemoryOffset()),
        sizeof(T), MemInst.getBoundIdx()));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  uint32_t EA = retrieveValue<uint32_t>(Val) + Instr.getMemoryOffset();

  /// Value = Mem.Data[EA : N / 8]
  using VT [[gnu::vector_size(16)]] = T;
  uint64_t Buffer;
  if (auto Res = MemInst.loadValue(Buffer, EA, sizeof(T)); !Res) {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(Res);
  }
  const T Part = Buffer;

  VT &Result = retrieveValue<VT>(Val);
  if constexpr (sizeof(T) == 1) {
    Result = VT{Part, Part, Part, Part, Part, Part, Part, Part,
                Part, Part, Part, Part, Part, Part, Part, Part};
  } else if constexpr (sizeof(T) == 2) {
    Result = VT{Part, Part, Part, Part, Part, Part, Part, Part};
  } else if constexpr (sizeof(T) == 4) {
    Result = VT{Part, Part, Part, Part};
  } else if constexpr (sizeof(T) == 8) {
    Result = VT{Part, Part};
  }
  return {};
}

template <typename T>
Expect<void>
Interpreter::runLoadLaneOp(Runtime::Instance::MemoryInstance &MemInst,
                           const AST::Instruction &Instr) {
  using VT [[gnu::vector_size(16)]] = T;
  VT Result = retrieveValue<VT>(StackMgr.pop());

  /// Calculate EA
  ValVariant &Val = StackMgr.getTop();
  const uint32_t Offset = retrieveValue<uint32_t>(Val);
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

  Result[Instr.getTargetIndex()] = Buffer;
  retrieveValue<VT>(Val) = Result;
  return {};
}

template <typename T>
Expect<void>
Interpreter::runStoreLaneOp(Runtime::Instance::MemoryInstance &MemInst,
                            const AST::Instruction &Instr) {
  using VT [[gnu::vector_size(16)]] = T;
  using TBuf = std::conditional_t<sizeof(T) < 4, uint32_t, T>;
  const TBuf C = retrieveValue<VT>(StackMgr.pop())[Instr.getTargetIndex()];

  /// Calculate EA = i + offset
  uint32_t I = retrieveValue<uint32_t>(StackMgr.pop());
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

} // namespace Interpreter
} // namespace WasmEdge
