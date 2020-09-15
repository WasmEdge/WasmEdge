// SPDX-License-Identifier: Apache-2.0
#include "common/ast/instruction.h"
#include "common/statistics.h"
#include "common/value.h"
#include "interpreter/interpreter.h"
#include "support/casting.h"
#include "support/log.h"
#include "support/measure.h"
#include "support/time.h"

namespace SSVM {
namespace Interpreter {

Interpreter *Interpreter::This = nullptr;
uint32_t Interpreter::TrapCodeProxy = 0;
std::jmp_buf *Interpreter::TrapJump = nullptr;

using TimerTag = Support::TimerTag;

void Interpreter::signalHandler(int Signal, siginfo_t *Siginfo, void *) {
  int Status;
  switch (Signal) {
  case SIGSEGV:
    Status = int(ErrCode::MemoryOutOfBounds);
    break;
  case SIGFPE:
    assert(Siginfo->si_code == FPE_INTDIV);
    Status = int(ErrCode::DivideByZero);
    break;
  case SIGILL:
  case SIGABRT:
    Status = TrapCodeProxy;
    break;
  }
  siglongjmp(*This->TrapJump, Status);
}

void Interpreter::callProxy(const uint32_t FuncIndex, const ValVariant *Args,
                            ValVariant *Rets) {
  {
    std::signal(SIGILL, SIG_DFL);
    std::signal(SIGABRT, SIG_DFL);
    std::signal(SIGFPE, SIG_DFL);
    std::signal(SIGSEGV, SIG_DFL);
  }
  auto SavedTrapJump = TrapJump;
  This->call(FuncIndex, Args, Rets);
  TrapJump = SavedTrapJump;
  {
    struct sigaction Action {};
    Action.sa_sigaction = &signalHandler;
    Action.sa_flags = SA_SIGINFO;
    sigaction(SIGILL, &Action, nullptr);
    sigaction(SIGABRT, &Action, nullptr);
    sigaction(SIGFPE, &Action, nullptr);
    sigaction(SIGSEGV, &Action, nullptr);
  }
}

uint32_t Interpreter::memGrowProxy(const uint32_t NewSize) {
  return This->memGrow(NewSize);
}

void Interpreter::call(const uint32_t FuncIndex, const ValVariant *Args,
                       ValVariant *Rets) {
  const auto *ModInst = *CurrentStore->getModule(StackMgr.getModuleAddr());
  const uint32_t FuncAddr = *ModInst->getFuncAddr(FuncIndex);
  const auto *FuncInst = *CurrentStore->getFunction(FuncAddr);
  const auto &FuncType = FuncInst->getFuncType();
  const unsigned ParamsSize = FuncType.Params.size();
  const unsigned ReturnsSize = FuncType.Returns.size();

  for (unsigned I = 0; I < ParamsSize; ++I) {
    StackMgr.push(Args[I]);
  }
  if (auto Res = enterFunction(*CurrentStore, *FuncInst); !Res) {
    siglongjmp(*TrapJump, uint32_t(Res.error()));
    return;
  }
  for (unsigned I = 0; I < ReturnsSize; ++I) {
    Rets[ReturnsSize - 1 - I] = StackMgr.pop();
  }
}

uint32_t Interpreter::memGrow(const uint32_t NewSize) {
  auto &MemInst = *getMemInstByIdx(*CurrentStore, 0);
  const uint32_t CurrPageSize = MemInst.getDataPageSize();
  if (MemInst.growPage(NewSize)) {
    return CurrPageSize;
  } else {
    return -1;
  }
}

Expect<void> Interpreter::runExpression(Runtime::StoreManager &StoreMgr,
                                        const AST::InstrVec &Instrs) {
  /// Set instruction vector to instruction provider.
  InstrPdr.pushInstrs(InstrProvider::SeqType::Expression, Instrs);
  return execute(StoreMgr);
}

Expect<void>
Interpreter::runFunction(Runtime::StoreManager &StoreMgr,
                         const Runtime::Instance::FunctionInstance &Func,
                         Span<const ValVariant> Params) {
  /// Set start time.
  if (Measure) {
    Measure->getTimeRecorder().startRecord(TimerTag::Execution);
  }

  /// Reset and push a dummy frame into stack.
  InstrPdr.reset();
  StackMgr.reset();
  StackMgr.pushDummyFrame();

  /// Push arguments.
  for (auto &Val : Params) {
    StackMgr.push(Val);
  }

  /// Enter and execute function.
  auto Res = enterFunction(StoreMgr, Func);
  if (Res) {
    Res = execute(StoreMgr);
  }

  if (Res) {
    LOG(DEBUG) << " Execution succeeded.";
  } else if (Res.error() == ErrCode::Terminated) {
    LOG(DEBUG) << " Terminated.";
  }

  /// Print time cost.
  if (Measure) {
    uint64_t ExecTime =
        Measure->getTimeRecorder().stopRecord(TimerTag::Execution);
    uint64_t HostFuncTime =
        Measure->getTimeRecorder().getRecord(TimerTag::HostFunc);

    Stat->setWasmExecTime(ExecTime);
    Stat->setHostFuncExecTime(HostFuncTime);
    Stat->setTotalGasCost(Measure->getCostSum());
    Stat->setInstrCount(Measure->getInstrCnt());
    LOG(DEBUG) << std::endl
               << " ====================  Statistics  ===================="
               << std::endl
               << " Total execution time: " << Stat->getTotalExecTime() << " us"
               << std::endl
               << " Wasm instructions execution time: "
               << Stat->getWasmExecTime() << " us" << std::endl
               << " Host functions execution time: "
               << Stat->getHostFuncExecTime() << " us" << std::endl
               << " Executed wasm instructions count: " << Stat->getInstrCount()
               << std::endl
               << " Gas costs: " << Stat->getTotalGasCost() << std::endl
               << " Instructions per second: "
               << static_cast<uint64_t>(Stat->getInstrPerSecond()) << std::endl;
  }

  if (Res || Res.error() == ErrCode::Terminated) {
    return {};
  }
  return Unexpect(Res);
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::ControlInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Unreachable:
    LOG(ERROR) << ErrCode::Unreachable;
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(ErrCode::Unreachable);
  case OpCode::Nop:
    return {};
  case OpCode::Return:
    return runReturnOp();
  default:
    LOG(ERROR) << ErrCode::InstrTypeMismatch;
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(ErrCode::InstrTypeMismatch);
  }
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::BlockControlInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Block:
    return runBlockOp(StoreMgr, Instr);
  case OpCode::Loop:
    return runLoopOp(StoreMgr, Instr);
  default:
    LOG(ERROR) << ErrCode::InstrTypeMismatch;
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(ErrCode::InstrTypeMismatch);
  }
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::IfElseControlInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::If:
    return runIfElseOp(StoreMgr, Instr);
  default:
    LOG(ERROR) << ErrCode::InstrTypeMismatch;
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(ErrCode::InstrTypeMismatch);
  }
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::BrControlInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Br:
    return runBrOp(StoreMgr, Instr);
  case OpCode::Br_if:
    return runBrIfOp(StoreMgr, Instr);
  default:
    LOG(ERROR) << ErrCode::InstrTypeMismatch;
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(ErrCode::InstrTypeMismatch);
  }
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::BrTableControlInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Br_table:
    return runBrTableOp(StoreMgr, Instr);
  default:
    LOG(ERROR) << ErrCode::InstrTypeMismatch;
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(ErrCode::InstrTypeMismatch);
  }
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::CallControlInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Call:
    return runCallOp(StoreMgr, Instr);
  case OpCode::Call_indirect:
    return runCallIndirectOp(StoreMgr, Instr);
  default:
    LOG(ERROR) << ErrCode::InstrTypeMismatch;
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(ErrCode::InstrTypeMismatch);
  }
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::ReferenceInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Ref__null:
    StackMgr.push(genRefType(Instr.getReferenceType()));
    return {};
  case OpCode::Ref__is_null: {
    ValVariant &Val = StackMgr.getTop();
    if (isNullRef(Val)) {
      retrieveValue<uint32_t>(Val) = 1;
    } else {
      retrieveValue<uint32_t>(Val) = 0;
    }
    return {};
  }
  case OpCode::Ref__func: {
    const auto *ModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
    const uint32_t FuncAddr = *ModInst->getFuncAddr(Instr.getTargetIndex());
    StackMgr.push(genRefType(RefType::FuncRef, FuncAddr));
    return {};
  }
  default:
    LOG(ERROR) << ErrCode::InstrTypeMismatch;
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(ErrCode::InstrTypeMismatch);
  }
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::ParametricInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Drop:
    StackMgr.pop();
    return {};
  case OpCode::Select:
  case OpCode::Select_t: {
    /// Pop the i32 value and select values from stack.
    ValVariant CondVal = StackMgr.pop();
    ValVariant Val2 = StackMgr.pop();
    ValVariant Val1 = StackMgr.pop();

    /// Select the value.
    if (retrieveValue<uint32_t>(CondVal) == 0) {
      StackMgr.push(Val2);
    } else {
      StackMgr.push(Val1);
    }
    return {};
  }
  default:
    LOG(ERROR) << ErrCode::InstrTypeMismatch;
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(ErrCode::InstrTypeMismatch);
  }
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::VariableInstruction &Instr) {
  /// Get variable index.
  uint32_t Index = Instr.getVariableIndex();

  /// Check OpCode and run the specific instruction.
  switch (Instr.getOpCode()) {
  case OpCode::Local__get:
    return runLocalGetOp(Index);
  case OpCode::Local__set:
    return runLocalSetOp(Index);
  case OpCode::Local__tee:
    return runLocalTeeOp(Index);
  case OpCode::Global__get:
    return runGlobalGetOp(StoreMgr, Index);
  case OpCode::Global__set:
    return runGlobalSetOp(StoreMgr, Index);
  default:
    LOG(ERROR) << ErrCode::InstrTypeMismatch;
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(ErrCode::InstrTypeMismatch);
  }
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::TableInstruction &Instr) {
  /// Handle elem.drop case.
  if (Instr.getOpCode() == OpCode::Elem__drop) {
    auto *ElemInst = getElemInstByIdx(StoreMgr, Instr.getElemIndex());
    return runElemDropOp(*ElemInst);
  }

  /// Other instructions need table instance.
  auto *TabInst = getTabInstByIdx(StoreMgr, Instr.getTargetIndex());
  switch (Instr.getOpCode()) {
  case OpCode::Table__get:
    return runTableGetOp(*TabInst, Instr);
  case OpCode::Table__set:
    return runTableSetOp(*TabInst, Instr);
  case OpCode::Table__init: {
    auto *ElemInst = getElemInstByIdx(StoreMgr, Instr.getElemIndex());
    return runTableInitOp(*TabInst, *ElemInst, Instr);
  }
  case OpCode::Table__copy: {
    auto *TabSrcInst = getTabInstByIdx(StoreMgr, Instr.getSourceIndex());
    return runTableCopyOp(*TabInst, *TabSrcInst, Instr);
  }
  case OpCode::Table__grow:
    return runTableGrowOp(*TabInst);
  case OpCode::Table__size:
    return runTableSizeOp(*TabInst);
  case OpCode::Table__fill:
    return runTableFillOp(*TabInst, Instr);
  default:
    LOG(ERROR) << ErrCode::InstrTypeMismatch;
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(ErrCode::InstrTypeMismatch);
  }
  return {};
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::MemoryInstruction &Instr) {
  /// Handle data.drop case.
  if (Instr.getOpCode() == OpCode::Data__drop) {
    auto *DataInst = getDataInstByIdx(StoreMgr, Instr.getDataIndex());
    return runDataDropOp(*DataInst);
  }

  /// Other instructions need memory instance.
  auto *MemInst = getMemInstByIdx(StoreMgr, 0);
  switch (Instr.getOpCode()) {
  case OpCode::I32__load:
    return runLoadOp<uint32_t>(*MemInst, Instr);
  case OpCode::I64__load:
    return runLoadOp<uint64_t>(*MemInst, Instr);
  case OpCode::F32__load:
    return runLoadOp<float>(*MemInst, Instr);
  case OpCode::F64__load:
    return runLoadOp<double>(*MemInst, Instr);
  case OpCode::I32__load8_s:
    return runLoadOp<int32_t>(*MemInst, Instr, 8);
  case OpCode::I32__load8_u:
    return runLoadOp<uint32_t>(*MemInst, Instr, 8);
  case OpCode::I32__load16_s:
    return runLoadOp<int32_t>(*MemInst, Instr, 16);
  case OpCode::I32__load16_u:
    return runLoadOp<uint32_t>(*MemInst, Instr, 16);
  case OpCode::I64__load8_s:
    return runLoadOp<int64_t>(*MemInst, Instr, 8);
  case OpCode::I64__load8_u:
    return runLoadOp<uint64_t>(*MemInst, Instr, 8);
  case OpCode::I64__load16_s:
    return runLoadOp<int64_t>(*MemInst, Instr, 16);
  case OpCode::I64__load16_u:
    return runLoadOp<uint64_t>(*MemInst, Instr, 16);
  case OpCode::I64__load32_s:
    return runLoadOp<int64_t>(*MemInst, Instr, 32);
  case OpCode::I64__load32_u:
    return runLoadOp<uint64_t>(*MemInst, Instr, 32);
  case OpCode::I32__store:
    return runStoreOp<uint32_t>(*MemInst, Instr);
  case OpCode::I64__store:
    return runStoreOp<uint64_t>(*MemInst, Instr);
  case OpCode::F32__store:
    return runStoreOp<float>(*MemInst, Instr);
  case OpCode::F64__store:
    return runStoreOp<double>(*MemInst, Instr);
  case OpCode::I32__store8:
    return runStoreOp<uint32_t>(*MemInst, Instr, 8);
  case OpCode::I32__store16:
    return runStoreOp<uint32_t>(*MemInst, Instr, 16);
  case OpCode::I64__store8:
    return runStoreOp<uint64_t>(*MemInst, Instr, 8);
  case OpCode::I64__store16:
    return runStoreOp<uint64_t>(*MemInst, Instr, 16);
  case OpCode::I64__store32:
    return runStoreOp<uint64_t>(*MemInst, Instr, 32);
  case OpCode::Memory__grow:
    return runMemoryGrowOp(*MemInst);
  case OpCode::Memory__size:
    return runMemorySizeOp(*MemInst);
  case OpCode::Memory__init: {
    auto *DataInst = getDataInstByIdx(StoreMgr, Instr.getDataIndex());
    return runMemoryInitOp(*MemInst, *DataInst, Instr);
  }
  case OpCode::Memory__copy:
    return runMemoryCopyOp(*MemInst, Instr);
  case OpCode::Memory__fill:
    return runMemoryFillOp(*MemInst, Instr);
  default:
    LOG(ERROR) << ErrCode::InstrTypeMismatch;
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(ErrCode::InstrTypeMismatch);
  }
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::ConstInstruction &Instr) {
  StackMgr.push(Instr.getConstValue());
  return {};
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::UnaryNumericInstruction &Instr) {
  ValVariant &Val = StackMgr.getTop();
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
  case OpCode::F32__trunc:
    return runTruncOp<float>(Val);
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
  case OpCode::F64__trunc:
    return runTruncOp<double>(Val);
  case OpCode::F64__nearest:
    return runNearestOp<double>(Val);
  case OpCode::F64__sqrt:
    return runSqrtOp<double>(Val);
  case OpCode::I32__wrap_i64:
    return runWrapOp<uint64_t, uint32_t>(Val);
  case OpCode::I32__trunc_f32_s:
    return runTruncateOp<float, int32_t>(Instr, Val);
  case OpCode::I32__trunc_f32_u:
    return runTruncateOp<float, uint32_t>(Instr, Val);
  case OpCode::I32__trunc_f64_s:
    return runTruncateOp<double, int32_t>(Instr, Val);
  case OpCode::I32__trunc_f64_u:
    return runTruncateOp<double, uint32_t>(Instr, Val);
  case OpCode::I64__extend_i32_s:
    return runExtendOp<int32_t, uint64_t>(Val);
  case OpCode::I64__extend_i32_u:
    return runExtendOp<uint32_t, uint64_t>(Val);
  case OpCode::I64__trunc_f32_s:
    return runTruncateOp<float, int64_t>(Instr, Val);
  case OpCode::I64__trunc_f32_u:
    return runTruncateOp<float, uint64_t>(Instr, Val);
  case OpCode::I64__trunc_f64_s:
    return runTruncateOp<double, int64_t>(Instr, Val);
  case OpCode::I64__trunc_f64_u:
    return runTruncateOp<double, uint64_t>(Instr, Val);
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
  case OpCode::I32__extend8_s:
    return runExtendOp<int32_t, uint32_t, 8>(Val);
  case OpCode::I32__extend16_s:
    return runExtendOp<int32_t, uint32_t, 16>(Val);
  case OpCode::I64__extend8_s:
    return runExtendOp<int64_t, uint64_t, 8>(Val);
  case OpCode::I64__extend16_s:
    return runExtendOp<int64_t, uint64_t, 16>(Val);
  case OpCode::I64__extend32_s:
    return runExtendOp<int64_t, uint64_t, 32>(Val);
  case OpCode::I32__trunc_sat_f32_s:
    return runTruncateSatOp<float, int32_t>(Val);
  case OpCode::I32__trunc_sat_f32_u:
    return runTruncateSatOp<float, uint32_t>(Val);
  case OpCode::I32__trunc_sat_f64_s:
    return runTruncateSatOp<double, int32_t>(Val);
  case OpCode::I32__trunc_sat_f64_u:
    return runTruncateSatOp<double, uint32_t>(Val);
  case OpCode::I64__trunc_sat_f32_s:
    return runTruncateSatOp<float, int64_t>(Val);
  case OpCode::I64__trunc_sat_f32_u:
    return runTruncateSatOp<float, uint64_t>(Val);
  case OpCode::I64__trunc_sat_f64_s:
    return runTruncateSatOp<double, int64_t>(Val);
  case OpCode::I64__trunc_sat_f64_u:
    return runTruncateSatOp<double, uint64_t>(Val);
  default:
    LOG(ERROR) << ErrCode::InstrTypeMismatch;
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(ErrCode::InstrTypeMismatch);
  }
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::BinaryNumericInstruction &Instr) {
  ValVariant Val2 = StackMgr.pop();
  ValVariant &Val1 = StackMgr.getTop();

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
    return runDivOp<int32_t>(Instr, Val1, Val2);
  case OpCode::I32__div_u:
    return runDivOp<uint32_t>(Instr, Val1, Val2);
  case OpCode::I32__rem_s:
    return runRemOp<int32_t>(Instr, Val1, Val2);
  case OpCode::I32__rem_u:
    return runRemOp<uint32_t>(Instr, Val1, Val2);
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
    return runDivOp<int64_t>(Instr, Val1, Val2);
  case OpCode::I64__div_u:
    return runDivOp<uint64_t>(Instr, Val1, Val2);
  case OpCode::I64__rem_s:
    return runRemOp<int64_t>(Instr, Val1, Val2);
  case OpCode::I64__rem_u:
    return runRemOp<uint64_t>(Instr, Val1, Val2);
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
    return runDivOp<float>(Instr, Val1, Val2);
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
    return runDivOp<double>(Instr, Val1, Val2);
  case OpCode::F64__min:
    return runMinOp<double>(Val1, Val2);
  case OpCode::F64__max:
    return runMaxOp<double>(Val1, Val2);
  case OpCode::F64__copysign:
    return runCopysignOp<double>(Val1, Val2);
  default:
    LOG(ERROR) << ErrCode::InstrTypeMismatch;
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(ErrCode::InstrTypeMismatch);
  }
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr) {
  /// Run instructions until end.
  while (InstrPdr.getScopeSize() > 0) {
    const AST::Instruction *Instr = InstrPdr.getNextInstr();
    if (Instr == nullptr) {
      /// Pop instruction sequence.
      if (InstrPdr.getTopScopeType() == InstrProvider::SeqType::FunctionCall) {
        if (auto Res = leaveFunction(); !Res) {
          return Unexpect(Res);
        }
      } else if (InstrPdr.getTopScopeType() == InstrProvider::SeqType::Block) {
        if (auto Res = leaveBlock(); !Res) {
          return Unexpect(Res);
        }
      } else {
        InstrPdr.popInstrs();
      }
    } else {
      OpCode Code = Instr->getOpCode();
      if (Measure) {
        Measure->incInstrCnt();
        /// Add cost. Note: if-else case should be processed additionally.
        if (!Measure->addInstrCost(Code)) {
          return Unexpect(ErrCode::CostLimitExceeded);
        }
      }
      /// Run instructions.
      auto Res = AST::dispatchInstruction(
          Code, [this, &Instr, &StoreMgr](auto &&Arg) -> Expect<void> {
            if constexpr (std::is_void_v<
                              typename std::decay_t<decltype(Arg)>::type>) {
              /// If the Code not matched, return null pointer.
              LOG(ERROR) << ErrCode::InstrTypeMismatch;
              LOG(ERROR) << ErrInfo::InfoInstruction(Instr->getOpCode(),
                                                     Instr->getOffset());
              return Unexpect(ErrCode::InstrTypeMismatch);
            } else {
              /// Make the instruction node according to Code.
              return execute(
                  StoreMgr,
                  *static_cast<const typename std::decay_t<decltype(Arg)>::type
                                   *>(Instr));
            }
          });
      if (!Res) {
        return Unexpect(Res);
      }
    }
  }
  /// Run out the expressions.
  return {};
}

