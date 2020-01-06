// SPDX-License-Identifier: Apache-2.0
#include "executor/worker.h"
#include "ast/common.h"
#include "ast/instruction.h"
#include "executor/worker/util.h"
#include "support/casting.h"

#include <functional>
#include <iostream>
#include <stdio.h>
#include <sys/time.h>

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
  if (ErrCode Status = invokeFunction(FuncAddr); Status != ErrCode::Success)
    return Status;

  /// Set start time.
  TimeRecorder.startRecord(TIMER_TAG_EXECUTION);

  /// Execute run loop.
  std::cout << " Info: Start running..." << std::endl;
  TheState = State::CodeSet;
  ErrCode Status = execute();
  if (Status == ErrCode::Revert) {
    std::cout << " Error: Reverted." << std::endl;
  } else if (Status == ErrCode::Terminated) {
    std::cout << " Info: Terminated." << std::endl;
  } else if (Status != ErrCode::Success) {
    std::cout << " Error: Worker execution failed. Code: "
              << (unsigned int)Status << std::endl;
  } else {
    std::cout << " Info: Worker execution succeeded." << std::endl;
  }

  /// Print time cost.
  uint64_t ExecTime = TimeRecorder.stopRecord(TIMER_TAG_EXECUTION);
  uint64_t HostFuncTime = TimeRecorder.getRecord(TIMER_TAG_HOSTFUNC);
  std::cout << " =================  Statistics  =================" << std::endl
            << " Total execution time: " << ExecTime + HostFuncTime << " us"
            << std::endl
            << " Wasm instructions execution time: " << ExecTime << " us"
            << std::endl
            << " Host functions execution time: " << HostFuncTime << " us"
            << std::endl
            << " Executed wasm instructions count: " << ExecInstrCnt
            << std::endl
#ifndef ONNC_WASM
            << " Gas costs: " << EnvMgr.getCostSum() << std::endl
#endif
            << " Instructions per second: "
            << static_cast<uint64_t>((double)ExecInstrCnt * 1000000 / ExecTime)
            << std::endl;

  if (Status == ErrCode::Terminated) {
    /// Forced terminated case.
    return ErrCode::Success;
  }
  return Status;
}

ErrCode Worker::reset() {
  TheState = State::Inited;
  InstrPdr.reset();
  StackMgr.reset();
  ExecInstrCnt = 0;
  return ErrCode::Success;
}

