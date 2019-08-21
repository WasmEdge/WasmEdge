#include "executor/worker.h"
#include "ast/common.h"
#include "ast/instruction.h"
#include "executor/worker_util.h"
#include "support/casting.h"

namespace SSVM {
namespace Executor {

namespace {

using OpCode = AST::Instruction::OpCode;
using Value = AST::ValVariant;

} // namespace

ErrCode Worker::setArguments(Bytes &Input) {
  Args.assign(Input.begin(), Input.end());
  return ErrCode::Success;
}

ErrCode
Worker::setCode(std::vector<std::unique_ptr<AST::Instruction>> *Instrs) {
  for (auto &Instr : *Instrs) {
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

  if (TheState == State::Terminated) {
    return ErrCode::Success;
  } else if (TheState == State::Unreachable) {
    return ErrCode::Unreachable;
  }

  return Status;
}

ErrCode Worker::runConstNumericOp(AST::Instruction *InstrPtr) {
  auto TheInstrPtr = dynamic_cast<AST::ConstInstruction *>(InstrPtr);
  if (TheInstrPtr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  std::unique_ptr<ValueEntry> VE = nullptr;
  std::visit([&VE](auto &&arg) { VE = std::make_unique<ValueEntry>(arg); },
             TheInstrPtr->value());

  StackMgr.push(VE);

  return ErrCode::Success;
}

ErrCode Worker::runNumericOp(AST::Instruction *InstrPtr) {
  auto TheInstrPtr = dynamic_cast<AST::NumericInstruction *>(InstrPtr);
  if (TheInstrPtr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  auto Opcode = TheInstrPtr->getOpCode();
  auto Status = ErrCode::Success;
  if (isBinaryOp(Opcode)) {
    std::unique_ptr<ValueEntry> Val1, Val2;
    StackMgr.pop(Val2);
    StackMgr.pop(Val1);

    if (isValueTypeEqual(*Val1.get(), *Val2.get())) {
      return ErrCode::TypeNotMatch;
    }

    switch (Opcode) {
    case OpCode::I32__add:
      Status = runAddOp<int32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__sub:
      Status = runSubOp<int32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__add:
      Status = runAddOp<int64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__sub:
      Status = runSubOp<int64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__mul:
      Status = runMulOp<int64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__div_u:
      Status = runDivUOp<int64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__rem_u:
      Status = runModUOp<int64_t>(Val1.get(), Val2.get());
      break;
    default:
      Status = ErrCode::Unimplemented;
      break;
    }
  } else if (isComparisonOp(Opcode)) {
    std::unique_ptr<ValueEntry> Val1, Val2;
    StackMgr.pop(Val2);
    StackMgr.pop(Val1);

    if (isValueTypeEqual(*Val1.get(), *Val2.get())) {
      return ErrCode::TypeNotMatch;
    }

    switch (Opcode) {
    case OpCode::I32__le_s:
      Status = runLeSOp<int32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__eq:
      Status = runEqOp<int32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__ne:
      Status = runNeOp<int32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__eq:
      Status = runEqOp<int64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__lt_u:
      Status = runLtUOp<int64_t>(Val1.get(), Val2.get());
      break;
    default:
      Status = ErrCode::Unimplemented;
      break;
    }
  } else {
    Status = ErrCode::Unimplemented;
  }
  return Status;
}

ErrCode Worker::runControlOp(AST::Instruction *InstrPtr) {
  auto TheInstrPtr = dynamic_cast<AST::ControlInstruction *>(InstrPtr);
  if (TheInstrPtr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  auto Status = ErrCode::Success;
  switch (TheInstrPtr->getOpCode()) {
  case OpCode::Unreachable:
    TheState = State::Unreachable;
    Status = ErrCode::Unreachable;
    break;
  case OpCode::Return:
    Status = runReturnOp();
    break;
  case OpCode::Br:
    Status = runBrOp(TheInstrPtr);
    break;
  case OpCode::Br_if:
    Status = runBrIfOp(TheInstrPtr);
    break;
  default:
    Status = ErrCode::Unimplemented;
    break;
  }

  return ErrCode::Success;
}

ErrCode Worker::runMemoryOp(AST::Instruction *InstrPtr) {
  auto TheInstrPtr = dynamic_cast<AST::MemoryInstruction *>(InstrPtr);
  if (TheInstrPtr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  auto Status = ErrCode::Success;
  auto Opcode = TheInstrPtr->getOpCode();
  if (isLoadOp(Opcode)) {
    switch (Opcode) {
    case OpCode::I32__load:
      Status = runLoadOp<int32_t>(TheInstrPtr);
      break;
    case OpCode::I64__load:
      Status = runLoadOp<int64_t>(TheInstrPtr);
      break;
    default:
      Status = ErrCode::Unimplemented;
      break;
    }
  } else if (isStoreOp(Opcode)) {
    switch (Opcode) {
    case OpCode::I32__store:
      Status = runStoreOp<int32_t>(TheInstrPtr);
      break;
    case OpCode::I64__store:
      Status = runStoreOp<int64_t>(TheInstrPtr);
      break;
    default:
      Status = ErrCode::Unimplemented;
      break;
    }
  } else {
    Status = ErrCode::Unimplemented;
  }

  return Status;
}

ErrCode Worker::runParametricOp(AST::Instruction *InstrPtr) {
  auto TheInstrPtr = dynamic_cast<AST::ParametricInstruction *>(InstrPtr);
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

ErrCode Worker::runVariableOp(AST::Instruction *InstrPtr) {
  auto TheInstrPtr = dynamic_cast<AST::VariableInstruction *>(InstrPtr);
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
    std::unique_ptr<ValueEntry> NewVal =
        std::make_unique<ValueEntry>(*Val.get());
    StackMgr.push(NewVal);
    CurrentFrame->setValue(Index, Val);
  } else if (Opcode == OpCode::Global__get) {
    StackMgr.getCurrentFrame(CurrentFrame);
    ValueEntry Val;
    Instance::GlobalInstance *GlobPtr = nullptr;
    StoreMgr.getGlobal(Index, GlobPtr);
    GlobPtr->getValue(Val);
    std::unique_ptr<ValueEntry> NewVal = std::make_unique<ValueEntry>(Val);
    StackMgr.push(NewVal);
  } else if (Opcode == OpCode::Global__set) {
    StackMgr.getCurrentFrame(CurrentFrame);
    Instance::GlobalInstance *GlobPtr = nullptr;
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
