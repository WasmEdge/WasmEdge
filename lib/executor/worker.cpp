#include "ast/common.h"
#include "ast/instruction.h"
#include "executor/worker.h"

namespace SSVM {
namespace Executor {

namespace {
using OpCode = AST::Instruction::OpCode;
using Value = AST::ValVariant;

/// helper functions for execution
inline bool isInRange(OpCode X, OpCode Y, OpCode Z) {
  auto XC = static_cast <unsigned char>(X);
  auto YC = static_cast <unsigned char>(Y);
  auto ZC = static_cast <unsigned char>(Z);
  return (XC <= YC && YC <= ZC);
}

inline bool isControlOp(OpCode Opcode) {
  return isInRange(OpCode::Unreachable, Opcode, OpCode::Call_indirect);
}

inline bool isParametricOp(OpCode Opcode) {
  return isInRange(OpCode::Drop, Opcode, OpCode::Select);
}

inline bool isVariableOp(OpCode Opcode) {
  return isInRange(OpCode::Local__get, Opcode, OpCode::Global__set);
}

inline bool isMemoryOp(OpCode Opcode) {
  return isInRange(OpCode::I32__load, Opcode, OpCode::Memory__grow);
}

inline bool isConstNumericOp(OpCode Opcode) {
  return isInRange(OpCode::I32__const, Opcode, OpCode::F64__const);
}

inline bool isNumericOp(OpCode Opcode) {
  return isInRange(OpCode::I32__eqz, Opcode, OpCode::F64__reinterpret_i64);
}

} // anonymous namespace

ErrCode Worker::setArguments(Bytes &Input) {
  Args.assign(Input.begin(), Input.end());
  return ErrCode::Success;
}

ErrCode Worker::setCode(std::vector<std::unique_ptr<AST::Instruction>> &Instrs) {
    for (auto &Instr : Instrs) {
      this->Instrs.push_back(Instr.get());
    }
    return ErrCode::Success;
}

ErrCode Worker::run() {
  ErrCode Status = ErrCode::Success;
  for (auto &Inst : Instrs) {
    OpCode Opcode = Inst->getOpCode();
    if (isConstNumericOp(Opcode)) {
      Status = runConstNumericOp(Inst);
    } else if (isControlOp(Opcode)) {
      Status = runControlOp(Inst);
    } else if (isNumericOp(Opcode)) {
      Status = runNumericOp(Inst);
    } else if (isMemoryOp(Opcode)) {
      Status = runMemoryOp(Inst);
    } else if (isParametricOp(Opcode)) {
      Status = runParametricOp(Inst);
    } else if (isVariableOp(Opcode)) {
      Status = runVariableOp(Inst);
    }

    if (Status != ErrCode::Success) {
      break;
    }
  }
  return Status;
}

ErrCode Worker::runConstNumericOp(AST::Instruction *Instr) {
  auto TheInstr = dynamic_cast<AST::ConstInstruction*>(Instr);
  if (TheInstr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  std::unique_ptr<ValueEntry> VE = nullptr;
  std::visit([&VE](auto&& arg) {
    VE = std::make_unique<ValueEntry>(arg);
  }, TheInstr->value());

  StackMgr.push(VE);

  return ErrCode::Success;
}

ErrCode runNumericOp(AST::Instruction* Instr) {
  // XXX: unimplemented
  return ErrCode::Success;
}

ErrCode runControlOp(AST::Instruction* Instr) {
  // XXX: unimplemented
  return ErrCode::Success;
}

ErrCode runMemoryOp(AST::Instruction* Instr) {
  // XXX: unimplemented
  return ErrCode::Success;
}

ErrCode runParametricOp(AST::Instruction* Instr) {
  // XXX: unimplemented
  return ErrCode::Success;
}

ErrCode runVariableOp(AST::Instruction* Instr) {
  // XXX: unimplemented
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