Expect<void> Interpreter::enterBlock(const uint32_t Locals,
                                     const uint32_t Arity,
                                     const AST::BlockControlInstruction *Instr,
                                     const AST::InstrVec &Seq) {
  /// Create and push label for block and jump to block body.
  StackMgr.pushLabel(Locals, Arity, Instr);
  InstrPdr.pushInstrs(InstrProvider::SeqType::Block, Seq);
  return {};
}

Expect<void> Interpreter::leaveBlock() {
  /// Pop label entry and the corresponding instruction sequence.
  StackMgr.leaveLabel();
  InstrPdr.popInstrs();
  return {};
}

Expect<void>
Interpreter::enterFunction(Runtime::StoreManager &StoreMgr,
                           const Runtime::Instance::FunctionInstance &Func) {
  /// Get function type
  const auto &FuncType = Func.getFuncType();

  if (Func.isHostFunction()) {
    /// Host function case: Push args and call function.
    auto &HostFunc = Func.getHostFunc();

    /// Get memory instance from current frame.
    /// It'll be nullptr if current frame is dummy frame or no memory instance
    /// in current module.
    auto *MemoryInst = getMemInstByIdx(StoreMgr, 0);

    if (Measure) {
      /// Check host function cost.
      if (!Measure->addCost(HostFunc.getCost())) {
        LOG(ERROR) << ErrCode::CostLimitExceeded;
        return Unexpect(ErrCode::CostLimitExceeded);
      }
      /// Start recording time of running host function.
      Measure->getTimeRecorder().stopRecord(TimerTag::Execution);
      Measure->getTimeRecorder().startRecord(TimerTag::HostFunc);
    }

    /// Run host function.
    const size_t ArgsN = FuncType.Params.size();
    const size_t RetsN = FuncType.Returns.size();
    Span<ValVariant> Args = StackMgr.getTopSpan(ArgsN);
    std::vector<ValVariant> Rets(RetsN);
    auto Ret = HostFunc.run(MemoryInst, std::move(Args), Rets);

    /// Push returns back to stack.
    for (size_t I = 0; I < ArgsN; ++I) {
      StackMgr.pop();
    }
    for (auto &R : Rets) {
      StackMgr.push(std::move(R));
    }

    if (Measure) {
      /// Stop recording time of running host function.
      Measure->getTimeRecorder().stopRecord(TimerTag::HostFunc);
      Measure->getTimeRecorder().startRecord(TimerTag::Execution);
    }

    if (!Ret && Ret.error() == ErrCode::ExecutionFailed) {
      LOG(ERROR) << Ret.error();
    }
    return Ret;
  } else if (auto CompiledFunc = Func.getSymbol()) {
    /// Compiled function case: Push frame with locals and args.
    const size_t ArgsN = FuncType.Params.size();
    const size_t RetsN = FuncType.Returns.size();

    StackMgr.pushFrame(Func.getModuleAddr(), /// Module address
                       ArgsN,                /// Arguments num
                       RetsN                 /// Returns num
    );

    Span<ValVariant> Args = StackMgr.getTopSpan(ArgsN);
    std::vector<ValVariant> Rets(RetsN);

    sigjmp_buf JumpBuffer;
    CurrentStore = &StoreMgr;
    TrapJump = &JumpBuffer;

    const int Status = sigsetjmp(*TrapJump, true);
    if (Status == 0) {
      {
        struct sigaction Action {};
        Action.sa_sigaction = &signalHandler;
        Action.sa_flags = SA_SIGINFO;
        sigaction(SIGILL, &Action, nullptr);
        sigaction(SIGABRT, &Action, nullptr);
        sigaction(SIGFPE, &Action, nullptr);
        sigaction(SIGSEGV, &Action, nullptr);
      }

      CompiledFunc(Args.data(), Rets.data());
    }

    {
      std::signal(SIGILL, SIG_DFL);
      std::signal(SIGABRT, SIG_DFL);
      std::signal(SIGFPE, SIG_DFL);
      std::signal(SIGSEGV, SIG_DFL);
    }

    TrapJump = nullptr;

    if (Status != 0) {
      return Unexpect(ErrCode(Status));
    }

    for (uint32_t I = 0; I < Rets.size(); ++I) {
      StackMgr.push(Rets[I]);
    }

    StackMgr.popFrame();
    return {};
  } else {
    /// Native function case: Push frame with locals and args.
    StackMgr.pushFrame(Func.getModuleAddr(),   /// Module address
                       FuncType.Params.size(), /// Arguments num
                       FuncType.Returns.size() /// Returns num
    );

    /// Push local variables to stack.
    for (auto &Def : Func.getLocals()) {
      for (uint32_t i = 0; i < Def.first; i++) {
        StackMgr.push(ValueFromType(Def.second));
      }
    }

    /// Push function body to instruction provider.
    InstrPdr.pushInstrs(InstrProvider::SeqType::FunctionCall);

    /// Enter function block []->[returns] with label{none}.
    return enterBlock(0, FuncType.Returns.size(), nullptr, Func.getInstrs());
  }
}

