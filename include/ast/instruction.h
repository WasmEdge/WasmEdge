// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/ast/instruction.h - Inst classes definition---------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Instruction node class and the
/// derived instruction classes.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <memory>
#include <vector>

#include "common/configure.h"
#include "common/enum_ast.h"
#include "common/errcode.h"
#include "common/types.h"
#include "common/variant.h"
#include "loader/filemgr.h"

namespace WasmEdge {
namespace AST {

/// Type aliasing
class Instruction;
using InstrVec = std::vector<Instruction>;
using InstrView = Span<const Instruction>;

/// Instruction node class.
class Instruction {
public:
  /// Constructor assigns the OpCode.
  Instruction(const OpCode Byte, const uint32_t Off = 0) noexcept
      : Code(Byte), Offset(Off) {}
  /// Copy constructor.
  Instruction(const Instruction &Instr) = default;

  ~Instruction() noexcept = default;

  /// Binary loading from file manager.
  Expect<void> loadBinary(FileMgr &Mgr, const Configure &Conf);

  /// Getter of OpCode.
  OpCode getOpCode() const { return Code; }

  /// Getter of Offset.
  uint32_t getOffset() const { return Offset; }

  /// Getter of block type.
  BlockType getBlockType() const { return ResType; }

  /// Getter and setter of jump count to End instruction.
  uint32_t getJumpEnd() const { return JumpEnd; }
  void setJumpEnd(const uint32_t Cnt) { JumpEnd = Cnt; }

  /// Getter and setter of jump count to Else instruction.
  uint32_t getJumpElse() const { return JumpElse; }
  void setJumpElse(const uint32_t Cnt) { JumpElse = Cnt; }

  /// Getter of reference type.
  RefType getReferenceType() const { return ReferenceType; }

  /// Getter of label list.
  Span<const uint32_t> getLabelList() const { return LabelList; }

  /// Getter of selecting value types list.
  Span<const ValType> getValTypeList() const { return ValTypeList; }

  /// Getter of target index.
  uint32_t getTargetIndex() const { return TargetIdx; }

  /// Getter of source index.
  uint32_t getSourceIndex() const { return SourceIdx; }

  /// Getter of memory alignment.
  uint32_t getMemoryAlign() const { return MemAlign; }

  /// Getter of memory offset.
  uint32_t getMemoryOffset() const { return MemOffset; }

  /// Getter of the constant value.
  ValVariant getNum() const { return Num; }

private:
  /// OpCode of this instruction node.
  const OpCode Code;
  const uint32_t Offset;
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
};

/// Load OpCode from file manager.
///
/// Read OpCode byte(s) from file manager and return OpCode.
///
/// \param Mgr the file manager object to load bytes.
/// \param Conf the WasmEdge configuration reference.
///
/// \returns OpCode if success, ErrCode when failed.
extern Expect<OpCode> loadOpCode(FileMgr &Mgr, const Configure &Conf);

/// Load instructions from file manager.
///
/// Read instructions until the End OpCode and return the vector.
///
/// \param Mgr the file manager object to load bytes.
/// \param Conf the WasmEdge configuration reference.
///
/// \returns InstrVec if success, ErrCode when failed.
extern Expect<InstrVec> loadInstrSeq(FileMgr &Mgr, const Configure &Conf);

} // namespace AST
} // namespace WasmEdge