ErrCode Worker::execute(AST::ControlInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Unreachable:
    TheState = State::Unreachable;
    return ErrCode::Unreachable;
  case OpCode::Nop:
    return ErrCode::Success;
  case OpCode::Return:
    return runReturnOp();
  default:
    __builtin_unreachable();
  }
}
ErrCode Worker::execute(AST::BlockControlInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Block:
    return runBlockOp(Instr);
  case OpCode::Loop:
    return runLoopOp(Instr);
  default:
    __builtin_unreachable();
  }
}
ErrCode Worker::execute(AST::IfElseControlInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::If:
    return runIfElseOp(Instr);
  default:
    __builtin_unreachable();
  }
}
ErrCode Worker::execute(AST::BrControlInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Br:
    return runBrOp(Instr);
  case OpCode::Br_if:
    return runBrIfOp(Instr);
  default:
    __builtin_unreachable();
  }
}
ErrCode Worker::execute(AST::BrTableControlInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Br_table:
    return runBrTableOp(Instr);
  default:
    __builtin_unreachable();
  }
}
ErrCode Worker::execute(AST::CallControlInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Call:
    return runCallOp(Instr);
  case OpCode::Call_indirect:
    return runCallIndirectOp(Instr);
  default:
    __builtin_unreachable();
  }
}
ErrCode Worker::execute(AST::ParametricInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Drop:
    StackMgr.pop();
    return ErrCode::Success;
  case OpCode::Select: {
    /// Pop the i32 value and select values from stack.
    Value CondVal, Val1, Val2;
    StackMgr.pop(CondVal);
    StackMgr.pop(Val2);
    StackMgr.pop(Val1);
    uint32_t CondValue = retrieveValue<uint32_t>(CondVal);

    /// Select the value.
    if (CondValue == 0) {
      StackMgr.push(Val2);
    } else {
      StackMgr.push(Val1);
    }

    return ErrCode::Success;
  }
  default:
    __builtin_unreachable();
  }
}
ErrCode Worker::execute(AST::VariableInstruction &Instr) {
  /// Get variable index.
  unsigned int Index = Instr.getVariableIndex();

  /// Check OpCode and run the specific instruction.
  switch (Instr.getOpCode()) {
  case OpCode::Local__get:
    return runLocalGetOp(Index);
  case OpCode::Local__set:
    return runLocalSetOp(Index);
  case OpCode::Local__tee:
    return runLocalTeeOp(Index);
  case OpCode::Global__get:
    return runGlobalGetOp(Index);
  case OpCode::Global__set:
    return runGlobalSetOp(Index);
  default:
    __builtin_unreachable();
  }
}
ErrCode Worker::execute(AST::MemoryInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::I32__load:
    return runLoadOp<uint32_t>(Instr);
  case OpCode::I64__load:
    return runLoadOp<uint64_t>(Instr);
  case OpCode::F32__load:
    return runLoadOp<float>(Instr);
  case OpCode::F64__load:
    return runLoadOp<double>(Instr);
  case OpCode::I32__load8_s:
    return runLoadOp<int32_t>(Instr, 8);
  case OpCode::I32__load8_u:
    return runLoadOp<uint32_t>(Instr, 8);
  case OpCode::I32__load16_s:
    return runLoadOp<int32_t>(Instr, 16);
  case OpCode::I32__load16_u:
    return runLoadOp<uint32_t>(Instr, 16);
  case OpCode::I64__load8_s:
    return runLoadOp<int64_t>(Instr, 8);
  case OpCode::I64__load8_u:
    return runLoadOp<uint64_t>(Instr, 8);
  case OpCode::I64__load16_s:
    return runLoadOp<int64_t>(Instr, 16);
  case OpCode::I64__load16_u:
    return runLoadOp<uint64_t>(Instr, 16);
  case OpCode::I64__load32_s:
    return runLoadOp<int64_t>(Instr, 32);
  case OpCode::I64__load32_u:
    return runLoadOp<uint64_t>(Instr, 32);
  case OpCode::I32__store:
    return runStoreOp<uint32_t>(Instr);
  case OpCode::I64__store:
    return runStoreOp<uint64_t>(Instr);
  case OpCode::F32__store:
    return runStoreOp<float>(Instr);
  case OpCode::F64__store:
    return runStoreOp<double>(Instr);
  case OpCode::I32__store8:
    return runStoreOp<uint32_t>(Instr, 8);
  case OpCode::I32__store16:
    return runStoreOp<uint32_t>(Instr, 16);
  case OpCode::I64__store8:
    return runStoreOp<uint64_t>(Instr, 8);
  case OpCode::I64__store16:
    return runStoreOp<uint64_t>(Instr, 16);
  case OpCode::I64__store32:
    return runStoreOp<uint64_t>(Instr, 32);
  case OpCode::Memory__grow:
    return runMemoryGrowOp();
  case OpCode::Memory__size:
    return runMemorySizeOp();
  default:
    __builtin_unreachable();
  }
}
ErrCode Worker::execute(AST::ConstInstruction &Instr) {
  return StackMgr.push(Instr.getConstValue());
}
ErrCode Worker::execute(AST::UnaryNumericInstruction &Instr) {
  Value &Val = StackMgr.getTop();
  switch (Instr.getOpCode()) {
  case OpCode::I32__eqz:
    return runEqzOp<uint32_t>(Val);
  case OpCode::I64__eqz:
    return runEqzOp<uint64_t>(Val);
  case OpCode::I32__clz:
    return runClzOp<uint32_t>(Val);
  case OpCode::I32__ctz:
    return runCtzOp<uint32_t>(Val);
  case OpCode::I32__popcnt:
    return runPopcntOp<uint32_t>(Val);
  case OpCode::I64__clz:
    return runClzOp<uint64_t>(Val);
  case OpCode::I64__ctz:
    return runCtzOp<uint64_t>(Val);
  case OpCode::I64__popcnt:
    return runPopcntOp<uint64_t>(Val);
  case OpCode::F32__abs:
    return runAbsOp<float>(Val);
  case OpCode::F32__neg:
    return runNegOp<float>(Val);
  case OpCode::F32__ceil:
    return runCeilOp<float>(Val);
  case OpCode::F32__floor:
    return runFloorOp<float>(Val);
  case OpCode::F32__nearest:
    return runNearestOp<float>(Val);
  case OpCode::F32__sqrt:
    return runSqrtOp<float>(Val);
  case OpCode::F64__abs:
    return runAbsOp<double>(Val);
  case OpCode::F64__neg:
    return runNegOp<double>(Val);
  case OpCode::F64__ceil:
    return runCeilOp<double>(Val);
  case OpCode::F64__floor:
    return runFloorOp<double>(Val);
  case OpCode::F64__nearest:
    return runNearestOp<double>(Val);
  case OpCode::F64__sqrt:
    return runSqrtOp<double>(Val);
  case OpCode::I32__wrap_i64:
    return runWrapOp<uint64_t, uint32_t>(Val);
  case OpCode::I32__trunc_f32_s:
    return runTruncateOp<float, int32_t>(Val);
  case OpCode::I32__trunc_f32_u:
    return runTruncateOp<float, uint32_t>(Val);
  case OpCode::I32__trunc_f64_s:
    return runTruncateOp<double, int32_t>(Val);
  case OpCode::I32__trunc_f64_u:
    return runTruncateOp<double, uint32_t>(Val);
  case OpCode::I64__extend_i32_s:
    return runExtendOp<int32_t, uint64_t>(Val);
  case OpCode::I64__extend_i32_u:
    return runExtendOp<uint32_t, uint64_t>(Val);
  case OpCode::I64__trunc_f32_s:
    return runTruncateOp<float, int64_t>(Val);
  case OpCode::I64__trunc_f32_u:
    return runTruncateOp<float, uint64_t>(Val);
  case OpCode::I64__trunc_f64_s:
    return runTruncateOp<double, int64_t>(Val);
  case OpCode::I64__trunc_f64_u:
    return runTruncateOp<double, uint64_t>(Val);
  case OpCode::F32__convert_i32_s:
    return runConvertOp<int32_t, float>(Val);
  case OpCode::F32__convert_i32_u:
    return runConvertOp<uint32_t, float>(Val);
  case OpCode::F32__convert_i64_s:
    return runConvertOp<int64_t, float>(Val);
  case OpCode::F32__convert_i64_u:
    return runConvertOp<uint64_t, float>(Val);
  case OpCode::F32__demote_f64:
    return runDemoteOp<double, float>(Val);
  case OpCode::F64__convert_i32_s:
    return runConvertOp<int32_t, double>(Val);
  case OpCode::F64__convert_i32_u:
    return runConvertOp<uint32_t, double>(Val);
  case OpCode::F64__convert_i64_s:
    return runConvertOp<int64_t, double>(Val);
  case OpCode::F64__convert_i64_u:
    return runConvertOp<uint64_t, double>(Val);
  case OpCode::F64__promote_f32:
    return runPromoteOp<float, double>(Val);
  case OpCode::I32__reinterpret_f32:
    return runReinterpretOp<float, uint32_t>(Val);
  case OpCode::I64__reinterpret_f64:
    return runReinterpretOp<double, uint64_t>(Val);
  case OpCode::F32__reinterpret_i32:
    return runReinterpretOp<uint32_t, float>(Val);
  case OpCode::F64__reinterpret_i64:
    return runReinterpretOp<uint64_t, double>(Val);
  default:
    __builtin_unreachable();
  }
}
ErrCode Worker::execute(AST::BinaryNumericInstruction &Instr) {
  Value Val2;
  StackMgr.pop(Val2);
  Value &Val1 = StackMgr.getTop();

  switch (Instr.getOpCode()) {
  case OpCode::I32__eq:
    return runEqOp<uint32_t>(Val1, Val2);
  case OpCode::I32__ne:
    return runNeOp<uint32_t>(Val1, Val2);
  case OpCode::I32__lt_s:
    return runLtOp<int32_t>(Val1, Val2);
  case OpCode::I32__lt_u:
    return runLtOp<uint32_t>(Val1, Val2);
  case OpCode::I32__gt_s:
    return runGtOp<int32_t>(Val1, Val2);
  case OpCode::I32__gt_u:
    return runGtOp<uint32_t>(Val1, Val2);
  case OpCode::I32__le_s:
    return runLeOp<int32_t>(Val1, Val2);
  case OpCode::I32__le_u:
    return runLeOp<uint32_t>(Val1, Val2);
  case OpCode::I32__ge_s:
    return runGeOp<int32_t>(Val1, Val2);
  case OpCode::I32__ge_u:
    return runGeOp<uint32_t>(Val1, Val2);
  case OpCode::I64__eq:
    return runEqOp<uint64_t>(Val1, Val2);
  case OpCode::I64__ne:
    return runNeOp<uint64_t>(Val1, Val2);
  case OpCode::I64__lt_s:
    return runLtOp<int64_t>(Val1, Val2);
  case OpCode::I64__lt_u:
    return runLtOp<uint64_t>(Val1, Val2);
  case OpCode::I64__gt_s:
    return runGtOp<int64_t>(Val1, Val2);
  case OpCode::I64__gt_u:
    return runGtOp<uint64_t>(Val1, Val2);
  case OpCode::I64__le_s:
    return runLeOp<int64_t>(Val1, Val2);
  case OpCode::I64__le_u:
    return runLeOp<uint64_t>(Val1, Val2);
  case OpCode::I64__ge_s:
    return runGeOp<int64_t>(Val1, Val2);
  case OpCode::I64__ge_u:
    return runGeOp<uint64_t>(Val1, Val2);
  case OpCode::F32__eq:
    return runEqOp<float>(Val1, Val2);
  case OpCode::F32__ne:
    return runNeOp<float>(Val1, Val2);
  case OpCode::F32__lt:
    return runLtOp<float>(Val1, Val2);
  case OpCode::F32__gt:
    return runGtOp<float>(Val1, Val2);
  case OpCode::F32__le:
    return runLeOp<float>(Val1, Val2);
  case OpCode::F32__ge:
    return runGeOp<float>(Val1, Val2);
  case OpCode::F64__eq:
    return runEqOp<double>(Val1, Val2);
  case OpCode::F64__ne:
    return runNeOp<double>(Val1, Val2);
  case OpCode::F64__lt:
    return runLtOp<double>(Val1, Val2);
  case OpCode::F64__gt:
    return runGtOp<double>(Val1, Val2);
  case OpCode::F64__le:
    return runLeOp<double>(Val1, Val2);
  case OpCode::F64__ge:
    return runGeOp<double>(Val1, Val2);
  case OpCode::I32__add:
    return runAddOp<uint32_t>(Val1, Val2);
  case OpCode::I32__sub:
    return runSubOp<uint32_t>(Val1, Val2);
  case OpCode::I32__mul:
    return runMulOp<uint32_t>(Val1, Val2);
  case OpCode::I32__div_s:
    return runDivOp<int32_t>(Val1, Val2);
  case OpCode::I32__div_u:
    return runDivOp<uint32_t>(Val1, Val2);
  case OpCode::I32__rem_s:
    return runRemOp<int32_t>(Val1, Val2);
  case OpCode::I32__rem_u:
    return runRemOp<uint32_t>(Val1, Val2);
  case OpCode::I32__and:
    return runAndOp<uint32_t>(Val1, Val2);
  case OpCode::I32__or:
    return runOrOp<uint32_t>(Val1, Val2);
  case OpCode::I32__xor:
    return runXorOp<uint32_t>(Val1, Val2);
  case OpCode::I32__shl:
    return runShlOp<uint32_t>(Val1, Val2);
  case OpCode::I32__shr_s:
    return runShrOp<int32_t>(Val1, Val2);
  case OpCode::I32__shr_u:
    return runShrOp<uint32_t>(Val1, Val2);
  case OpCode::I32__rotl:
    return runRotlOp<uint32_t>(Val1, Val2);
  case OpCode::I32__rotr:
    return runRotrOp<uint32_t>(Val1, Val2);
  case OpCode::I64__add:
    return runAddOp<uint64_t>(Val1, Val2);
  case OpCode::I64__sub:
    return runSubOp<uint64_t>(Val1, Val2);
  case OpCode::I64__mul:
    return runMulOp<uint64_t>(Val1, Val2);
  case OpCode::I64__div_s:
    return runDivOp<int64_t>(Val1, Val2);
  case OpCode::I64__div_u:
    return runDivOp<uint64_t>(Val1, Val2);
  case OpCode::I64__rem_s:
    return runRemOp<int64_t>(Val1, Val2);
  case OpCode::I64__rem_u:
    return runRemOp<uint64_t>(Val1, Val2);
  case OpCode::I64__and:
    return runAndOp<uint64_t>(Val1, Val2);
  case OpCode::I64__or:
    return runOrOp<uint64_t>(Val1, Val2);
  case OpCode::I64__xor:
    return runXorOp<uint64_t>(Val1, Val2);
  case OpCode::I64__shl:
    return runShlOp<uint64_t>(Val1, Val2);
  case OpCode::I64__shr_s:
    return runShrOp<int64_t>(Val1, Val2);
  case OpCode::I64__shr_u:
    return runShrOp<uint64_t>(Val1, Val2);
  case OpCode::I64__rotl:
    return runRotlOp<uint64_t>(Val1, Val2);
  case OpCode::I64__rotr:
    return runRotrOp<uint64_t>(Val1, Val2);
  case OpCode::F32__add:
    return runAddOp<float>(Val1, Val2);
  case OpCode::F32__sub:
    return runSubOp<float>(Val1, Val2);
  case OpCode::F32__mul:
    return runMulOp<float>(Val1, Val2);
  case OpCode::F32__div:
    return runDivOp<float>(Val1, Val2);
  case OpCode::F32__min:
    return runMinOp<float>(Val1, Val2);
  case OpCode::F32__max:
    return runMaxOp<float>(Val1, Val2);
  case OpCode::F32__copysign:
    return runCopysignOp<float>(Val1, Val2);
  case OpCode::F64__add:
    return runAddOp<double>(Val1, Val2);
  case OpCode::F64__sub:
    return runSubOp<double>(Val1, Val2);
  case OpCode::F64__mul:
    return runMulOp<double>(Val1, Val2);
  case OpCode::F64__div:
    return runDivOp<double>(Val1, Val2);
  case OpCode::F64__min:
    return runMinOp<double>(Val1, Val2);
  case OpCode::F64__max:
    return runMaxOp<double>(Val1, Val2);
  case OpCode::F64__copysign:
    return runCopysignOp<double>(Val1, Val2);
  default:
    __builtin_unreachable();
  }
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
      ++ExecInstrCnt;
      OpCode Code = Instr->getOpCode();
#ifndef ONNC_WASM
      /// Add cost.
      /// Note: if-else case should be processed additionally.
      if (!EnvMgr.addCost(CostTable[static_cast<uint32_t>(Code)])) {
        return ErrCode::Revert;
      }
#endif
      /// Run instructions.
      Status = dispatchInstruction(Code, [this, &Instr](auto &&Arg) {
        if constexpr (std::is_void_v<
                          typename std::decay_t<decltype(Arg)>::type>) {
          /// If the Code not matched, return null pointer.
          return ErrCode::Unimplemented;
        } else {
          /// Make the instruction node according to Code.
          return execute(
              *static_cast<typename std::decay_t<decltype(Arg)>::type *>(
                  Instr));
        }
      });
    }
  }

  /// Check result
  if (TheState == State::Unreachable)
    return ErrCode::Unreachable;
  TheState = State::Inited;
  return Status;
}