Expect<void> Interpreter::leaveFunction() {
  /// Pop the frame entry from the Stack.
  const uint32_t LabelPoped = StackMgr.popFrame();
  for (uint32_t I = 0; I < LabelPoped; ++I) {
    InstrPdr.popInstrs();
  }
  InstrPdr.popInstrs();
  return {};
}

Expect<void> Interpreter::branchToLabel(Runtime::StoreManager &StoreMgr,
                                        const uint32_t Cnt) {
  /// Get the L-th label from top of stack and the continuation instruction.
  const auto *ContInstr = StackMgr.getLabelWithCount(Cnt).Target;

  /// Pop L + 1 labels.
  StackMgr.popLabel(Cnt + 1);

  /// Repeat LabelIndex + 1 times
  for (uint32_t I = 0; I < Cnt + 1; I++) {
    /// Pop the corresponding instruction sequence.
    InstrPdr.popInstrs();
  }

  /// Jump to the continuation of Label
  if (ContInstr != nullptr) {
    return runLoopOp(StoreMgr, *ContInstr);
  }
  return {};
}

Runtime::Instance::TableInstance *
Interpreter::getTabInstByIdx(Runtime::StoreManager &StoreMgr,
                             const uint32_t Idx) {
  /// When top frame is dummy frame, cannot find instance.
  if (StackMgr.isTopDummyFrame()) {
    return nullptr;
  }
  const auto *ModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
  uint32_t TabAddr;
  if (auto Res = ModInst->getTableAddr(Idx)) {
    TabAddr = *Res;
  } else {
    return nullptr;
  }
  if (auto Res = StoreMgr.getTable(TabAddr)) {
    return *Res;
  } else {
    return nullptr;
  }
}

