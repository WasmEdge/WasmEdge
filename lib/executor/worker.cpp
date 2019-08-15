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

ErrCode Worker::runConstNumericOp(AST::Instruction *InstrPtr) {
  auto TheInstrPtr = dynamic_cast<AST::ConstInstruction*>(InstrPtr);
  if (TheInstrPtr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  std::unique_ptr<ValueEntry> VE = nullptr;
  std::visit([&VE](auto&& arg) {
    VE = std::make_unique<ValueEntry>(arg);
  }, TheInstrPtr->value());

  StackMgr.push(VE);

  return ErrCode::Success;
}

ErrCode Worker::runNumericOp(AST::Instruction* InstrPtr) {
  auto TheInstrPtr = dynamic_cast<AST::NumericInstruction*>(InstrPtr);
  if (TheInstrPtr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  auto Opcode = TheInstrPtr->getOpCode();
  if (Opcode == OpCode::I32__add) {
    std::unique_ptr<ValueEntry> Val1, Val2;
    StackMgr.pop(Val2);
    StackMgr.pop(Val1);
    /// Type check
    AST::ValType ValTp1, ValTp2;
    Val1->getType(ValTp1);
    Val2->getType(ValTp2);
    if (ValTp1 == AST::ValType::I32
        && ValTp1 == ValTp2) {
      int32_t Int1, Int2;
      Val1->getValue(Int1);
      Val2->getValue(Int2);
      std::unique_ptr<ValueEntry> NewVal = std::make_unique<ValueEntry>(Int1+Int2);
      StackMgr.push(NewVal);
      return ErrCode::Success;
    }
    return ErrCode::TypeNotMatch;
  } else if (Opcode == OpCode::I32__sub) {
    std::unique_ptr<ValueEntry> Val1, Val2;
    StackMgr.pop(Val2);
    StackMgr.pop(Val1);
    /// Type check
    AST::ValType ValTp1, ValTp2;
    Val1->getType(ValTp1);
    Val2->getType(ValTp2);
    if (ValTp1 == AST::ValType::I32
        && ValTp1 == ValTp2) {
      int32_t Int1, Int2;
      Val1->getValue(Int1);
      Val2->getValue(Int2);
      std::unique_ptr<ValueEntry> NewVal = std::make_unique<ValueEntry>(Int1-Int2);
      StackMgr.push(NewVal);
      return ErrCode::Success;
    }
    return ErrCode::TypeNotMatch;
  } else {
    return ErrCode::Unimplemented;
  }
  return ErrCode::Success;
}

ErrCode Worker::runControlOp(AST::Instruction* Instr) {
  // XXX: unimplemented
  return ErrCode::Success;
}

ErrCode Worker::runMemoryOp(AST::Instruction *InstrPtr) {
  auto TheInstrPtr = dynamic_cast<AST::MemoryInstruction*>(InstrPtr);
  if (TheInstrPtr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  // XXX: unimplemented
  return ErrCode::Success;
}

ErrCode Worker::runParametricOp(AST::Instruction *InstrPtr) {
  auto TheInstrPtr = dynamic_cast<AST::ParametricInstruction*>(InstrPtr);
  if (TheInstrPtr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  if (TheInstrPtr->getOpCode() == OpCode::Drop) {
    StackMgr.pop();
  } else if (TheInstrPtr->getOpCode() == OpCode::Select) {

    // Pop the value i32.const from the stack.
    std::unique_ptr<ValueEntry> VE;
    StackMgr.pop(VE);
    int32_t Val;
    VE->getValue(Val);

    std::unique_ptr<ValueEntry> Val1, Val2;
    StackMgr.pop(Val2);
    StackMgr.pop(Val1);

    if (Val == 0) {
      StackMgr.push(Val2);
    } else {
      StackMgr.push(Val1);
    }
  } else {
    return ErrCode::InstructionTypeMismatch;
  }
  return ErrCode::Success;
}

ErrCode Worker::runVariableOp(AST::Instruction* InstrPtr) {
  auto TheInstrPtr = dynamic_cast<AST::VariableInstruction*>(InstrPtr);
  if (TheInstrPtr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  auto Opcode = TheInstrPtr->getOpCode();
  unsigned int Index = TheInstrPtr->getIndex();

  if (Opcode == OpCode::Local__get) {
    StackMgr.getCurrentFrame(CurrentFrame);
    ValueEntry *Val;
    CurrentFrame->getValue(Index, Val);
    std::unique_ptr<ValueEntry> NewVal = std::make_unique<ValueEntry>(*Val);
    StackMgr.push(NewVal);
  } else if (Opcode == OpCode::Local__set) {
    StackMgr.getCurrentFrame(CurrentFrame);
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);
    CurrentFrame->setValue(Index, Val);
  } else if (Opcode == OpCode::Local__tee) {
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);
    std::unique_ptr<ValueEntry> NewVal = std::make_unique<ValueEntry>(*Val.get());
    StackMgr.push(NewVal);
    CurrentFrame->setValue(Index, Val);
  } else if (Opcode == OpCode::Global__get) {
    StackMgr.getCurrentFrame(CurrentFrame);
    ValueEntry Val;
    GlobalInstance *GlobPtr = nullptr;
    StoreMgr.getGlobal(Index, GlobPtr);
    GlobPtr->getValue(Val);
    std::unique_ptr<ValueEntry> NewVal = std::make_unique<ValueEntry>(Val);
    StackMgr.push(NewVal);
  } else if (Opcode == OpCode::Global__set) {
    StackMgr.getCurrentFrame(CurrentFrame);
    GlobalInstance *GlobPtr = nullptr;
    StoreMgr.getGlobal(Index, GlobPtr);
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);
    GlobPtr->setValue(*Val.get());
  } else {
    return ErrCode::InstructionTypeMismatch;
  }

  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
