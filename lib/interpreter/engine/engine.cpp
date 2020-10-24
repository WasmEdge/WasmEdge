// SPDX-License-Identifier: Apache-2.0
#include "ast/instruction.h"
#include "common/log.h"
#include "common/statistics.h"
#include "common/value.h"
#include "interpreter/interpreter.h"

namespace SSVM {
namespace Interpreter {

Interpreter *Interpreter::This = nullptr;
uint32_t Interpreter::TrapCode = 0;
std::jmp_buf *Interpreter::TrapJump = nullptr;

struct Interpreter::SignalEnabler {
  SignalEnabler() noexcept { Interpreter::signalEnable(); }
  ~SignalEnabler() noexcept { Interpreter::signalDisable(); }
};

struct Interpreter::SignalDisabler {
  SignalDisabler() noexcept { Interpreter::signalDisable(); }
  ~SignalDisabler() noexcept { Interpreter::signalEnable(); }
};

template <typename RetT, typename... ArgsT>
struct Interpreter::ProxyHelper<Expect<RetT> (Interpreter::*)(
    Runtime::StoreManager &, ArgsT...) noexcept> {
  template <Expect<RetT> (Interpreter::*Func)(Runtime::StoreManager &,
                                              ArgsT...) noexcept>
  static RetT proxy(ArgsT... Args) noexcept {
    Interpreter::SignalDisabler Disabler;
    if (auto Res = (This->*Func)(*This->CurrentStore, Args...);
        unlikely(!Res)) {
      siglongjmp(*TrapJump, uint8_t(Res.error()));
    } else if constexpr (!std::is_void_v<RetT>) {
      return *Res;
    }
  }
};

#if defined(__clang_major__) && __clang_major__ >= 10
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-designator"
#endif

AST::Module::IntrinsicsTable Interpreter::IntrinsicsTable = {
#define ENTRY(NAME, FUNC)                                                      \
  [uint8_t(AST::Module::Intrinsics::NAME)] = reinterpret_cast<void *>(         \
      &Interpreter::ProxyHelper<decltype(                                      \
          &Interpreter::FUNC)>::proxy<&Interpreter::FUNC>)
    ENTRY(kCall, call),           ENTRY(kCallIndirect, callIndirect),
    ENTRY(kMemCopy, memCopy),     ENTRY(kMemFill, memFill),
    ENTRY(kMemGrow, memGrow),     ENTRY(kMemSize, memSize),
    ENTRY(kMemInit, memInit),     ENTRY(kDataDrop, dataDrop),
    ENTRY(kTableGet, tableGet),   ENTRY(kTableSet, tableSet),
    ENTRY(kTableCopy, tableCopy), ENTRY(kTableFill, tableFill),
    ENTRY(kTableGrow, tableGrow), ENTRY(kTableSize, tableSize),
    ENTRY(kTableInit, tableInit), ENTRY(kElemDrop, elemDrop),
    ENTRY(kRefFunc, refFunc),
#undef ENTRY
};

#if defined(__clang_major__) && __clang_major__ >= 10
#pragma clang diagnostic pop
#endif

void Interpreter::signalHandler(int Signal, siginfo_t *Siginfo,
                                void *) noexcept {
  int Status;
  switch (Signal) {
  case SIGSEGV:
    Status = uint8_t(ErrCode::MemoryOutOfBounds);
    break;
  case SIGFPE:
    assert(Siginfo->si_code == FPE_INTDIV);
    Status = uint8_t(ErrCode::DivideByZero);
    break;
  case SIGILL:
  case SIGABRT:
    Status = TrapCode;
    break;
  }
  siglongjmp(*This->TrapJump, Status);
}

void Interpreter::signalEnable() noexcept {
  struct sigaction Action {};
  Action.sa_sigaction = &signalHandler;
  Action.sa_flags = SA_SIGINFO;
  sigaction(SIGILL, &Action, nullptr);
  sigaction(SIGABRT, &Action, nullptr);
  sigaction(SIGFPE, &Action, nullptr);
  sigaction(SIGSEGV, &Action, nullptr);
}

void Interpreter::signalDisable() noexcept {
  std::signal(SIGILL, SIG_DFL);
  std::signal(SIGABRT, SIG_DFL);
  std::signal(SIGFPE, SIG_DFL);
  std::signal(SIGSEGV, SIG_DFL);
}

Expect<void> Interpreter::call(Runtime::StoreManager &StoreMgr,
                               const uint32_t FuncIndex, const ValVariant *Args,
                               ValVariant *Rets) noexcept {
  const auto *ModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
  const uint32_t FuncAddr = *ModInst->getFuncAddr(FuncIndex);
  const auto *FuncInst = *StoreMgr.getFunction(FuncAddr);
  const auto &FuncType = FuncInst->getFuncType();
  const unsigned ParamsSize = FuncType.Params.size();
  const unsigned ReturnsSize = FuncType.Returns.size();

  for (unsigned I = 0; I < ParamsSize; ++I) {
    StackMgr.push(Args[I]);
  }

  if (auto Res = enterFunction(StoreMgr, *FuncInst); unlikely(!Res)) {
    return Unexpect(Res);
  }
  if (auto Res = execute(StoreMgr); unlikely(!Res)) {
    return Unexpect(Res);
  }

  for (unsigned I = 0; I < ReturnsSize; ++I) {
    Rets[ReturnsSize - 1 - I] = StackMgr.pop();
  }

  return {};
}

Expect<void> Interpreter::callIndirect(Runtime::StoreManager &StoreMgr,
                                       const uint32_t TableIndex,
                                       const uint32_t FuncTypeIndex,
                                       const uint32_t FuncIndex,
                                       const ValVariant *Args,
                                       ValVariant *Rets) noexcept {
  const auto *TabInst = getTabInstByIdx(StoreMgr, TableIndex);

  if (unlikely(FuncIndex >= TabInst->getSize())) {
    return Unexpect(ErrCode::UndefinedElement);
  }

  ValVariant Ref = *TabInst->getRefAddr(FuncIndex);
  if (unlikely(isNullRef(Ref))) {
    return Unexpect(ErrCode::UninitializedElement);
  }
  const auto FuncAddr = retrieveFuncIdx(Ref);

  const auto *ModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
  const auto *TargetFuncType = *ModInst->getFuncType(FuncTypeIndex);
  const auto *FuncInst = *StoreMgr.getFunction(FuncAddr);
  const auto &FuncType = FuncInst->getFuncType();
  if (unlikely(*TargetFuncType != FuncType)) {
    return Unexpect(ErrCode::IndirectCallTypeMismatch);
  }

  const unsigned ParamsSize = FuncType.Params.size();
  const unsigned ReturnsSize = FuncType.Returns.size();

  for (unsigned I = 0; I < ParamsSize; ++I) {
    StackMgr.push(Args[I]);
  }

  if (auto Res = enterFunction(StoreMgr, *FuncInst); unlikely(!Res)) {
    return Unexpect(Res);
  }
  if (auto Res = execute(StoreMgr); unlikely(!Res)) {
    return Unexpect(Res);
  }

  for (unsigned I = 0; I < ReturnsSize; ++I) {
    Rets[ReturnsSize - 1 - I] = StackMgr.pop();
  }

  return {};
}

Expect<uint32_t> Interpreter::memGrow(Runtime::StoreManager &StoreMgr,
                                      const uint32_t NewSize) noexcept {
  auto &MemInst = *getMemInstByIdx(StoreMgr, 0);
  const uint32_t CurrPageSize = MemInst.getDataPageSize();
  if (MemInst.growPage(NewSize)) {
    return CurrPageSize;
  } else {
    return -1;
  }
}

Expect<uint32_t>
Interpreter::memSize(Runtime::StoreManager &StoreMgr) noexcept {
  auto &MemInst = *getMemInstByIdx(StoreMgr, 0);
  return MemInst.getDataPageSize();
}

Expect<void> Interpreter::memCopy(Runtime::StoreManager &StoreMgr,
                                  const uint32_t Dst, const uint32_t Src,
                                  const uint32_t Len) noexcept {
  auto &MemInst = *getMemInstByIdx(StoreMgr, 0);

  if (auto Data = MemInst.getBytes(Src, Len); unlikely(!Data)) {
    return Unexpect(Data);
  } else {
    if (auto Res = MemInst.setBytes(*Data, Dst, 0, Len); unlikely(!Res)) {
      return Unexpect(Res);
    }
  }

  return {};
}

Expect<void> Interpreter::memFill(Runtime::StoreManager &StoreMgr,
                                  const uint32_t Off, const uint8_t Val,
                                  const uint32_t Len) noexcept {
  auto &MemInst = *getMemInstByIdx(StoreMgr, 0);
  if (auto Res = MemInst.fillBytes(Val, Off, Len); unlikely(!Res)) {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Interpreter::memInit(Runtime::StoreManager &StoreMgr,
                                  const uint32_t DataIdx, const uint32_t Dst,
                                  const uint32_t Src,
                                  const uint32_t Len) noexcept {
  auto &MemInst = *getMemInstByIdx(StoreMgr, 0);
  auto &DataInst = *getDataInstByIdx(StoreMgr, DataIdx);

  if (auto Res = MemInst.setBytes(DataInst.getData(), Dst, Src, Len);
      unlikely(!Res)) {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Interpreter::dataDrop(Runtime::StoreManager &StoreMgr,
                                   const uint32_t DataIdx) noexcept {
  auto &DataInst = *getDataInstByIdx(StoreMgr, DataIdx);
  DataInst.clear();

  return {};
}

Expect<ValVariant> Interpreter::tableGet(Runtime::StoreManager &StoreMgr,
                                         const uint32_t TableIndex,
                                         const uint32_t Idx) noexcept {
  auto &TabInst = *getTabInstByIdx(StoreMgr, TableIndex);
  if (auto Res = TabInst.getRefAddr(Idx); unlikely(!Res)) {
    return Unexpect(Res);
  } else {
    return *Res;
  }

  return {};
}

Expect<void> Interpreter::tableSet(Runtime::StoreManager &StoreMgr,
                                   const uint32_t TableIndex,
                                   const uint32_t Idx,
                                   const ValVariant Ref) noexcept {
  auto &TabInst = *getTabInstByIdx(StoreMgr, TableIndex);
  if (auto Res = TabInst.setRefAddr(Idx, Ref); unlikely(!Res)) {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Interpreter::tableCopy(Runtime::StoreManager &StoreMgr,
                                    const uint32_t TableIndexSrc,
                                    const uint32_t TableIndexDst,
                                    const uint32_t Dst, const uint32_t Src,
                                    const uint32_t Len) noexcept {
  auto &TabInstSrc = *getTabInstByIdx(StoreMgr, TableIndexSrc);
  auto &TabInstDst = *getTabInstByIdx(StoreMgr, TableIndexDst);
  if (auto Refs = TabInstSrc.getRefs(Src, Len); unlikely(!Refs)) {
    return Unexpect(Refs);
  } else {
    if (auto Res = TabInstDst.setRefs(*Refs, Dst, 0, Len); unlikely(!Res)) {
      return Unexpect(Res);
    }
  }

  return {};
}

Expect<uint32_t> Interpreter::tableGrow(Runtime::StoreManager &StoreMgr,
                                        const uint32_t TableIndex,
                                        const ValVariant Val,
                                        const uint32_t NewSize) noexcept {
  auto &TabInst = *getTabInstByIdx(StoreMgr, TableIndex);
  const uint32_t CurrTableSize = TabInst.getSize();
  if (likely(TabInst.growTable(NewSize, Val))) {
    return CurrTableSize;
  } else {
    return -1;
  }
}

Expect<uint32_t> Interpreter::tableSize(Runtime::StoreManager &StoreMgr,
                                        const uint32_t TableIndex) noexcept {
  auto &TabInst = *getTabInstByIdx(StoreMgr, TableIndex);
  return TabInst.getSize();
}

Expect<void> Interpreter::tableFill(Runtime::StoreManager &StoreMgr,
                                    const uint32_t TableIndex,
                                    const uint32_t Off, const ValVariant Ref,
                                    const uint32_t Len) noexcept {
  auto &TabInst = *getTabInstByIdx(StoreMgr, TableIndex);
  if (auto Res = TabInst.fillRefs(Ref, Off, Len); unlikely(!Res)) {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Interpreter::tableInit(Runtime::StoreManager &StoreMgr,
                                    const uint32_t TableIndex,
                                    const uint32_t ElementIndex,
                                    const uint32_t Dst, const uint32_t Src,
                                    const uint32_t Len) noexcept {
  auto &TabInst = *getTabInstByIdx(StoreMgr, TableIndex);
  auto &ElemInst = *getElemInstByIdx(StoreMgr, ElementIndex);
  if (auto Res = TabInst.setRefs(ElemInst.getRefs(), Dst, Src, Len);
      unlikely(!Res)) {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Interpreter::elemDrop(Runtime::StoreManager &StoreMgr,
                                   const uint32_t ElementIndex) noexcept {
  auto &ElemInst = *getElemInstByIdx(StoreMgr, ElementIndex);
  ElemInst.clear();

  return {};
}

Expect<ValVariant> Interpreter::refFunc(Runtime::StoreManager &StoreMgr,
                                        const uint32_t FuncIndex) noexcept {
  const auto *ModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
  const uint32_t FuncAddr = *ModInst->getFuncAddr(FuncIndex);
  return genFuncRef(FuncAddr);
}

Expect<void> Interpreter::runExpression(Runtime::StoreManager &StoreMgr,
                                        const AST::InstrVec &Instrs) {
  enterBlock(0, 0, nullptr, Instrs);
  return execute(StoreMgr);
}

Expect<void>
Interpreter::runFunction(Runtime::StoreManager &StoreMgr,
                         const Runtime::Instance::FunctionInstance &Func,
                         Span<const ValVariant> Params) {
  /// Set start time.
  if (Stat) {
    Stat->startRecordWasm();
  }

  /// Reset and push a dummy frame into stack.
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
  if (Stat) {
    Stat->stopRecordWasm();

    auto Nano = [](auto &&Duration) {
      return std::chrono::nanoseconds(Duration).count();
    };
    LOG(DEBUG) << "\n"
                  " ====================  Statistics  ====================\n"
                  " Total execution time: "
               << Nano(Stat->getTotalExecTime())
               << " ns\n"
                  " Wasm instructions execution time: "
               << Nano(Stat->getWasmExecTime())
               << " ns\n"
                  " Host functions execution time: "
               << Nano(Stat->getHostFuncExecTime())
               << " ns\n"
                  " Executed wasm instructions count: "
               << Stat->getInstrCount() << "\n"
               << " Gas costs: " << Stat->getTotalCost()
               << "\n"
                  " Instructions per second: "
               << uint64_t(Stat->getInstrPerSecond());
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
  case OpCode::End:
    StackMgr.endExpression();
    return {};
  default:
    __builtin_unreachable();
    return Unexpect(ErrCode::InvalidOpCode);
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
    __builtin_unreachable();
    return Unexpect(ErrCode::InvalidOpCode);
  }
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::IfElseControlInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::If:
    return runIfElseOp(StoreMgr, Instr);
  default:
    __builtin_unreachable();
    return Unexpect(ErrCode::InvalidOpCode);
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
    __builtin_unreachable();
    return Unexpect(ErrCode::InvalidOpCode);
  }
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::BrTableControlInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Br_table:
    return runBrTableOp(StoreMgr, Instr);
  default:
    __builtin_unreachable();
    return Unexpect(ErrCode::InvalidOpCode);
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
    __builtin_unreachable();
    return Unexpect(ErrCode::InvalidOpCode);
  }
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::ReferenceInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Ref__null:
    StackMgr.push(genNullRef(Instr.getReferenceType()));
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
    StackMgr.push(genFuncRef(FuncAddr));
    return {};
  }
  default:
    __builtin_unreachable();
    return Unexpect(ErrCode::InvalidOpCode);
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
    __builtin_unreachable();
    return Unexpect(ErrCode::InvalidOpCode);
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
    __builtin_unreachable();
    return Unexpect(ErrCode::InvalidOpCode);
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
    __builtin_unreachable();
    return Unexpect(ErrCode::InvalidOpCode);
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
    __builtin_unreachable();
    return Unexpect(ErrCode::InvalidOpCode);
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
    __builtin_unreachable();
    return Unexpect(ErrCode::InvalidOpCode);
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
    __builtin_unreachable();
    return Unexpect(ErrCode::InvalidOpCode);
  }
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr) {
  /// Run instructions until end.
  const AST::Instruction *Instr = StackMgr.getNextInstr();
  while (Instr != nullptr) {
    OpCode Code = Instr->getOpCode();
    if (Stat) {
      Stat->incInstrCount();
      /// Add cost. Note: if-else case should be processed additionally.
      if (unlikely(!Stat->addInstrCost(Code))) {
        return Unexpect(ErrCode::CostLimitExceeded);
      }
    }
    /// Run instructions.
    auto Res = AST::dispatchInstruction(
        Code, [this, &Instr, &StoreMgr](auto &&Arg) -> Expect<void> {
          using InstrT = typename std::decay_t<decltype(Arg)>::type;
          if constexpr (std::is_void_v<InstrT>) {
            /// OpCode was checked in validator
            __builtin_unreachable();
            return Unexpect(ErrCode::InvalidOpCode);
          } else {
            /// Make the instruction node according to Code.
            return execute(StoreMgr, *static_cast<const InstrT *>(Instr));
          }
        });
    if (!Res) {
      return Unexpect(Res);
    }
    Instr = StackMgr.getNextInstr();
  }
  return {};
}

Expect<void> Interpreter::enterBlock(const uint32_t Locals,
                                     const uint32_t Arity,
                                     const AST::BlockControlInstruction *Instr,
                                     const AST::InstrVec &Seq) {
  /// Create and push label for block and jump to block body.
  StackMgr.pushLabel(Locals, Arity, Seq, Instr);
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

    if (Stat) {
      /// Check host function cost.
      if (unlikely(!Stat->addCost(HostFunc.getCost()))) {
        LOG(ERROR) << ErrCode::CostLimitExceeded;
        return Unexpect(ErrCode::CostLimitExceeded);
      }
      /// Start recording time of running host function.
      Stat->stopRecordWasm();
      Stat->startRecordHost();
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

    if (Stat) {
      /// Stop recording time of running host function.
      Stat->stopRecordHost();
      Stat->startRecordWasm();
    }

    if (!Ret && Ret.error() == ErrCode::ExecutionFailed) {
      LOG(ERROR) << Ret.error();
    }
    return Ret;
  } else if (auto CompiledFunc = Func.getSymbol()) {
    auto Wrapper = Func.getFuncType().getSymbol();
    /// Compiled function case: Push frame with locals and args.
    const size_t ArgsN = FuncType.Params.size();
    const size_t RetsN = FuncType.Returns.size();

    StackMgr.pushFrame(Func.getModuleAddr(), /// Module address
                       ArgsN,                /// No Arguments in stack
                       RetsN                 /// Returns num
    );

    Span<ValVariant> Args = StackMgr.getTopSpan(ArgsN);
    std::vector<ValVariant> Rets(RetsN);

    sigjmp_buf JumpBuffer;
    CurrentStore = &StoreMgr;
    auto OldTrapJump = std::exchange(TrapJump, &JumpBuffer);

    const int Status = sigsetjmp(*TrapJump, true);
    if (Status == 0) {
      SignalEnabler Enabler;
      Wrapper(CompiledFunc.get(), Args.data(), Rets.data());
    }

    TrapJump = std::move(OldTrapJump);

    if (Status != 0) {
      return Unexpect(ErrCode(uint8_t(Status)));
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

    /// Enter function block []->[returns] with label{none}.
    return enterBlock(0, FuncType.Returns.size(), nullptr, Func.getInstrs());
  }
}

Expect<void> Interpreter::branchToLabel(Runtime::StoreManager &StoreMgr,
                                        const uint32_t Cnt) {
  /// Get the L-th label from top of stack and the continuation instruction.
  const auto *ContInstr = StackMgr.getLabelWithCount(Cnt).Target;

  /// Pop L + 1 labels.
  StackMgr.popLabel(Cnt + 1);

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