Runtime::Instance::MemoryInstance *
Interpreter::getMemInstByIdx(Runtime::StoreManager &StoreMgr,
                             const uint32_t Idx) {
  /// When top frame is dummy frame, cannot find instance.
  if (StackMgr.isTopDummyFrame()) {
    return nullptr;
  }
  const auto *ModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
  uint32_t MemAddr;
  if (auto Res = ModInst->getMemAddr(Idx)) {
    MemAddr = *Res;
  } else {
    return nullptr;
  }
  if (auto Res = StoreMgr.getMemory(MemAddr)) {
    return *Res;
  } else {
    return nullptr;
  }
}

Runtime::Instance::GlobalInstance *
Interpreter::getGlobInstByIdx(Runtime::StoreManager &StoreMgr,
                              const uint32_t Idx) {
  /// When top frame is dummy frame, cannot find instance.
  if (StackMgr.isTopDummyFrame()) {
    return nullptr;
  }
  const auto *ModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
  uint32_t GlobAddr;
  if (auto Res = ModInst->getGlobalAddr(Idx)) {
    GlobAddr = *Res;
  } else {
    return nullptr;
  }
  if (auto Res = StoreMgr.getGlobal(GlobAddr)) {
    return *Res;
  } else {
    return nullptr;
  }
}

