#include "executor/worker.h"
#include "ast/common.h"
#include "ast/instruction.h"
#include "executor/worker/util.h"
#include "support/casting.h"

namespace SSVM {
namespace Executor {

namespace {
using OpCode = AST::Instruction::OpCode;
} // namespace

ErrCode Worker::runExpression(const AST::InstrVec &Instrs) {
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

  /// Enter start function. Args should be pushed into stack.
  ErrCode Status = ErrCode::Success;
  if ((Status = invokeFunction(FuncAddr)) != ErrCode::Success)
    return Status;

  /// Execute run loop.
  TheState = State::CodeSet;
  Status = execute();
  if (Status == ErrCode::Terminated) {
    /// Forced terminated case.
    return ErrCode::Success;
  }
  return Status;
}

ErrCode Worker::reset() {
  TheState = State::Inited;
  CurrentFrame = nullptr;
  InstrPdr.reset();
  return ErrCode::Success;
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
      StackMgr.getCurrentFrame(CurrentFrame);
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

ErrCode Worker::runControlOp(AST::Instruction *Instr) {
  /// Check OpCode and run the specific instruction.
  ErrCode Status = ErrCode::Success;
  switch (Instr->getOpCode()) {
  case OpCode::Unreachable:
    TheState = State::Unreachable;
    Status = ErrCode::Unreachable;
    break;
  case OpCode::Nop:
    break;
  case OpCode::Block:
    Status = runBlockOp(Instr);
    break;
  case OpCode::Loop:
    Status = runLoopOp(Instr);
    break;
  case OpCode::If:
    Status = runIfElseOp(Instr);
    break;
  case OpCode::Br:
    Status = runBrOp(Instr);
    break;
  case OpCode::Br_if:
    Status = runBrIfOp(Instr);
    break;
  case OpCode::Br_table:
    Status = runBrTableOp(Instr);
    break;
  case OpCode::Return:
    Status = runReturnOp();
    break;
  case OpCode::Call:
    Status = runCallOp(Instr);
    break;
  case OpCode::Call_indirect:
    Status = runCallIndirectOp(Instr);
    break;
  default:
    Status = ErrCode::InstructionTypeMismatch;
    break;
  }

  return Status;
}

ErrCode Worker::runParametricOp(AST::Instruction *Instr) {
  /// Check OpCode and run the specific instruction.
  ErrCode Status = ErrCode::Success;
  if (Instr->getOpCode() == OpCode::Drop) {
    StackMgr.pop();
  } else if (Instr->getOpCode() == OpCode::Select) {
    /// Pop the i32 value and select values from stack.
    std::unique_ptr<ValueEntry> CondValEntry, ValEntry1, ValEntry2;
    StackMgr.pop(CondValEntry);
    StackMgr.pop(ValEntry2);
    StackMgr.pop(ValEntry1);
    uint32_t CondValue;
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
  return Status;
}

ErrCode Worker::runVariableOp(AST::Instruction *Instr) {
  /// Get variable index.
  unsigned int Index = Instr->getVariableIndex();

  /// Check OpCode and run the specific instruction.
  ErrCode Status = ErrCode::Success;
  switch (Instr->getOpCode()) {
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

  return Status;
}

ErrCode Worker::runMemoryOp(AST::Instruction *Instr) {
  /// Check OpCode and run the specific instruction.
  ErrCode Status = ErrCode::Success;
  switch (Instr->getOpCode()) {
  case OpCode::I32__load:
    Status = runLoadOp<uint32_t>(Instr);
    break;
  case OpCode::I64__load:
    Status = runLoadOp<uint64_t>(Instr);
    break;
  case OpCode::F32__load:
    Status = runLoadOp<float>(Instr);
    break;
  case OpCode::F64__load:
    Status = runLoadOp<double>(Instr);
    break;
  case OpCode::I32__load8_s:
    Status = runLoadOp<int32_t>(Instr, 8);
    break;
  case OpCode::I32__load8_u:
    Status = runLoadOp<uint32_t>(Instr, 8);
    break;
  case OpCode::I32__load16_s:
    Status = runLoadOp<int32_t>(Instr, 16);
    break;
  case OpCode::I32__load16_u:
    Status = runLoadOp<uint32_t>(Instr, 16);
    break;
  case OpCode::I64__load8_s:
    Status = runLoadOp<int64_t>(Instr, 8);
    break;
  case OpCode::I64__load8_u:
    Status = runLoadOp<uint64_t>(Instr, 8);
    break;
  case OpCode::I64__load16_s:
    Status = runLoadOp<int64_t>(Instr, 16);
    break;
  case OpCode::I64__load16_u:
    Status = runLoadOp<uint64_t>(Instr, 16);
    break;
  case OpCode::I64__load32_s:
    Status = runLoadOp<int64_t>(Instr, 32);
    break;
  case OpCode::I64__load32_u:
    Status = runLoadOp<uint64_t>(Instr, 32);
    break;
  case OpCode::I32__store:
    Status = runStoreOp<uint32_t>(Instr);
    break;
  case OpCode::I64__store:
    Status = runStoreOp<uint64_t>(Instr);
    break;
  case OpCode::F32__store:
    Status = runStoreOp<float>(Instr);
    break;
  case OpCode::F64__store:
    Status = runStoreOp<double>(Instr);
    break;
  case OpCode::I32__store8:
    Status = runStoreOp<uint32_t>(Instr, 8);
    break;
  case OpCode::I32__store16:
    Status = runStoreOp<uint32_t>(Instr, 16);
    break;
  case OpCode::I64__store8:
    Status = runStoreOp<uint64_t>(Instr, 8);
    break;
  case OpCode::I64__store16:
    Status = runStoreOp<uint64_t>(Instr, 16);
    break;
  case OpCode::I64__store32:
    Status = runStoreOp<uint64_t>(Instr, 32);
    break;
  case OpCode::Memory__grow:
    Status = runMemoryGrowOp();
    break;
  case OpCode::Memory__size:
    Status = runMemorySizeOp();
    break;
  default:
    Status = ErrCode::InstructionTypeMismatch;
    break;
  }
  return Status;
}

ErrCode Worker::runConstNumericOp(AST::Instruction *Instr) {
  std::unique_ptr<ValueEntry> VE = nullptr;
  std::visit(
      [&VE](auto &&arg) {
        VE = std::make_unique<ValueEntry>();
        VE->InitValueEntry(arg);
      },
             Instr->getConstValue());
  StackMgr.push(VE);

  return ErrCode::Success;
}

ErrCode Worker::runNumericOp(AST::Instruction *Instr) {
  /// Check OpCode and run the specific instruction.
  auto Opcode = Instr->getOpCode();
  ErrCode Status = ErrCode::Success;
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
      Status = runEqOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__ne:
      Status = runNeOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__lt_s:
      Status = runLtOp<int32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__lt_u:
      Status = runLtOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__gt_s:
      Status = runGtOp<int32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__gt_u:
      Status = runGtOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__le_s:
      Status = runLeOp<int32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__le_u:
      Status = runLeOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__ge_s:
      Status = runGeOp<int32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__ge_u:
      Status = runGeOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__eq:
      Status = runEqOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__ne:
      Status = runNeOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__lt_s:
      Status = runLtOp<int64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__lt_u:
      Status = runLtOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__gt_s:
      Status = runGtOp<int64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__gt_u:
      Status = runGtOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__le_s:
      Status = runLeOp<int64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__le_u:
      Status = runLeOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__ge_s:
      Status = runGeOp<int64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__ge_u:
      Status = runGeOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::F32__eq:
      Status = runEqOp<float>(Val1.get(), Val2.get());
      break;
    case OpCode::F32__ne:
      Status = runNeOp<float>(Val1.get(), Val2.get());
      break;
    case OpCode::F32__lt:
      Status = runLtOp<float>(Val1.get(), Val2.get());
      break;
    case OpCode::F32__gt:
      Status = runGtOp<float>(Val1.get(), Val2.get());
      break;
    case OpCode::F32__le:
      Status = runLeOp<float>(Val1.get(), Val2.get());
      break;
    case OpCode::F32__ge:
      Status = runGeOp<float>(Val1.get(), Val2.get());
      break;
    case OpCode::F64__eq:
      Status = runEqOp<double>(Val1.get(), Val2.get());
      break;
    case OpCode::F64__ne:
      Status = runNeOp<double>(Val1.get(), Val2.get());
      break;
    case OpCode::F64__lt:
      Status = runLtOp<double>(Val1.get(), Val2.get());
      break;
    case OpCode::F64__gt:
      Status = runGtOp<double>(Val1.get(), Val2.get());
      break;
    case OpCode::F64__le:
      Status = runLeOp<double>(Val1.get(), Val2.get());
      break;
    case OpCode::F64__ge:
      Status = runGeOp<double>(Val1.get(), Val2.get());
      break;
    default:
      Status = ErrCode::InstructionTypeMismatch;
      break;
    }
  } else if (isUnaryNumericOp(Opcode)) {
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);

    switch (Opcode) {
    case OpCode::I32__clz:
      Status = runClzOp<uint32_t>(Val.get());
      break;
    case OpCode::I32__ctz:
      Status = runCtzOp<uint32_t>(Val.get());
      break;
    case OpCode::I32__popcnt:
      Status = runPopcntOp<uint32_t>(Val.get());
      break;
    case OpCode::I64__clz:
      Status = runClzOp<uint64_t>(Val.get());
      break;
    case OpCode::I64__ctz:
      Status = runCtzOp<uint64_t>(Val.get());
      break;
    case OpCode::I64__popcnt:
      Status = runPopcntOp<uint64_t>(Val.get());
      break;
    case OpCode::F32__abs:
      Status = runAbsOp<float>(Val.get());
      break;
    case OpCode::F32__neg:
      Status = runNegOp<float>(Val.get());
      break;
    case OpCode::F32__ceil:
      Status = runCeilOp<float>(Val.get());
      break;
    case OpCode::F32__floor:
      Status = runFloorOp<float>(Val.get());
      break;
    case OpCode::F32__nearest:
      Status = runNearestOp<float>(Val.get());
      break;
    case OpCode::F32__sqrt:
      Status = runSqrtOp<float>(Val.get());
      break;
    case OpCode::F64__abs:
      Status = runAbsOp<double>(Val.get());
      break;
    case OpCode::F64__neg:
      Status = runNegOp<double>(Val.get());
      break;
    case OpCode::F64__ceil:
      Status = runCeilOp<double>(Val.get());
      break;
    case OpCode::F64__floor:
      Status = runFloorOp<double>(Val.get());
      break;
    case OpCode::F64__nearest:
      Status = runNearestOp<double>(Val.get());
      break;
    case OpCode::F64__sqrt:
      Status = runSqrtOp<double>(Val.get());
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
      Status = runAddOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__sub:
      Status = runSubOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__mul:
      Status = runMulOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__div_s:
      Status = runDivOp<int32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__div_u:
      Status = runDivOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__rem_s:
      Status = runRemOp<int32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__rem_u:
      Status = runRemOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__and:
      Status = runAndOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__or:
      Status = runOrOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__xor:
      Status = runXorOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__shl:
      Status = runShlOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__shr_s:
      Status = runShrOp<int32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__shr_u:
      Status = runShrOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__rotl:
      Status = runRotlOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I32__rotr:
      Status = runRotrOp<uint32_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__add:
      Status = runAddOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__sub:
      Status = runSubOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__mul:
      Status = runMulOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__div_s:
      Status = runDivOp<int64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__div_u:
      Status = runDivOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__rem_s:
      Status = runRemOp<int64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__rem_u:
      Status = runRemOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__and:
      Status = runAndOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__or:
      Status = runOrOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__xor:
      Status = runXorOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__shl:
      Status = runShlOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__shr_s:
      Status = runShrOp<int64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__shr_u:
      Status = runShrOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__rotl:
      Status = runRotlOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::I64__rotr:
      Status = runRotrOp<uint64_t>(Val1.get(), Val2.get());
      break;
    case OpCode::F32__add:
      Status = runAddOp<float>(Val1.get(), Val2.get());
      break;
    case OpCode::F32__sub:
      Status = runSubOp<float>(Val1.get(), Val2.get());
      break;
    case OpCode::F32__mul:
      Status = runMulOp<float>(Val1.get(), Val2.get());
      break;
    case OpCode::F32__div:
      Status = runDivOp<float>(Val1.get(), Val2.get());
      break;
    case OpCode::F32__min:
      Status = runMinOp<float>(Val1.get(), Val2.get());
      break;
    case OpCode::F32__max:
      Status = runMaxOp<float>(Val1.get(), Val2.get());
      break;
    case OpCode::F32__copysign:
      Status = runCopysignOp<float>(Val1.get(), Val2.get());
      break;
    case OpCode::F64__add:
      Status = runAddOp<double>(Val1.get(), Val2.get());
      break;
    case OpCode::F64__sub:
      Status = runSubOp<double>(Val1.get(), Val2.get());
      break;
    case OpCode::F64__mul:
      Status = runMulOp<double>(Val1.get(), Val2.get());
      break;
    case OpCode::F64__div:
      Status = runDivOp<double>(Val1.get(), Val2.get());
      break;
    case OpCode::F64__min:
      Status = runMinOp<double>(Val1.get(), Val2.get());
      break;
    case OpCode::F64__max:
      Status = runMaxOp<double>(Val1.get(), Val2.get());
      break;
    case OpCode::F64__copysign:
      Status = runCopysignOp<double>(Val1.get(), Val2.get());
      break;
    default:
      Status = ErrCode::InstructionTypeMismatch;
      break;
    }
  } else if (isCastNumericOp(Opcode)) {
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);

    switch (Opcode) {
    case OpCode::I32__wrap_i64:
      Status = runWrapOp<uint64_t, uint32_t>(Val.get());
      break;
    case OpCode::I32__trunc_f32_s:
      Status = runTruncateOp<float, int32_t>(Val.get());
      break;
    case OpCode::I32__trunc_f32_u:
      Status = runTruncateOp<float, uint32_t>(Val.get());
      break;
    case OpCode::I32__trunc_f64_s:
      Status = runTruncateOp<double, int32_t>(Val.get());
      break;
    case OpCode::I32__trunc_f64_u:
      Status = runTruncateOp<double, uint32_t>(Val.get());
      break;
    case OpCode::I64__extend_i32_s:
      Status = runExtendOp<int32_t, uint64_t>(Val.get());
      break;
    case OpCode::I64__extend_i32_u:
      Status = runExtendOp<uint32_t, uint64_t>(Val.get());
      break;
    case OpCode::I64__trunc_f32_s:
      Status = runTruncateOp<float, int64_t>(Val.get());
      break;
    case OpCode::I64__trunc_f32_u:
      Status = runTruncateOp<float, uint64_t>(Val.get());
      break;
    case OpCode::I64__trunc_f64_s:
      Status = runTruncateOp<double, int64_t>(Val.get());
      break;
    case OpCode::I64__trunc_f64_u:
      Status = runTruncateOp<double, uint64_t>(Val.get());
      break;
    case OpCode::F32__convert_i32_s:
      Status = runConvertOp<int32_t, float>(Val.get());
      break;
    case OpCode::F32__convert_i32_u:
      Status = runConvertOp<uint32_t, float>(Val.get());
      break;
    case OpCode::F32__convert_i64_s:
      Status = runConvertOp<int64_t, float>(Val.get());
      break;
    case OpCode::F32__convert_i64_u:
      Status = runConvertOp<uint64_t, float>(Val.get());
      break;
    case OpCode::F32__demote_f64:
      Status = runDemoteOp<double, float>(Val.get());
      break;
    case OpCode::F64__convert_i32_s:
      Status = runConvertOp<int32_t, double>(Val.get());
      break;
    case OpCode::F64__convert_i32_u:
      Status = runConvertOp<uint32_t, double>(Val.get());
      break;
    case OpCode::F64__convert_i64_s:
      Status = runConvertOp<int64_t, double>(Val.get());
      break;
    case OpCode::F64__convert_i64_u:
      Status = runConvertOp<uint64_t, double>(Val.get());
      break;
    case OpCode::F64__promote_f32:
      Status = runPromoteOp<float, double>(Val.get());
      break;
    case OpCode::I32__reinterpret_f32:
      Status = runReinterpretOp<float, uint32_t>(Val.get());
      break;
    case OpCode::I64__reinterpret_f64:
      Status = runReinterpretOp<double, uint64_t>(Val.get());
      break;
    case OpCode::F32__reinterpret_i32:
      Status = runReinterpretOp<uint32_t, float>(Val.get());
      break;
    case OpCode::F64__reinterpret_i64:
      Status = runReinterpretOp<uint64_t, double>(Val.get());
      break;
    default:
      Status = ErrCode::InstructionTypeMismatch;
      break;
    }
  } else {
    Status = ErrCode::InstructionTypeMismatch;
  }
  return Status;
}

ErrCode Worker::enterBlock(unsigned int Arity, AST::Instruction *Instr,
                           const AST::InstrVec &Seq) {
  /// Create label for block.
  std::unique_ptr<LabelEntry> Label = std::make_unique<LabelEntry>();
  if (Instr == nullptr) {
    Label->InitLabelEntry(Arity);
  } else {
    Label->InitLabelEntry(Arity, Instr);
  }

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
  if ((Status = StoreMgr.getFunction(FuncAddr, FuncInst)) != ErrCode::Success)
    return Status;

  /// Get function type
  Instance::ModuleInstance::FType *FuncType = FuncInst->getFuncType();

  /// Pop argument vals
  std::vector<std::unique_ptr<ValueEntry>> Vals;
  for (unsigned int I = 0; I < FuncType->Params.size(); I++) {
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);
    Vals.push_back(std::move(Val));
  }

