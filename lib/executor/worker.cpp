#include "executor/worker.h"
#include "ast/common.h"
#include "ast/instruction.h"
#include "executor/worker/util.h"
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

ErrCode Worker::runExpression(const InstrVec &Instrs) {
  /// Check worker's flow.
  if (TheState != State::Inited)
    return ErrCode::WrongWorkerFlow;

  /// Set instruction vector to instruction provider.
  InstrPdr.pushInstrs(InstrProvider::SeqType::Expression, Instrs);
  TheState = State::CodeSet;
  return execute();
}

ErrCode Worker::runStartFunction(unsigned int FuncAddr) {
  /// Check worker's flow.
  if (TheState != State::Inited)
    return ErrCode::WrongWorkerFlow;

  /// TODO: Push arguments of start function into stack.

  /// Enter start function.
  ErrCode Status = ErrCode::Success;
  if ((Status = invokeFunction(FuncAddr)) != ErrCode::Success)
    return Status;

  /// Execute run loop.
  TheState = State::CodeSet;
  if ((Status = execute()) != ErrCode::Success)
    return Status;

  /// TODO: Pop return value.
  return Status;
}

ErrCode Worker::execute() {
  /// Check worker's flow
  if (TheState == State::Unreachable)
    return ErrCode::Unreachable;
  if (TheState != State::CodeSet)
    return ErrCode::WrongWorkerFlow;

  /// Run instructions
  ErrCode Status = ErrCode::Success;
  AST::Instruction *Instr = nullptr;
  TheState = State::Active;
  while (InstrPdr.getScopeSize() > 0 && Status == ErrCode::Success) {
    Instr = InstrPdr.getNextInstr();
    if (Instr == nullptr) {
      /// Pop instruction sequence.
      if (InstrPdr.getTopScopeType() == InstrProvider::SeqType::FunctionCall)
        Status = returnFunction();
      else if (InstrPdr.getTopScopeType() == InstrProvider::SeqType::Block)
        Status = leaveBlock();
      else
        Status = InstrPdr.popInstrs();
    } else {
      /// Run instructions.
      OpCode Opcode = Instr->getOpCode();
      if (isControlOp(Opcode)) {
        Status = runControlOp(Instr);
      } else if (isParametricOp(Opcode)) {
        Status = runParametricOp(Instr);
      } else if (isVariableOp(Opcode)) {
        Status = runVariableOp(Instr);
      } else if (isMemoryOp(Opcode)) {
        Status = runMemoryOp(Instr);
      } else if (isConstNumericOp(Opcode)) {
        Status = runConstNumericOp(Instr);
      } else if (isNumericOp(Opcode)) {
        Status = runNumericOp(Instr);
      }
    }
  }

  /// Check result
  if (TheState == State::Unreachable)
    return ErrCode::Unreachable;
  TheState = State::Inited;
  return Status;
}

