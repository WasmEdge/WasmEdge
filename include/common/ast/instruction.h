// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/common/ast/instruction.h - Inst classes definition------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Instruction node class and the
/// derived instruction classes.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/ast.h"
#include "common/errcode.h"
#include "common/types.h"
#include "common/value.h"
#include "loader/filemgr.h"
#include "support/variant.h"

#include <memory>
#include <vector>

namespace SSVM {
namespace AST {

/// Type aliasing
class Instruction;
using InstrVec = std::vector<std::unique_ptr<Instruction>>;
using InstrIter = InstrVec::const_iterator;

/// Loader class of Instruction node.
class Instruction {
public:
  /// Constructor assigns the OpCode.
  Instruction(const OpCode Byte, const uint32_t Off = 0)
      : Code(Byte), Offset(Off) {}
  virtual ~Instruction() noexcept = default;

  /// Binary loading from file manager. Default not load anything.
  virtual Expect<void> loadBinary(FileMgr &Mgr) { return {}; }

  /// Getter of OpCode.
  OpCode getOpCode() const { return Code; }

  /// Getter of Offset.
  uint32_t getOffset() const { return Offset; }

protected:
  /// OpCode of this instruction node.
  const OpCode Code;
  const uint32_t Offset;
};

/// Derived control instruction node.
class ControlInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  ControlInstruction(const OpCode Byte, const uint32_t Off = 0)
      : Instruction(Byte, Off) {}
  /// Copy constructor.
  ControlInstruction(const ControlInstruction &Instr)
      : Instruction(Instr.Code, Instr.Offset) {}
};

/// Derived block control instruction node.
class BlockControlInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  BlockControlInstruction(const OpCode Byte, const uint32_t Off = 0)
      : Instruction(Byte, Off) {}
  /// Copy constructor.
  BlockControlInstruction(const BlockControlInstruction &Instr);

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Instruction.
  /// Read the return type, instructions in block body.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr) override;

  /// Getter of block type
  BlockType getBlockType() const { return ResType; }

  /// Getter of Block Body
  const InstrVec &getBody() const { return Body; }

private:
  /// \name Data of block instruction: return type and block body.
  /// @{
  BlockType ResType;
  InstrVec Body;
  /// @}
}; // namespace AST

/// Derived if-else control instruction node.
class IfElseControlInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  IfElseControlInstruction(const OpCode Byte, const uint32_t Off = 0)
      : Instruction(Byte, Off) {}
  /// Copy constructor.
  IfElseControlInstruction(const IfElseControlInstruction &Instr);

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Instruction.
  /// Read the return type, instructions in If and Else statements.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr) override;

  /// Getter of block type
  BlockType getBlockType() const { return ResType; }

  /// Getter of if statement.
  const InstrVec &getIfStatement() const { return IfStatement; }

  /// Getter of else statement.
  const InstrVec &getElseStatement() const { return ElseStatement; }

private:
  /// \name Data of block instruction: return type and statements.
  /// @{
  BlockType ResType;
  InstrVec IfStatement;
  InstrVec ElseStatement;
  /// @}
};

/// Derived branch control instruction node.
class BrControlInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  BrControlInstruction(const OpCode Byte, const uint32_t Off = 0)
      : Instruction(Byte, Off) {}
  /// Copy constructor.
  BrControlInstruction(const BrControlInstruction &Instr)
      : Instruction(Instr.Code, Instr.Offset), LabelIdx(Instr.LabelIdx) {}

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Instruction.
  /// Read the branch label index.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr) override;

  /// Get label index
  uint32_t getLabelIndex() const { return LabelIdx; }

private:
  /// Branch-to label index.
  uint32_t LabelIdx = 0;
};

/// Derived branch table control instruction node.
class BrTableControlInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  BrTableControlInstruction(const OpCode Byte, const uint32_t Off = 0)
      : Instruction(Byte, Off) {}
  /// Copy constructor.
  BrTableControlInstruction(const BrTableControlInstruction &Instr)
      : Instruction(Instr.Code, Instr.Offset), LabelList(Instr.LabelList),
        LabelIdx(Instr.LabelIdx) {}

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Instruction.
  /// Read the vector of labels and default branch label of indirect branch.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr) override;

  /// Getter of label table
  Span<const uint32_t> getLabelList() const { return LabelList; }

  /// Getter of label index
  uint32_t getLabelIndex() const { return LabelIdx; }

