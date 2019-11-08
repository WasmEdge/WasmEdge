//===-- ssvm/ast/instruction.h - Inst classes definition---------*- C++ -*-===//
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

#include "ast/common.h"
#include "loader/filemgr.h"

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
  /// Instruction opcode enumeration class.
  enum class OpCode : unsigned char {
    /// Control instructions
    Unreachable = 0x00,
    Nop = 0x01,
    Block = 0x02,
    Loop = 0x03,
    If = 0x04,
    Else = 0x05,
    End = 0x0B,
    Br = 0x0C,
    Br_if = 0x0D,
    Br_table = 0x0E,
    Return = 0x0F,
    Call = 0x10,
    Call_indirect = 0x11,

    /// Parametric Instructions
    Drop = 0x1A,
    Select = 0x1B,

    /// Variable Instructions
    Local__get = 0x20,
    Local__set = 0x21,
    Local__tee = 0x22,
    Global__get = 0x23,
    Global__set = 0x24,

    /// Memory Instructions
    I32__load = 0x28,
    I64__load = 0x29,
    F32__load = 0x2A,
    F64__load = 0x2B,
    I32__load8_s = 0x2C,
    I32__load8_u = 0x2D,
    I32__load16_s = 0x2E,
    I32__load16_u = 0x2F,
    I64__load8_s = 0x30,
    I64__load8_u = 0x31,
    I64__load16_s = 0x32,
    I64__load16_u = 0x33,
    I64__load32_s = 0x34,
    I64__load32_u = 0x35,
    I32__store = 0x36,
    I64__store = 0x37,
    F32__store = 0x38,
    F64__store = 0x39,
    I32__store8 = 0x3A,
    I32__store16 = 0x3B,
    I64__store8 = 0x3C,
    I64__store16 = 0x3D,
    I64__store32 = 0x3E,
    Memory__size = 0x3F,
    Memory__grow = 0x40,

    /// Const numeric instructions
    I32__const = 0x41,
    I64__const = 0x42,
    F32__const = 0x43,
    F64__const = 0x44,

    /// Numeric instructions
    I32__eqz = 0x45,
    I32__eq = 0x46,
    I32__ne = 0x47,
    I32__lt_s = 0x48,
    I32__lt_u = 0x49,
    I32__gt_s = 0x4A,
    I32__gt_u = 0x4B,
    I32__le_s = 0x4C,
    I32__le_u = 0x4D,
    I32__ge_s = 0x4E,
    I32__ge_u = 0x4F,
    I64__eqz = 0x50,
    I64__eq = 0x51,
    I64__ne = 0x52,
    I64__lt_s = 0x53,
    I64__lt_u = 0x54,
    I64__gt_s = 0x55,
    I64__gt_u = 0x56,
    I64__le_s = 0x57,
    I64__le_u = 0x58,
    I64__ge_s = 0x59,
    I64__ge_u = 0x5A,
    F32__eq = 0x5B,
    F32__ne = 0x5C,
    F32__lt = 0x5D,
    F32__gt = 0x5E,
    F32__le = 0x5F,
    F32__ge = 0x60,
    F64__eq = 0x61,
    F64__ne = 0x62,
    F64__lt = 0x63,
    F64__gt = 0x64,
    F64__le = 0x65,
    F64__ge = 0x66,
    I32__clz = 0x67,
    I32__ctz = 0x68,
    I32__popcnt = 0x69,
    I32__add = 0x6A,
    I32__sub = 0x6B,
    I32__mul = 0x6C,
    I32__div_s = 0x6D,
    I32__div_u = 0x6E,
    I32__rem_s = 0x6F,
    I32__rem_u = 0x70,
    I32__and = 0x71,
    I32__or = 0x72,
    I32__xor = 0x73,
    I32__shl = 0x74,
    I32__shr_s = 0x75,
    I32__shr_u = 0x76,
    I32__rotl = 0x77,
    I32__rotr = 0x78,
    I64__clz = 0x79,
    I64__ctz = 0x7a,
    I64__popcnt = 0x7b,
    I64__add = 0x7c,
    I64__sub = 0x7d,
    I64__mul = 0x7e,
    I64__div_s = 0x7f,
    I64__div_u = 0x80,
    I64__rem_s = 0x81,
    I64__rem_u = 0x82,
    I64__and = 0x83,
    I64__or = 0x84,
    I64__xor = 0x85,
    I64__shl = 0x86,
    I64__shr_s = 0x87,
    I64__shr_u = 0x88,
    I64__rotl = 0x89,
    I64__rotr = 0x8A,
    F32__abs = 0x8B,
    F32__neg = 0x8C,
    F32__ceil = 0x8D,
    F32__floor = 0x8E,
    F32__trunc = 0x8F,
    F32__nearest = 0x90,
    F32__sqrt = 0x91,
    F32__add = 0x92,
    F32__sub = 0x93,
    F32__mul = 0x94,
    F32__div = 0x95,
    F32__min = 0x96,
    F32__max = 0x97,
    F32__copysign = 0x98,
    F64__abs = 0x99,
    F64__neg = 0x9A,
    F64__ceil = 0x9B,
    F64__floor = 0x9C,
    F64__trunc = 0x9D,
    F64__nearest = 0x9E,
    F64__sqrt = 0x9F,
    F64__add = 0xA0,
    F64__sub = 0xA1,
    F64__mul = 0xA2,
    F64__div = 0xA3,
    F64__min = 0xA4,
    F64__max = 0xA5,
    F64__copysign = 0xA6,
    I32__wrap_i64 = 0xA7,
    I32__trunc_f32_s = 0xA8,
    I32__trunc_f32_u = 0xA9,
    I32__trunc_f64_s = 0xAA,
    I32__trunc_f64_u = 0xAB,
    I64__extend_i32_s = 0xAC,
    I64__extend_i32_u = 0xAD,
    I64__trunc_f32_s = 0xAE,
    I64__trunc_f32_u = 0xAF,
    I64__trunc_f64_s = 0xB0,
    I64__trunc_f64_u = 0xB1,
    F32__convert_i32_s = 0xB2,
    F32__convert_i32_u = 0xB3,
    F32__convert_i64_s = 0xB4,
    F32__convert_i64_u = 0xB5,
    F32__demote_f64 = 0xB6,
    F64__convert_i32_s = 0xB7,
    F64__convert_i32_u = 0xB8,
    F64__convert_i64_s = 0xB9,
    F64__convert_i64_u = 0xBA,
    F64__promote_f32 = 0xBB,
    I32__reinterpret_f32 = 0xBC,
    I64__reinterpret_f64 = 0xBD,
    F32__reinterpret_i32 = 0xBE,
    F64__reinterpret_i64 = 0xBF
  };

  /// Constructor assigns the OpCode.
  Instruction(OpCode &Byte) { Code = Byte; };
  virtual ~Instruction() = default;

  /// Binary loading from file manager. Default not load anything.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr) {
    return Loader::ErrCode::Success;
  };

  /// Getter of OpCode.
  OpCode getOpCode() const { return Code; }

  /// For IfElse and BlockControl instructions.
  virtual ValType getResultType() const { return ValType::None; }
  virtual const InstrVec *getBody() const { return nullptr; }
  virtual const InstrVec *getIfStatement() const { return nullptr; }
  virtual const InstrVec *getElseStatement() const { return nullptr; }

  /// For Br and BrTable instructions.
  virtual unsigned int getLabelIndex() const { return 0; }
  virtual const std::vector<unsigned int> *getLabelTable() const {
    return nullptr;
  }

  /// For Call instructions.
  virtual unsigned int getFuncIndex() const { return 0; }

  /// For Variable instructions.
  virtual unsigned int getVariableIndex() const { return 0; }

  /// For Memory instructions.
  virtual unsigned int getMemoryAlign() const { return 0; }
  virtual unsigned int getMemoryOffset() const { return 0; }

  /// For ConstNumeric instructions.
  virtual ValVariant getConstValue() { return ValVariant(0U); }