ErrCode Worker::enterBlock(unsigned int Arity,
                           AST::BlockControlInstruction *Instr,
                           const AST::InstrVec &Seq) {
  /// Create label for block and push.
  StackMgr.pushLabel(Arity, Instr);

  /// Jump to block body.
  return InstrPdr.pushInstrs(InstrProvider::SeqType::Block, Seq);
}

ErrCode Worker::leaveBlock() {
  StackMgr.popLabel();
  /// Pop label entry and the corresponding instruction sequence.
  InstrPdr.popInstrs();

  return ErrCode::Success;
}

ErrCode Worker::invokeFunction(unsigned int FuncAddr) {
  /// Get Function Instance and module address.
  Instance::FunctionInstance *FuncInst = nullptr;
  if (ErrCode Status = StoreMgr.getFunction(FuncAddr, FuncInst);
      Status != ErrCode::Success)
    return Status;

  /// Get function type
  const Instance::ModuleInstance::FType *FuncType = FuncInst->getFuncType();

  if (FuncInst->isHostFunction()) {
    /// Host function case: Push args and call function.
    HostFunction *HostFunc = nullptr;
    if (ErrCode Status =
            HostFuncMgr.getHostFunction(FuncInst->getHostFuncAddr(), HostFunc);
        Status != ErrCode::Success) {
      return Status;
    }

    Instance::MemoryInstance *MemoryInst = nullptr;
    if (ErrCode Status = getMemInstByIdx(0, MemoryInst);
        Status != ErrCode::Success) {
      return Status;
    }

#ifdef ONNC_WASM
    if (EnvMgr.IsQITCTimer) {
      TimeRecorder.stopRecord(TIMER_TAG_QITC_INFER_SSVM);
      TimeRecorder.startRecord(TIMER_TAG_QITC_INFER_HOST);
    }
#endif
    /// Set start time.
    TimeRecorder.stopRecord(TIMER_TAG_EXECUTION);
    TimeRecorder.startRecord(TIMER_TAG_HOSTFUNC);

    /// Run host function.
    ErrCode Status = HostFunc->run(EnvMgr, StackMgr, *MemoryInst);

    TimeRecorder.stopRecord(TIMER_TAG_HOSTFUNC);
    TimeRecorder.startRecord(TIMER_TAG_EXECUTION);
#ifdef ONNC_WASM
    if (EnvMgr.IsQITCTimer) {
      TimeRecorder.stopRecord(TIMER_TAG_QITC_INFER_HOST);
      TimeRecorder.startRecord(TIMER_TAG_QITC_INFER_SSVM);
    }
#endif

    return Status;
  } else {
    /// Native function case: Push frame with locals and args.
    StackMgr.pushFrame(FuncInst->getModuleAddr(), /// Module address
                       FuncType->Params.size(),   /// Arity
                       FuncType->Returns.size()   /// Coarity
    );

    /// Prepare local variables
    for (auto &Def : FuncInst->getLocals()) {
      for (unsigned int i = 0; i < Def.first; i++) {
        StackMgr.push(AST::ValueFromType(Def.second));
      }
    }

    AST::InstrVec EmptySeq;
    InstrPdr.pushInstrs(InstrProvider::SeqType::FunctionCall, EmptySeq);

    /// Run block of function body
    return enterBlock(FuncType->Returns.size(), nullptr, FuncInst->getInstrs());
  }
}