private:
  /// \name Data of branch instruction: label vector and defalt label.
  /// @{
  std::vector<uint32_t> LabelList;
  uint32_t LabelIdx = 0;
  /// @}
};

/// Derived call control instruction node.
class CallControlInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  CallControlInstruction(const OpCode Byte, const uint32_t Off = 0)
      : Instruction(Byte, Off) {}
  /// Copy constructor.
  CallControlInstruction(const CallControlInstruction &Instr)
      : Instruction(Instr.Code, Instr.Offset), TargetIdx(Instr.TargetIdx),
        TableIdx(Instr.TableIdx) {}

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Instruction.
  /// Read the function or function type index and table index.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr) override;

  /// Getter of function/functype index
  uint32_t getTargetIndex() const { return TargetIdx; }

  /// Getter of table index
  uint32_t getTableIndex() const { return TableIdx; }

private:
  /// Call function or function type index.
  uint32_t TargetIdx = 0;
  uint32_t TableIdx = 0;
};

/// Derived reference instruction node.
class ReferenceInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  ReferenceInstruction(const OpCode Byte, const uint32_t Off = 0)
      : Instruction(Byte, Off) {}
  /// Copy constructor.
  ReferenceInstruction(const ReferenceInstruction &Instr)
      : Instruction(Instr.Code, Instr.Offset), Type(Instr.Type),
        TargetIdx(Instr.TargetIdx) {}

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Instruction.
  /// Read the reference type and reference index.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr) override;

  /// Getter of the reference type.
  RefType getReferenceType() const { return Type; }

  /// Getter of the reference index.
  uint32_t getTargetIndex() const { return TargetIdx; }

private:
  /// \name Data of reference instruction: reference index and reference type.
  /// @{
  RefType Type = RefType::FuncRef;
  uint32_t TargetIdx = 0;
  /// @}
};

/// Derived parametric instruction node.
class ParametricInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  ParametricInstruction(const OpCode Byte, const uint32_t Off = 0)
      : Instruction(Byte, Off) {}
  /// Copy constructor.
  ParametricInstruction(const ParametricInstruction &Instr)
      : Instruction(Instr.Code, Instr.Offset), ValTypeList(Instr.ValTypeList) {}

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Instruction.
  /// Read the vector of value type in select instruction.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr) override;

  /// Getter of the index
  Span<const ValType> getValTypeList() const { return ValTypeList; }

private:
  /// Vector of valtype.
  std::vector<ValType> ValTypeList;
};

/// Derived variable instruction node.
class VariableInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  VariableInstruction(const OpCode Byte, const uint32_t Off = 0)
      : Instruction(Byte, Off) {}
  /// Copy constructor.
  VariableInstruction(const VariableInstruction &Instr)
      : Instruction(Instr.Code, Instr.Offset), VarIdx(Instr.VarIdx) {}

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Instruction.
  /// Read the global or local variable index.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr) override;

  /// Getter of the index
  uint32_t getVariableIndex() const { return VarIdx; }

private:
  /// Global or local index.
  uint32_t VarIdx = 0;
};

/// Derived table instruction node.
class TableInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  TableInstruction(const OpCode Byte, const uint32_t Off = 0)
      : Instruction(Byte, Off) {}
  /// Copy constructor.
  TableInstruction(const TableInstruction &Instr)
      : Instruction(Instr.Code, Instr.Offset), TargetIdx(Instr.TargetIdx),
        SourceIdx(Instr.SourceIdx), ElemIdx(Instr.ElemIdx) {}

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Instruction.
  /// Read the table index or element index.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr) override;

  /// Getter of target table index.
  uint32_t getTargetIndex() const { return TargetIdx; }

  /// Getter of source table index.
  uint32_t getSourceIndex() const { return SourceIdx; }

  /// Getter of element instance index.
  uint32_t getElemIndex() const { return ElemIdx; }