protected:
  /// OpCode if this instruction node.
  OpCode Code;
};

/// Derived control instruction node.
class ControlInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  ControlInstruction(OpCode &Byte) : Instruction(Byte) {}
};

/// Derived block control instruction node.
class BlockControlInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  BlockControlInstruction(OpCode &Byte) : Instruction(Byte) {}

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Instruction.
  /// Read the return type, instructions in block body.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

  /// Getter of block type
  virtual ValType getResultType() const { return BlockType; }

  /// Getter of Block Body
  virtual const InstrVec *getBody() const { return &Body; }

private:
  /// \name Data of block instruction: return type and block body.
  /// @{
  ValType BlockType;
  InstrVec Body;
  /// @}
}; // namespace AST

/// Derived if-else control instruction node.
class IfElseControlInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  IfElseControlInstruction(OpCode &Byte) : Instruction(Byte) {}

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Instruction.
  /// Read the return type, instructions in If and Else statements.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

  /// Getter of block type
  virtual ValType getResultType() const { return BlockType; }

  /// Getter of if statement.
  virtual const InstrVec *getIfStatement() const { return &IfStatement; }

  /// Getter of else statement.
  virtual const InstrVec *getElseStatement() const { return &ElseStatement; }

private:
  /// \name Data of block instruction: return type and statements.
  /// @{
  ValType BlockType;
  InstrVec IfStatement;
  InstrVec ElseStatement;
  /// @}
};