Runtime::Instance::ElementInstance *
Interpreter::getElemInstByIdx(Runtime::StoreManager &StoreMgr,
                              const uint32_t Idx) {
  /// When top frame is dummy frame, cannot find instance.
  if (StackMgr.isTopDummyFrame()) {
    return nullptr;
  }
  const auto *ModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
  uint32_t ElemAddr;
  if (auto Res = ModInst->getElemAddr(Idx)) {
    ElemAddr = *Res;
  } else {
    return nullptr;
  }
  if (auto Res = StoreMgr.getElement(ElemAddr)) {
    return *Res;
  } else {
    return nullptr;
  }
}

Runtime::Instance::DataInstance *
Interpreter::getDataInstByIdx(Runtime::StoreManager &StoreMgr,
                              const uint32_t Idx) {
  /// When top frame is dummy frame, cannot find instance.
  if (StackMgr.isTopDummyFrame()) {
    return nullptr;
  }
  const auto *ModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
  uint32_t DataAddr;
  if (auto Res = ModInst->getDataAddr(Idx)) {
    DataAddr = *Res;
  } else {
    return nullptr;
  }
  if (auto Res = StoreMgr.getData(DataAddr)) {
    return *Res;
  } else {
    return nullptr;
  }
}

} // namespace Interpreter
} // namespace SSVM