ErrCode Worker::returnFunction() {
  /// TODO: Validate top of stack is a frame when reach end of function.

  /// Pop the frame entry from the Stack.
  unsigned int LabelPoped;
  if (ErrCode Status = StackMgr.popFrame(LabelPoped);
      Status != ErrCode::Success) {
    return Status;
  }
  for (unsigned int I = 0; I < LabelPoped; ++I) {
    InstrPdr.popInstrs();
  }

  InstrPdr.popInstrs();

  return ErrCode::Success;
}

ErrCode Worker::branchToLabel(unsigned int LabelId) {
  /// Get the L-th label from top of stack and the continuation instruction.
  Label *L = nullptr;
  if (ErrCode Status = StackMgr.getLabelWithCount(L, LabelId);
      Status != ErrCode::Success) {
    return Status;
  }
  AST::BlockControlInstruction *const ContInstr = L->getTarget();

  /// Repeat LabelIndex + 1 times
  for (unsigned int I = 0; I < LabelId + 1; I++) {
    StackMgr.popLabel();
    /// Pop label entry and the corresponding instruction sequence.
    InstrPdr.popInstrs();
  }

  /// Jump to the continuation of Label
  if (ContInstr != nullptr) {
    return runLoopOp(*ContInstr);
  }

  return ErrCode::Success;
}