/// Derived branch control instruction node.
class BrControlInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  BrControlInstruction(OpCode &Byte) : Instruction(Byte) {}

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Instruction.
  /// Read the branch label index.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

  /// Get label index
  virtual unsigned int getLabelIndex() const { return LabelIdx; }

private:
  /// Branch-to label index.
  unsigned int LabelIdx = 0;
};

/// Derived branch table control instruction node.
class BrTableControlInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  BrTableControlInstruction(OpCode &Byte) : Instruction(Byte) {}

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Instruction.
  /// Read the vector of labels and default branch label of indirect branch.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

  /// Getter of label table
  virtual const std::vector<unsigned int> *getLabelTable() const {
    return &LabelTable;
  }

  /// Getter of label index
  virtual unsigned int getLabelIndex() const { return LabelIdx; }

private:
  /// \name Data of branch instruction: label vector and defalt label.
  /// @{
  std::vector<unsigned int> LabelTable;
  unsigned int LabelIdx = 0;
  /// @}
};

/// Derived call control instruction node.
class CallControlInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  CallControlInstruction(OpCode &Byte) : Instruction(Byte) {}

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Instruction.
  /// Read the function index.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

  /// Getter of the index
  virtual unsigned int getFuncIndex() const { return FuncIdx; }

private:
  /// Call function index.
  unsigned int FuncIdx = 0;
};

/// Derived parametric instruction node.
class ParametricInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  ParametricInstruction(OpCode &Byte) : Instruction(Byte) {}
};

/// Derived variable instruction node.
class VariableInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  VariableInstruction(OpCode &Byte) : Instruction(Byte) {}

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Instruction.
  /// Read the global or local variable index.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

  /// Getter of the index
  virtual unsigned int getVariableIndex() const { return VarIdx; }

private:
  /// Global or local index.
  unsigned int VarIdx = 0;
};

/// Derived memory instruction node.
class MemoryInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  MemoryInstruction(OpCode &Byte) : Instruction(Byte) {}

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Instruction.
  /// Read the memory arguments: alignment and offset.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

  /// Getters of memory align and offset.
  virtual unsigned int getMemoryAlign() const { return Align; }
  virtual unsigned int getMemoryOffset() const { return Offset; }

private:
  /// \name Data of memory instruction: Alignment and offset.
  /// @{
  unsigned int Align = 0;
  unsigned int Offset = 0;
  /// @}
};

/// Derived const numeric instruction node.
class ConstInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  ConstInstruction(OpCode &Byte) : Instruction(Byte) {}

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Instruction.
  /// Read and decode the const value.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

  /// Getter of the constant value.
  virtual ValVariant getConstValue() { return Num; }

private:
  /// Const value of this instruction.
  ValVariant Num;
};

/// Derived numeric instruction node.
class NumericInstruction : public Instruction {
public:
  /// Call base constructor to initialize OpCode.
  NumericInstruction(OpCode &Byte) : Instruction(Byte) {}
};

/// Make the new instruction node.
///
/// Select the node type corresponding to the input Code.
/// Create the derived instruction class and return pointer.
///
/// \param Code the OpCode of instruction to make.
/// \param NewInst the unique pointer to created instruction node.
///
/// \returns ErrCode.
Loader::ErrCode makeInstructionNode(Instruction::OpCode Code,
                                    std::unique_ptr<Instruction> &NewInst);

} // namespace AST
} // namespace SSVM
