#include "ast/instruction.h"
#include "executor/worker.h"

namespace SSVM {
namespace Executor {

namespace detail {
using OpCode = AST::Instruction::OpCode;

/// helper functions for execution
inline bool isInRange(OpCode X, OpCode Y, OpCode Z) const {
  auto XC = static_case <unsigned char>(X);
  auto YC = static_case <unsigned char>(Y);
  auto ZC = static_case <unsigned char>(Z);
  return (XC <= YC && YC <= ZC);
}

inline bool isControlOp(OpCode Opcode) const {
  return isInRange(OpCode::Unreachable, Opcode, OpCode::Call_indirect);
}

inline bool isParametricOp(OpCode Opcode) const {
  return isInRange(OpCode::Drop, Opcode, OpCode::Select);
}

inline bool isVariableOp(OpCode Opcode) const {
  return isInRange(OpCode::Local__get, Opcode, OpCode::Global__set);
}

inline bool isMemoryOp(OpCode Opcode) const {
  return isInRange(OpCode::I32__load, Opcode, OpCode::Memory__grow);
}

inline bool isConstNumericOp(OpCode Opcode) const {
  return isInRange(OpCode::I32__const, Opcode, OpCode::F64__const);
}

inline bool isNumericOp(OpCode Opcode) const {
  return isInRange(OpCode::I32__eqz, Opcode, OpCode::F64__reinterpret_i64);
}

} // namespace detail

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
  for (auto &Inst : Instrs) {
    OpCode opcode = Inst.getOpCode();
    if (isConstNumericOp(opcode)) {
      // TODO: const numeric instructions
    } else if (isControlOp(opcode)) {
      // TODO: control instructions
    } else if (isNumericOp(opcode)) {
      // TODO: numeric instructions
    } else if (isMemoryOp(opcode)) {
      // TODO: memory instructions
    } else if (isParametricOp(opcode)) {
      // TODO: parametric instructions
    } else if (isVariableOp(opcode)) {
      // TODO: variable instructions
    }
  }
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
