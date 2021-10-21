// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/ast/instruction.h - Instruction class definition ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Instruction node class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/enum_ast.h"
#include "common/span.h"
#include "common/types.h"

#include <vector>

namespace WasmEdge {
namespace AST {

/// Instruction node class.
class Instruction {
public:
  /// Constructor assigns the OpCode.
  Instruction(OpCode Byte, uint32_t Off = 0) noexcept
      : Code(Byte), Offset(Off) {}
  /// Copy constructor.
  Instruction(const Instruction &Instr) = default;
  /// Destructor.
  ~Instruction() noexcept = default;

  /// Getter of OpCode.
  OpCode getOpCode() const noexcept { return Code; }

  /// Getter of Offset.
  uint32_t getOffset() const noexcept { return Offset; }

  /// Getter and setter of block type.
  BlockType getBlockType() const noexcept { return ResType; }
  void setBlockType(BlockType BType) noexcept { ResType = BType; }

  /// Getter and setter of jump count to End instruction.
  uint32_t getJumpEnd() const noexcept { return JumpEnd; }
  void setJumpEnd(const uint32_t Cnt) noexcept { JumpEnd = Cnt; }

  /// Getter and setter of jump count to Else instruction.
  uint32_t getJumpElse() const noexcept { return JumpElse; }
  void setJumpElse(const uint32_t Cnt) noexcept { JumpElse = Cnt; }

  /// Getter and setter of reference type.
  RefType getRefType() const noexcept { return ReferenceType; }
  void setRefType(RefType RType) noexcept { ReferenceType = RType; }

  /// Getter of label list.
  Span<const uint32_t> getLabelList() const noexcept { return LabelList; }
  std::vector<uint32_t> &getLabelList() noexcept { return LabelList; }

  /// Getter of selecting value types list.
  Span<const ValType> getValTypeList() const noexcept { return ValTypeList; }
  std::vector<ValType> &getValTypeList() noexcept { return ValTypeList; }

  /// Getter and setter of target index.
  uint32_t getTargetIndex() const noexcept { return TargetIdx; }
  uint32_t &getTargetIndex() noexcept { return TargetIdx; }

  /// Getter and setter of source index.
  uint32_t getSourceIndex() const noexcept { return SourceIdx; }
  uint32_t &getSourceIndex() noexcept { return SourceIdx; }

  /// Getter and setter of memory alignment.
  uint32_t getMemoryAlign() const noexcept { return MemAlign; }
  uint32_t &getMemoryAlign() noexcept { return MemAlign; }

  /// Getter of memory offset.
  uint32_t getMemoryOffset() const noexcept { return MemOffset; }
  uint32_t &getMemoryOffset() noexcept { return MemOffset; }

  /// Getter and setter of the constant value.
  ValVariant getNum() const noexcept { return Num; }
  void setNum(ValVariant N) noexcept { Num = N; }

private:
  /// \name Data of instructions.
  /// @{
  const OpCode Code = OpCode::End;
  const uint32_t Offset = 0;
  BlockType ResType = ValType::None;
  uint32_t JumpEnd = 0;
  uint32_t JumpElse = 0;
  RefType ReferenceType = RefType::FuncRef;
  std::vector<uint32_t> LabelList;
  std::vector<ValType> ValTypeList;
  uint32_t TargetIdx = 0;
  uint32_t SourceIdx = 0;
  uint32_t MemAlign = 0;
  uint32_t MemOffset = 0;
  ValVariant Num = 0U;
  /// @}
};

/// Type aliasing
using InstrVec = std::vector<Instruction>;
using InstrView = Span<const Instruction>;

} // namespace AST
} // namespace WasmEdge