ErrCode Worker::getTabInstByIdx(unsigned int Idx,
                                Instance::TableInstance *&TabInst) {
  Instance::ModuleInstance *ModInst = nullptr;
  unsigned int TableAddr = 0;
  unsigned int ModuleAddr = StackMgr.getModuleAddr();
  if (ErrCode Status = StoreMgr.getModule(ModuleAddr, ModInst);
      Status != ErrCode::Success) {
    return Status;
  };
  if (ErrCode Status = ModInst->getTableAddr(Idx, TableAddr);
      Status != ErrCode::Success) {
    return Status;
  };
  return StoreMgr.getTable(TableAddr, TabInst);
}

ErrCode Worker::getMemInstByIdx(unsigned int Idx,
                                Instance::MemoryInstance *&MemInst) {
  Instance::ModuleInstance *ModInst = nullptr;
  unsigned int MemoryAddr = 0;
  unsigned int ModuleAddr = StackMgr.getModuleAddr();
  if (ErrCode Status = StoreMgr.getModule(ModuleAddr, ModInst);
      Status != ErrCode::Success) {
    return Status;
  };
  if (ErrCode Status = ModInst->getMemAddr(Idx, MemoryAddr);
      Status != ErrCode::Success) {
    return Status;
  };
  return StoreMgr.getMemory(MemoryAddr, MemInst);
}

ErrCode Worker::getGlobInstByIdx(unsigned int Idx,
                                 Instance::GlobalInstance *&GlobInst) {
  Instance::ModuleInstance *ModInst = nullptr;
  unsigned int GlobalAddr = 0;
  unsigned int ModuleAddr = StackMgr.getModuleAddr();
  if (ErrCode Status = StoreMgr.getModule(ModuleAddr, ModInst);
      Status != ErrCode::Success) {
    return Status;
  };
  if (ErrCode Status = ModInst->getGlobalAddr(Idx, GlobalAddr);
      Status != ErrCode::Success) {
    return Status;
  };
  return StoreMgr.getGlobal(GlobalAddr, GlobInst);
}

} // namespace Executor
} // namespace SSVM