ErrCode Worker::runControlOp(AST::Instruction *InstrPtr) {
  /// Check instruction type.
  auto CtrlInstrPtr = dynamic_cast<AST::ControlInstruction *>(InstrPtr);
  if (CtrlInstrPtr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  /// Check OpCode and run the specific instruction.
  ErrCode Status = ErrCode::Success;
  switch (CtrlInstrPtr->getOpCode()) {
  case OpCode::Unreachable:
    TheState = State::Unreachable;
    Status = ErrCode::Unreachable;
    break;
  case OpCode::Nop:
    break;
  case OpCode::Block:
    Status = runBlockOp(CtrlInstrPtr);
    break;
  case OpCode::Loop:
    Status = runLoopOp(CtrlInstrPtr);
    break;
  case OpCode::If:
    Status = runIfElseOp(CtrlInstrPtr);
    break;
  case OpCode::Br:
    Status = runBrOp(CtrlInstrPtr);
    break;
  case OpCode::Br_if:
    Status = runBrIfOp(CtrlInstrPtr);
    break;
  case OpCode::Br_table:
    Status = runBrTableOp(CtrlInstrPtr);
    break;
  case OpCode::Return:
    Status = runReturnOp();
    break;
  case OpCode::Call:
    Status = runCallOp(CtrlInstrPtr);
    break;
  case OpCode::Call_indirect:
    // Status = runCallIndirectOp(CtrlInstrPtr);
    break;
  default:
    Status = ErrCode::InstructionTypeMismatch;
    break;
  }

  return Status;
}

ErrCode Worker::runParametricOp(AST::Instruction *InstrPtr) {
  /// Check instruction type.
  auto ParamInstrPtr = dynamic_cast<AST::ParametricInstruction *>(InstrPtr);
  if (ParamInstrPtr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  /// Check OpCode and run the specific instruction.
  ErrCode Status = ErrCode::Success;
  if (ParamInstrPtr->getOpCode() == OpCode::Drop) {
    StackMgr.pop();
  } else if (ParamInstrPtr->getOpCode() == OpCode::Select) {
    /// Pop the i32 value and select values from stack.
    std::unique_ptr<ValueEntry> CondValEntry, ValEntry1, ValEntry2;
    StackMgr.pop(CondValEntry);
    StackMgr.pop(ValEntry2);
    StackMgr.pop(ValEntry1);
    int32_t CondValue;
    if ((Status = CondValEntry->getValue(CondValue)) != ErrCode::Success) {
      return Status;
    }

    /// Select the value.
    if (CondValue == 0) {
      StackMgr.push(ValEntry2);
    } else {
      StackMgr.push(ValEntry1);
    }
  } else {
    return ErrCode::InstructionTypeMismatch;
  }
  return ErrCode::Success;
}

ErrCode Worker::runVariableOp(AST::Instruction *InstrPtr) {
  /// Check instruction type.
  auto VarInstr = dynamic_cast<AST::VariableInstruction *>(InstrPtr);
  if (VarInstr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  /// Get variable instruction OpCode, index, and current frame.
  auto Opcode = VarInstr->getOpCode();
  unsigned int Index = VarInstr->getIndex();
  StackMgr.getCurrentFrame(CurrentFrame);

  /// Check OpCode and run the specific instruction.
  ErrCode Status = ErrCode::Success;
  switch (VarInstr->getOpCode()) {
  case OpCode::Local__get:
    Status = runLocalGetOp(Index);
    break;
  case OpCode::Local__set:
    Status = runLocalSetOp(Index);
    break;
  case OpCode::Local__tee:
    Status = runLocalTeeOp(Index);
    break;
  case OpCode::Global__get:
    Status = runGlobalGetOp(Index);
    break;
  case OpCode::Global__set:
    Status = runGlobalSetOp(Index);
    break;
  default:
    Status = ErrCode::InstructionTypeMismatch;
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
  if (isTestNumericOp(Opcode)) {
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);

    switch (Opcode) {
    case OpCode::I32__eqz:
      Status = runEqzOp<uint32_t>(Val.get());
      break;
    case OpCode::I64__eqz:
      Status = runEqzOp<uint64_t>(Val.get());
      break;
    default:
      Status = ErrCode::InstructionTypeMismatch;
      break;
    }
  } else if (isRelationNumericOp(Opcode)) {
    std::unique_ptr<ValueEntry> Val1, Val2;
    StackMgr.pop(Val2);
    StackMgr.pop(Val1);

    if (!isValueTypeEqual(*Val1.get(), *Val2.get())) {
      return ErrCode::TypeNotMatch;
    }

    switch (Opcode) {
    case OpCode::I32__eq:
      Status = runEqIOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__ne:
      Status = runNeIOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__lt_s:
      Status = runLtSOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__lt_u:
      Status = runLtUOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__gt_s:
      Status = runGtSOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__gt_u:
      Status = runGtUOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__le_s:
      Status = runLeSOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__le_u:
      Status = runLeUOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__ge_s:
      Status = runGeSOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__ge_u:
      Status = runGeUOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__eq:
      Status = runEqIOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__ne:
      Status = runNeIOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__lt_s:
      Status = runLtSOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__lt_u:
      Status = runLtUOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__gt_s:
      Status = runGtSOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__gt_u:
      Status = runGtUOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__le_s:
      Status = runLeSOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__le_u:
      Status = runLeUOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__ge_s:
      Status = runGeSOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__ge_u:
      Status = runGeUOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::F32__eq:
      Status = runEqFOp<float>(Val1.get(), Val2.get());
      break;
    case OpCode::F32__ne:
      Status = runNeFOp<float>(Val1.get(), Val2.get());
      break;
    case OpCode::F32__lt:
      Status = runLtFOp<float>(Val1.get(), Val2.get());
      break;
    case OpCode::F32__gt:
      Status = runGtFOp<float>(Val1.get(), Val2.get());
      break;
    case OpCode::F32__le:
      Status = runLeFOp<float>(Val1.get(), Val2.get());
      break;
    case OpCode::F32__ge:
      Status = runGeFOp<float>(Val1.get(), Val2.get());
      break;
    case OpCode::F64__eq:
      Status = runEqFOp<double>(Val1.get(), Val2.get());
      break;
    case OpCode::F64__ne:
      Status = runNeFOp<double>(Val1.get(), Val2.get());
      break;
    case OpCode::F64__lt:
      Status = runLtFOp<double>(Val1.get(), Val2.get());
      break;
    case OpCode::F64__gt:
      Status = runGtFOp<double>(Val1.get(), Val2.get());
      break;
    case OpCode::F64__le:
      Status = runLeFOp<double>(Val1.get(), Val2.get());
      break;
    case OpCode::F64__ge:
      Status = runGeFOp<double>(Val1.get(), Val2.get());
      break;
    default:
      Status = ErrCode::InstructionTypeMismatch;
      break;
    }
  } else if (isBinaryNumericOp(Opcode)) {
    std::unique_ptr<ValueEntry> Val1, Val2;
    StackMgr.pop(Val2);
    StackMgr.pop(Val1);

    if (!isValueTypeEqual(*Val1.get(), *Val2.get())) {
      return ErrCode::TypeNotMatch;
    }

    switch (Opcode) {
    case OpCode::I32__add:
      Status = runAddOp<int32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__sub:
      Status = runSubOp<int32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__mul:
      Status = runMulOp<int32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__div_s:
      // Status = runDivSOp<int32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__div_u:
      Status = runDivUOp<int32_t>(Val1.get(), Val2.get());
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
      Status = runRemUOp<int64_t>(Val1.get(), Val2.get());
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

ErrCode Worker::enterBlock(unsigned int Arity, AST::Instruction *Instr,
                           const InstrVec &Seq) {
  /// Create label for block.
  std::unique_ptr<LabelEntry> Label;
  if (Instr == nullptr)
    Label = std::make_unique<LabelEntry>(Arity);
  else
    Label = std::make_unique<LabelEntry>(Arity, Instr);

  /// Push label and jump to block body.
  StackMgr.push(Label);
  return InstrPdr.pushInstrs(InstrProvider::SeqType::Block, Seq);
}

ErrCode Worker::leaveBlock() {
  /// Pop top values on stack until a label.
  std::vector<std::unique_ptr<ValueEntry>> Vals;
  while (!StackMgr.isTopLabel()) {
    std::unique_ptr<ValueEntry> Val = nullptr;
    StackMgr.pop(Val);
    Vals.push_back(std::move(Val));
  }

  /// Pop label entry and the corresponding instruction sequence.
  InstrPdr.popInstrs();
  StackMgr.pop();

  /// Push the Vals back into the Stack
  for (auto Iter = Vals.rbegin(); Iter != Vals.rend(); Iter++)
    StackMgr.push(*Iter);
  return ErrCode::Success;
}

ErrCode Worker::invokeFunction(unsigned int FuncAddr) {
  ErrCode Status = ErrCode::Success;

  /// Get Function Instance and module address.
  Instance::FunctionInstance *FuncInst = nullptr;
  Instance::ModuleInstance *ModuleInst = nullptr;
  if ((Status = StoreMgr.getFunction(FuncAddr, FuncInst)) != ErrCode::Success)
    return Status;
  if ((StoreMgr.getModule(FuncInst->getModuleAddr(), ModuleInst)) !=
      ErrCode::Success)
    return Status;

  /// Get function type
  Instance::ModuleInstance::FType *FuncType = nullptr;
  if ((ModuleInst->getFuncType(FuncInst->getTypeIdx(), FuncType)) !=
      ErrCode::Success)
    return Status;

  /// Pop argument vals
  std::vector<std::unique_ptr<ValueEntry>> Vals;
  for (unsigned int I = 0; I < FuncType->Params.size(); I++) {
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);
    Vals.push_back(std::move(Val));
  }

  /// Push frame with locals and args and set instruction vector
  unsigned int Arity = FuncType->Returns.size();
  InstrVec EmprySeq;
  auto Frame =
      std::make_unique<FrameEntry>(FuncInst->getModuleAddr(), /// Module address
                                   Arity,                     /// Arity
                                   Vals,                 /// Reversed arguments
                                   FuncInst->getLocals() /// Local defs
      );
  StackMgr.push(Frame);
  InstrPdr.pushInstrs(InstrProvider::SeqType::FunctionCall, EmprySeq);

  /// Run block of function body
  return enterBlock(Arity, nullptr, FuncInst->getInstrs());
}

ErrCode Worker::returnFunction() {
  /// Get current frame and arity.
  StackMgr.getCurrentFrame(CurrentFrame);
  unsigned int Arity = CurrentFrame->getArity();

  /// Pop the results from stack.
  std::vector<std::unique_ptr<ValueEntry>> Vals;
  for (unsigned int I = 0; I < Arity; I++) {
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);
    Vals.push_back(std::move(Val));
  }

  /// TODO: Validate top of stack is a frame when reach end of function.

  /// Pop until the top of stack is a frame.
  while (!StackMgr.isTopFrame()) {
    /// If pop a label, need to pop the instruction sequence of block.
    if (StackMgr.isTopLabel())
      InstrPdr.popInstrs();
    StackMgr.pop();
  }

  /// Pop the frame entry from the Stack.
  InstrPdr.popInstrs();
  StackMgr.pop();

  /// Push the retrun Vals into Stack.
  for (auto Iter = Vals.rbegin(); Iter != Vals.rend(); Iter++) {
    std::unique_ptr<ValueEntry> Val = std::move(*Iter);
    StackMgr.push(Val);
  }
  return ErrCode::Success;
}

ErrCode Worker::branchToLabel(unsigned int L) {
  /// Get the L-th label from top of stack and the continuation instruction.
  ErrCode Status = ErrCode::Success;
  LabelEntry *Label;
  AST::Instruction *ContInstr = nullptr;
  if ((Status = StackMgr.getLabelWithCount(Label, L)) != ErrCode::Success) {
    return Status;
  }
  ContInstr = Label->getTarget();

  /// Get arity of Label and pop n values.
  unsigned int Arity = Label->getArity();
  std::vector<std::unique_ptr<ValueEntry>> Vals;
  for (unsigned int I = 0; I < Arity; I++) {
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);
    Vals.push_back(std::move(Val));
  }

  /// Repeat LabelIndex + 1 times
  for (unsigned int I = 0; I < L + 1; I++) {
    while (StackMgr.isTopValue()) {
      StackMgr.pop();
    }
    /// Pop label entry and the corresponding instruction sequence.
    InstrPdr.popInstrs();
    StackMgr.pop();
  }

  /// Push the Vals back into the Stack
  for (auto Iter = Vals.rbegin(); Iter != Vals.rend(); Iter++) {
    std::unique_ptr<ValueEntry> Val = std::move(*Iter);
    StackMgr.push(Val);
  }

  /// Jump to the continuation of Label
  if (ContInstr != nullptr)
    Status = runLoopOp(dynamic_cast<AST::ControlInstruction *>(ContInstr));
  return Status;
}

} // namespace Executor
} // namespace SSVM