private:
  /// \name Data of table instruction: element index and table index.
  /// @{
  uint32_t TargetIdx = 0;
  uint32_t SourceIdx = 0;
  uint32_t ElemIdx = 0;
  /// @}
};

/// Derived memory instruction node.
class MemoryInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  MemoryInstruction(const OpCode Byte, const uint32_t Off = 0)
      : Instruction(Byte, Off) {}
  /// Copy constructor.
  MemoryInstruction(const MemoryInstruction &Instr)
      : Instruction(Instr.Code, Instr.Offset), Align(Instr.Align),
        Offset(Instr.Offset), DataIdx(Instr.DataIdx) {}

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Instruction.
  /// Read the memory arguments: alignment and offset.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr) override;

  /// Getters of memory align and offset.
  uint32_t getMemoryAlign() const { return Align; }
  uint32_t getMemoryOffset() const { return Offset; }

  /// Getter of data instance index.
  uint32_t getDataIndex() const { return DataIdx; }

private:
  /// \name Data of memory instruction: Alignment and offset.
  /// @{
  uint32_t Align = 0;
  uint32_t Offset = 0;
  uint32_t DataIdx = 0;
  /// @}
};

/// Derived const numeric instruction node.
class ConstInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  ConstInstruction(const OpCode Byte, const uint32_t Off = 0)
      : Instruction(Byte, Off) {}
  /// Copy constructor.
  ConstInstruction(const ConstInstruction &Instr)
      : Instruction(Instr.Code, Instr.Offset), Num(Instr.Num) {}

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Instruction.
  /// Read and decode the const value.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr) override;

  /// Getter of the constant value.
  ValVariant getConstValue() const { return Num; }

private:
  /// Const value of this instruction.
  ValVariant Num;
};

/// Derived numeric instruction node.
class UnaryNumericInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  UnaryNumericInstruction(const OpCode Byte, const uint32_t Off = 0)
      : Instruction(Byte, Off) {}
  /// Copy constructor.
  UnaryNumericInstruction(const UnaryNumericInstruction &Instr)
      : Instruction(Instr.Code, Instr.Offset) {}
};

/// Derived numeric instruction node.
class BinaryNumericInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  BinaryNumericInstruction(const OpCode Byte, const uint32_t Off = 0)
      : Instruction(Byte, Off) {}
  /// Copy constructor.
  BinaryNumericInstruction(const BinaryNumericInstruction &Instr)
      : Instruction(Instr.Code, Instr.Offset) {}
};