  if (FuncInst->isHostFunction()) {
    /// Host function case: Push args and call function.
    Instance::ModuleInstance *ModuleInst = nullptr;
    if ((Status = StoreMgr.getModule(CurrentFrame->getModuleAddr(),
                                     ModuleInst)) != ErrCode::Success) {
      return Status;
    }
    HostFunction *HostFunc = nullptr;
    if ((Status = HostFuncMgr.getHostFunction(FuncInst->getHostFuncAddr(),
                                              HostFunc)) != ErrCode::Success) {
      return Status;
    }

    /// Prepare return list.
    std::vector<std::unique_ptr<ValueEntry>> Returns;
    for (auto It = FuncType->Returns.cbegin(); It != FuncType->Returns.cend();
         It++) {
      std::unique_ptr<ValueEntry> VE = std::make_unique<ValueEntry>();
      VE->InitValueEntry(*It);
      Returns.push_back(std::move(VE));
    }

    if ((Status = HostFunc->run(Vals, Returns, StoreMgr, ModuleInst)) !=
        ErrCode::Success) {
      return Status;
    }

    /// Push result value into stack.
    for (auto Iter = Returns.rbegin(); Iter != Returns.rend(); Iter++) {
      StackMgr.push(std::move(*Iter));
    }
    return Status;
  } else {
    /// Native function case: Push frame with locals and args.
    unsigned int Arity = FuncType->Returns.size();
    AST::InstrVec EmprySeq;
    auto Frame = std::make_unique<FrameEntry>();
    Frame->InitFrameEntry(FuncInst->getModuleAddr(), /// Module address
        Arity,                     /// Arity
        Vals,                      /// Reversed arguments
        FuncInst->getLocals()      /// Local defs
    );
    StackMgr.push(Frame);
    InstrPdr.pushInstrs(InstrProvider::SeqType::FunctionCall, EmprySeq);

    /// Run block of function body
    return enterBlock(Arity, nullptr, FuncInst->getInstrs());
  }
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
  if (ContInstr != nullptr) {
    Status = runLoopOp(ContInstr);
  }
  return Status;
}