template <typename T> auto dispatchInstruction(OpCode Code, T &&Visitor) {
  switch (Code) {
    /// The OpCode::End and OpCode::Else will not make nodes.
  case OpCode::Unreachable:
  case OpCode::Nop:
  case OpCode::Return:
    return Visitor(Support::tag<ControlInstruction>());

  case OpCode::Block:
  case OpCode::Loop:
    return Visitor(Support::tag<BlockControlInstruction>());

  case OpCode::If:
    return Visitor(Support::tag<IfElseControlInstruction>());

  case OpCode::Br:
  case OpCode::Br_if:
    return Visitor(Support::tag<BrControlInstruction>());

  case OpCode::Br_table:
    return Visitor(Support::tag<BrTableControlInstruction>());

  case OpCode::Call:
  case OpCode::Call_indirect:
    return Visitor(Support::tag<CallControlInstruction>());

  case OpCode::Ref__null:
  case OpCode::Ref__is_null:
  case OpCode::Ref__func:
    return Visitor(Support::tag<ReferenceInstruction>());

  case OpCode::Drop:
  case OpCode::Select:
  case OpCode::Select_t:
    return Visitor(Support::tag<ParametricInstruction>());

  case OpCode::Local__get:
  case OpCode::Local__set:
  case OpCode::Local__tee:
  case OpCode::Global__get:
  case OpCode::Global__set:
    return Visitor(Support::tag<VariableInstruction>());

  case OpCode::Table__get:
  case OpCode::Table__set:
  case OpCode::Table__init:
  case OpCode::Elem__drop:
  case OpCode::Table__copy:
  case OpCode::Table__grow:
  case OpCode::Table__size:
  case OpCode::Table__fill:
    return Visitor(Support::tag<TableInstruction>());

  case OpCode::I32__load:
  case OpCode::I64__load:
  case OpCode::F32__load:
  case OpCode::F64__load:
  case OpCode::I32__load8_s:
  case OpCode::I32__load8_u:
  case OpCode::I32__load16_s:
  case OpCode::I32__load16_u:
  case OpCode::I64__load8_s:
  case OpCode::I64__load8_u:
  case OpCode::I64__load16_s:
  case OpCode::I64__load16_u:
  case OpCode::I64__load32_s:
  case OpCode::I64__load32_u:
  case OpCode::I32__store:
  case OpCode::I64__store:
  case OpCode::F32__store:
  case OpCode::F64__store:
  case OpCode::I32__store8:
  case OpCode::I32__store16:
  case OpCode::I64__store8:
  case OpCode::I64__store16:
  case OpCode::I64__store32:
  case OpCode::Memory__size:
  case OpCode::Memory__grow:
  case OpCode::Memory__init:
  case OpCode::Data__drop:
  case OpCode::Memory__copy:
  case OpCode::Memory__fill:
    return Visitor(Support::tag<MemoryInstruction>());

  case OpCode::I32__const:
  case OpCode::I64__const:
  case OpCode::F32__const:
  case OpCode::F64__const:
    return Visitor(Support::tag<ConstInstruction>());

  case OpCode::I32__eqz:
  case OpCode::I32__clz:
  case OpCode::I32__ctz:
  case OpCode::I32__popcnt:
  case OpCode::I64__eqz:
  case OpCode::I64__clz:
  case OpCode::I64__ctz:
  case OpCode::I64__popcnt:
  case OpCode::F32__abs:
  case OpCode::F32__neg:
  case OpCode::F32__ceil:
  case OpCode::F32__floor:
  case OpCode::F32__trunc:
  case OpCode::F32__nearest:
  case OpCode::F32__sqrt:
  case OpCode::F64__abs:
  case OpCode::F64__neg:
  case OpCode::F64__ceil:
  case OpCode::F64__floor:
  case OpCode::F64__trunc:
  case OpCode::F64__nearest:
  case OpCode::F64__sqrt:
  case OpCode::I32__wrap_i64:
  case OpCode::I32__trunc_f32_s:
  case OpCode::I32__trunc_f32_u:
  case OpCode::I32__trunc_f64_s:
  case OpCode::I32__trunc_f64_u:
  case OpCode::I64__extend_i32_s:
  case OpCode::I64__extend_i32_u:
  case OpCode::I64__trunc_f32_s:
  case OpCode::I64__trunc_f32_u:
  case OpCode::I64__trunc_f64_s:
  case OpCode::I64__trunc_f64_u:
  case OpCode::F32__convert_i32_s:
  case OpCode::F32__convert_i32_u:
  case OpCode::F32__convert_i64_s:
  case OpCode::F32__convert_i64_u:
  case OpCode::F32__demote_f64:
  case OpCode::F64__convert_i32_s:
  case OpCode::F64__convert_i32_u:
  case OpCode::F64__convert_i64_s:
  case OpCode::F64__convert_i64_u:
  case OpCode::F64__promote_f32:
  case OpCode::I32__reinterpret_f32:
  case OpCode::I64__reinterpret_f64:
  case OpCode::F32__reinterpret_i32:
  case OpCode::F64__reinterpret_i64:
  case OpCode::I32__extend8_s:
  case OpCode::I32__extend16_s:
  case OpCode::I64__extend8_s:
  case OpCode::I64__extend16_s:
  case OpCode::I64__extend32_s:
  case OpCode::I32__trunc_sat_f32_s:
  case OpCode::I32__trunc_sat_f32_u:
  case OpCode::I32__trunc_sat_f64_s:
  case OpCode::I32__trunc_sat_f64_u:
  case OpCode::I64__trunc_sat_f32_s:
  case OpCode::I64__trunc_sat_f32_u:
  case OpCode::I64__trunc_sat_f64_s:
  case OpCode::I64__trunc_sat_f64_u:
    return Visitor(Support::tag<UnaryNumericInstruction>());

  case OpCode::I32__eq:
  case OpCode::I32__ne:
  case OpCode::I32__lt_s:
  case OpCode::I32__lt_u:
  case OpCode::I32__gt_s:
  case OpCode::I32__gt_u:
  case OpCode::I32__le_s:
  case OpCode::I32__le_u:
  case OpCode::I32__ge_s:
  case OpCode::I32__ge_u:
  case OpCode::I64__eq:
  case OpCode::I64__ne:
  case OpCode::I64__lt_s:
  case OpCode::I64__lt_u:
  case OpCode::I64__gt_s:
  case OpCode::I64__gt_u:
  case OpCode::I64__le_s:
  case OpCode::I64__le_u:
  case OpCode::I64__ge_s:
  case OpCode::I64__ge_u:
  case OpCode::F32__eq:
  case OpCode::F32__ne:
  case OpCode::F32__lt:
  case OpCode::F32__gt:
  case OpCode::F32__le:
  case OpCode::F32__ge:
  case OpCode::F64__eq:
  case OpCode::F64__ne:
  case OpCode::F64__lt:
  case OpCode::F64__gt:
  case OpCode::F64__le:
  case OpCode::F64__ge:

  case OpCode::I32__add:
  case OpCode::I32__sub:
  case OpCode::I32__mul:
  case OpCode::I32__div_s:
  case OpCode::I32__div_u:
  case OpCode::I32__rem_s:
  case OpCode::I32__rem_u:
  case OpCode::I32__and:
  case OpCode::I32__or:
  case OpCode::I32__xor:
  case OpCode::I32__shl:
  case OpCode::I32__shr_s:
  case OpCode::I32__shr_u:
  case OpCode::I32__rotl:
  case OpCode::I32__rotr:
  case OpCode::I64__add:
  case OpCode::I64__sub:
  case OpCode::I64__mul:
  case OpCode::I64__div_s:
  case OpCode::I64__div_u:
  case OpCode::I64__rem_s:
  case OpCode::I64__rem_u:
  case OpCode::I64__and:
  case OpCode::I64__or:
  case OpCode::I64__xor:
  case OpCode::I64__shl:
  case OpCode::I64__shr_s:
  case OpCode::I64__shr_u:
  case OpCode::I64__rotl:
  case OpCode::I64__rotr:
  case OpCode::F32__add:
  case OpCode::F32__sub:
  case OpCode::F32__mul:
  case OpCode::F32__div:
  case OpCode::F32__min:
  case OpCode::F32__max:
  case OpCode::F32__copysign:
  case OpCode::F64__add:
  case OpCode::F64__sub:
  case OpCode::F64__mul:
  case OpCode::F64__div:
  case OpCode::F64__min:
  case OpCode::F64__max:
  case OpCode::F64__copysign:
    return Visitor(Support::tag<BinaryNumericInstruction>());

  default:
    return Visitor(Support::tag<void>());
  }
}

/// Load OpCode from file manager.
///
/// Read OpCode byte(s) from file manager and return OpCode.
///
/// \param FileMgr the file manager object to load bytes.
///
/// \returns OpCode if success, ErrCode when failed.
Expect<OpCode> loadOpCode(FileMgr &Mgr);

/// Make the new instruction node.
///
/// Select the node type corresponding to the input Code.
/// Create the derived instruction class and return pointer.
///
/// \param Code the OpCode of instruction to make.
/// \param Offset the Offset of loaded file or vector.
///
/// \returns unique pointer of instruction node if success, ErrCode when failed.
Expect<std::unique_ptr<Instruction>> makeInstructionNode(OpCode Code,
                                                         uint32_t Offset);

/// Make the new instruction node from old one.
///
/// Select the node type corresponding to the input Code.
/// Create the duplicated instruction node and return pointer.
///
/// \param Instr the instruction to duplicate.
///
/// \returns unique pointer of instruction node if success, ErrCode when failed.
Expect<std::unique_ptr<Instruction>>
makeInstructionNode(const Instruction &Instr);

} // namespace AST
} // namespace SSVM