ErrCode Worker::getTabInstByIdx(unsigned int Idx,
                                Instance::TableInstance *&TabInst) {
  ErrCode Status = ErrCode::Success;
  Instance::ModuleInstance *ModInst = nullptr;
  unsigned int TableAddr = 0;
  unsigned int ModuleAddr = CurrentFrame->getModuleAddr();
  if ((Status = StoreMgr.getModule(ModuleAddr, ModInst)) != ErrCode::Success) {
    return Status;
  };
  if ((Status = ModInst->getTableAddr(Idx, TableAddr)) != ErrCode::Success) {
    return Status;
  };
  return StoreMgr.getTable(TableAddr, TabInst);
}

ErrCode Worker::getMemInstByIdx(unsigned int Idx,
                                Instance::MemoryInstance *&MemInst) {
  ErrCode Status = ErrCode::Success;
  Instance::ModuleInstance *ModInst = nullptr;
  unsigned int MemoryAddr = 0;
  unsigned int ModuleAddr = CurrentFrame->getModuleAddr();
  if ((Status = StoreMgr.getModule(ModuleAddr, ModInst)) != ErrCode::Success) {
    return Status;
  };
  if ((Status = ModInst->getMemAddr(Idx, MemoryAddr)) != ErrCode::Success) {
    return Status;
  };
  return StoreMgr.getMemory(MemoryAddr, MemInst);
}

ErrCode Worker::getGlobInstByIdx(unsigned int Idx,
                                 Instance::GlobalInstance *&GlobInst) {
  ErrCode Status = ErrCode::Success;
  Instance::ModuleInstance *ModInst = nullptr;
  unsigned int GlobalAddr = 0;
  unsigned int ModuleAddr = CurrentFrame->getModuleAddr();
  if ((Status = StoreMgr.getModule(ModuleAddr, ModInst)) != ErrCode::Success) {
    return Status;
  };
  if ((Status = ModInst->getGlobalAddr(Idx, GlobalAddr)) != ErrCode::Success) {
    return Status;
  };
  return StoreMgr.getGlobal(GlobalAddr, GlobInst);
}

} // namespace Executor
} // namespace SSVM
