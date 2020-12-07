// SPDX-License-Identifier: Apache-2.0
#include "ast/instruction.h"
#include "common/log.h"
#include "common/statistics.h"
#include "common/value.h"
#include "interpreter/interpreter.h"

namespace SSVM {
namespace Interpreter {

Interpreter *Interpreter::This = nullptr;
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

/// Intrinsics table
extern "C" {
__attribute__((visibility("default")))
AST::Module::IntrinsicsTable intrinsics = {
#define ENTRY(NAME, FUNC)                                                      \
  [uint8_t(AST::Module::Intrinsics::NAME)] = reinterpret_cast<void *>(         \
      &Interpreter::ProxyHelper<decltype(                                      \
          &Interpreter::FUNC)>::proxy<&Interpreter::FUNC>)
    ENTRY(kTrap, trap),
    ENTRY(kCall, call),
    ENTRY(kCallIndirect, callIndirect),
    ENTRY(kMemCopy, memCopy),
    ENTRY(kMemFill, memFill),
    ENTRY(kMemGrow, memGrow),
    ENTRY(kMemSize, memSize),
    ENTRY(kMemInit, memInit),
    ENTRY(kDataDrop, dataDrop),
    ENTRY(kTableGet, tableGet),
    ENTRY(kTableSet, tableSet),
    ENTRY(kTableCopy, tableCopy),
    ENTRY(kTableFill, tableFill),
    ENTRY(kTableGrow, tableGrow),
    ENTRY(kTableSize, tableSize),
    ENTRY(kTableInit, tableInit),
    ENTRY(kElemDrop, elemDrop),
    ENTRY(kRefFunc, refFunc),
#undef ENTRY
};
}

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
  default:
    __builtin_unreachable();
  }
  siglongjmp(*This->TrapJump, Status);
}

void Interpreter::signalEnable() noexcept {
  struct sigaction Action {};
  Action.sa_sigaction = &signalHandler;
  Action.sa_flags = SA_SIGINFO;
  sigaction(SIGFPE, &Action, nullptr);
  sigaction(SIGSEGV, &Action, nullptr);
}

void Interpreter::signalDisable() noexcept {
  std::signal(SIGFPE, SIG_DFL);
  std::signal(SIGSEGV, SIG_DFL);
}

Expect<void> Interpreter::trap(Runtime::StoreManager &StoreMgr,
                               const uint8_t Code) noexcept {
  return Unexpect(ErrCode(Code));
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

Expect<RefVariant> Interpreter::tableGet(Runtime::StoreManager &StoreMgr,
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
                                   const RefVariant Ref) noexcept {
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
                                        const RefVariant Val,
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
                                    const uint32_t Off, const RefVariant Ref,
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

Expect<RefVariant> Interpreter::refFunc(Runtime::StoreManager &StoreMgr,
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

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::SIMDMemoryInstruction &Instr) {
  auto *MemInst = getMemInstByIdx(StoreMgr, 0);
  switch (Instr.getOpCode()) {
  case OpCode::V128__load:
    return runLoadOp(*MemInst, Instr);
  case OpCode::I16x8__load8x8_s:
    return runLoadExpandOp<int8_t, int16_t>(*MemInst, Instr);
  case OpCode::I16x8__load8x8_u:
    return runLoadExpandOp<uint8_t, uint16_t>(*MemInst, Instr);
  case OpCode::I32x4__load16x4_s:
    return runLoadExpandOp<int16_t, int32_t>(*MemInst, Instr);
  case OpCode::I32x4__load16x4_u:
    return runLoadExpandOp<uint16_t, uint32_t>(*MemInst, Instr);
  case OpCode::I64x2__load32x2_s:
    return runLoadExpandOp<int32_t, int64_t>(*MemInst, Instr);
  case OpCode::I64x2__load32x2_u:
    return runLoadExpandOp<uint32_t, uint64_t>(*MemInst, Instr);
  case OpCode::I8x16__load_splat:
    return runLoadSplatOp<uint8_t>(*MemInst, Instr);
  case OpCode::I16x8__load_splat:
    return runLoadSplatOp<uint16_t>(*MemInst, Instr);
  case OpCode::I32x4__load_splat:
    return runLoadSplatOp<uint32_t>(*MemInst, Instr);
  case OpCode::I64x2__load_splat:
    return runLoadSplatOp<uint64_t>(*MemInst, Instr);
  case OpCode::V128__load32_zero:
    return runLoadOp(*MemInst, Instr, 32);
  case OpCode::V128__load64_zero:
    return runLoadOp(*MemInst, Instr, 64);
  case OpCode::V128__store:
    return runStoreOp(*MemInst, Instr);

  default:
    __builtin_unreachable();
    return Unexpect(ErrCode::InvalidOpCode);
  }
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::SIMDConstInstruction &Instr) {
  StackMgr.push(Instr.getConstValue());
  return {};
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::SIMDShuffleInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::I8x16__shuffle: {
    ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    std::array<uint8_t, 32> Data;
    std::array<uint8_t, 16> Result;
    std::memcpy(&Data[0], &Val1, 16);
    std::memcpy(&Data[16], &Val2, 16);
    const auto V3 = Instr.getShuffleValue();
    for (size_t I = 0; I < 16; ++I) {
      const uint8_t Index = static_cast<uint8_t>(V3 >> (I * 8));
      Result[I] = Data[Index];
    }
    std::memcpy(&Val1, &Result[0], 16);
    break;
  }
  default:
    __builtin_unreachable();
  }
  return {};
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::SIMDLaneInstruction &Instr) {
  const auto Index = Instr.getLaneIndex();
  switch (Instr.getOpCode()) {
  case OpCode::I8x16__extract_lane_s:
    return runExtractLaneOp<int8_t, int32_t>(StackMgr.getTop(), Index);
  case OpCode::I8x16__extract_lane_u:
    return runExtractLaneOp<uint8_t, uint32_t>(StackMgr.getTop(), Index);
  case OpCode::I16x8__extract_lane_s:
    return runExtractLaneOp<int16_t, int32_t>(StackMgr.getTop(), Index);
  case OpCode::I16x8__extract_lane_u:
    return runExtractLaneOp<uint16_t, uint32_t>(StackMgr.getTop(), Index);
  case OpCode::I32x4__extract_lane:
    return runExtractLaneOp<uint32_t>(StackMgr.getTop(), Index);
  case OpCode::I64x2__extract_lane:
    return runExtractLaneOp<uint64_t>(StackMgr.getTop(), Index);
  case OpCode::F32x4__extract_lane:
    return runExtractLaneOp<float>(StackMgr.getTop(), Index);
  case OpCode::F64x2__extract_lane:
    return runExtractLaneOp<double>(StackMgr.getTop(), Index);
  default:
    break;
  }

  ValVariant Val2 = StackMgr.pop();
  ValVariant &Val1 = StackMgr.getTop();
  switch (Instr.getOpCode()) {
  case OpCode::I8x16__replace_lane:
    return runReplaceLaneOp<uint32_t, uint8_t>(Val1, Val2, Index);
  case OpCode::I16x8__replace_lane:
    return runReplaceLaneOp<uint32_t, uint16_t>(Val1, Val2, Index);
  case OpCode::I32x4__replace_lane:
    return runReplaceLaneOp<uint32_t>(Val1, Val2, Index);
  case OpCode::I64x2__replace_lane:
    return runReplaceLaneOp<uint64_t>(Val1, Val2, Index);
  case OpCode::F32x4__replace_lane:
    return runReplaceLaneOp<float>(Val1, Val2, Index);
  case OpCode::F64x2__replace_lane:
    return runReplaceLaneOp<double>(Val1, Val2, Index);
  default:
    __builtin_unreachable();
  }
}

Expect<void> Interpreter::execute(Runtime::StoreManager &StoreMgr,
                                  const AST::SIMDNumericInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::I8x16__swizzle: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    const uint8x16_t &Index = retrieveValue<uint8x16_t>(Val2);
    uint8x16_t &Vector = retrieveValue<uint8x16_t>(Val1);
    const uint8x16_t Limit = {16, 16, 16, 16, 16, 16, 16, 16,
                              16, 16, 16, 16, 16, 16, 16, 16};
    const uint8x16_t Zero = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint8x16_t Exceed = (Index >= Limit);
#ifdef __clang__
    uint8x16_t Result = {
        Vector[Index[0]],  Vector[Index[1]],  Vector[Index[2]],
        Vector[Index[3]],  Vector[Index[4]],  Vector[Index[5]],
        Vector[Index[6]],  Vector[Index[7]],  Vector[Index[8]],
        Vector[Index[9]],  Vector[Index[10]], Vector[Index[11]],
        Vector[Index[12]], Vector[Index[13]], Vector[Index[14]],
        Vector[Index[15]]};
#else
    uint8x16_t Result = __builtin_shuffle(Vector, Index);
#endif
    Vector = Exceed ? Zero : Result;
    return {};
  }
  case OpCode::I8x16__splat: {
    ValVariant &Val = StackMgr.getTop();
    return runSplatOp<uint32_t, uint8_t>(Val);
  }
  case OpCode::I16x8__splat: {
    ValVariant &Val = StackMgr.getTop();
    return runSplatOp<uint32_t, uint16_t>(Val);
  }
  case OpCode::I32x4__splat: {
    ValVariant &Val = StackMgr.getTop();
    return runSplatOp<uint32_t>(Val);
  }
  case OpCode::I64x2__splat: {
    ValVariant &Val = StackMgr.getTop();
    return runSplatOp<uint64_t>(Val);
  }
  case OpCode::F32x4__splat: {
    ValVariant &Val = StackMgr.getTop();
    return runSplatOp<float>(Val);
  }
  case OpCode::F64x2__splat: {
    ValVariant &Val = StackMgr.getTop();
    return runSplatOp<double>(Val);
  }

  case OpCode::I8x16__eq: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorEqOp<uint8_t>(Val1, Val2);
  }
  case OpCode::I8x16__ne: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorNeOp<uint8_t>(Val1, Val2);
  }
  case OpCode::I8x16__lt_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorLtOp<int8_t>(Val1, Val2);
  }
  case OpCode::I8x16__lt_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorLtOp<uint8_t>(Val1, Val2);
  }
  case OpCode::I8x16__gt_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorGtOp<int8_t>(Val1, Val2);
  }
  case OpCode::I8x16__gt_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorGtOp<uint8_t>(Val1, Val2);
  }
  case OpCode::I8x16__le_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorLeOp<int8_t>(Val1, Val2);
  }
  case OpCode::I8x16__le_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorLeOp<uint8_t>(Val1, Val2);
  }
  case OpCode::I8x16__ge_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorGeOp<int8_t>(Val1, Val2);
  }
  case OpCode::I8x16__ge_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorGeOp<uint8_t>(Val1, Val2);
  }

  case OpCode::I16x8__eq: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorEqOp<uint16_t>(Val1, Val2);
  }
  case OpCode::I16x8__ne: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorNeOp<uint16_t>(Val1, Val2);
  }
  case OpCode::I16x8__lt_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorLtOp<int16_t>(Val1, Val2);
  }
  case OpCode::I16x8__lt_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorLtOp<uint16_t>(Val1, Val2);
  }
  case OpCode::I16x8__gt_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorGtOp<int16_t>(Val1, Val2);
  }
  case OpCode::I16x8__gt_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorGtOp<uint16_t>(Val1, Val2);
  }
  case OpCode::I16x8__le_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorLeOp<int16_t>(Val1, Val2);
  }
  case OpCode::I16x8__le_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorLeOp<uint16_t>(Val1, Val2);
  }
  case OpCode::I16x8__ge_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorGeOp<int16_t>(Val1, Val2);
  }
  case OpCode::I16x8__ge_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorGeOp<uint16_t>(Val1, Val2);
  }

  case OpCode::I32x4__eq: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorEqOp<uint32_t>(Val1, Val2);
  }
  case OpCode::I32x4__ne: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorNeOp<uint32_t>(Val1, Val2);
  }
  case OpCode::I32x4__lt_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorLtOp<int32_t>(Val1, Val2);
  }
  case OpCode::I32x4__lt_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorLtOp<uint32_t>(Val1, Val2);
  }
  case OpCode::I32x4__gt_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorGtOp<int32_t>(Val1, Val2);
  }
  case OpCode::I32x4__gt_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorGtOp<uint32_t>(Val1, Val2);
  }
  case OpCode::I32x4__le_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorLeOp<int32_t>(Val1, Val2);
  }
  case OpCode::I32x4__le_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorLeOp<uint32_t>(Val1, Val2);
  }
  case OpCode::I32x4__ge_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorGeOp<int32_t>(Val1, Val2);
  }
  case OpCode::I32x4__ge_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorGeOp<uint32_t>(Val1, Val2);
  }

  case OpCode::F32x4__eq: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorEqOp<float>(Val1, Val2);
  }
  case OpCode::F32x4__ne: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorNeOp<float>(Val1, Val2);
  }
  case OpCode::F32x4__lt: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorLtOp<float>(Val1, Val2);
  }
  case OpCode::F32x4__gt: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorGtOp<float>(Val1, Val2);
  }
  case OpCode::F32x4__le: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorLeOp<float>(Val1, Val2);
  }
  case OpCode::F32x4__ge: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorGeOp<float>(Val1, Val2);
  }

  case OpCode::F64x2__eq: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorEqOp<double>(Val1, Val2);
  }
  case OpCode::F64x2__ne: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorNeOp<double>(Val1, Val2);
  }
  case OpCode::F64x2__lt: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorLtOp<double>(Val1, Val2);
  }
  case OpCode::F64x2__gt: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorGtOp<double>(Val1, Val2);
  }
  case OpCode::F64x2__le: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorLeOp<double>(Val1, Val2);
  }
  case OpCode::F64x2__ge: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorGeOp<double>(Val1, Val2);
  }

  case OpCode::V128__not: {
    ValVariant &Val = StackMgr.getTop();
    Val = ~retrieveValue<uint64x2_t>(Val);
    return {};
  }
  case OpCode::V128__and: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    retrieveValue<uint64x2_t>(Val1) &= retrieveValue<uint64x2_t>(Val2);
    return {};
  }
  case OpCode::V128__andnot: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    retrieveValue<uint64x2_t>(Val1) &= ~retrieveValue<uint64x2_t>(Val2);
    return {};
  }
  case OpCode::V128__or: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    retrieveValue<uint64x2_t>(Val1) |= retrieveValue<uint64x2_t>(Val2);
    return {};
  }
  case OpCode::V128__xor: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    retrieveValue<uint64x2_t>(Val1) ^= retrieveValue<uint64x2_t>(Val2);
    return {};
  }
  case OpCode::V128__bitselect: {
    const uint64x2_t C = retrieveValue<uint64x2_t>(StackMgr.pop());
    const uint64x2_t Val2 = retrieveValue<uint64x2_t>(StackMgr.pop());
    uint64x2_t &Val1 = retrieveValue<uint64x2_t>(StackMgr.getTop());
    Val1 = (Val1 & C) | (Val2 & ~C);
    return {};
  }

  case OpCode::I8x16__abs: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorAbsOp<int8_t>(Val);
  }
  case OpCode::I8x16__neg: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorNegOp<int8_t>(Val);
  }
  case OpCode::I8x16__any_true: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorAnyTrueOp<uint8_t>(Val);
  }
  case OpCode::I8x16__all_true: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorAllTrueOp<uint8_t>(Val);
  }
  case OpCode::I8x16__bitmask: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorBitMaskOp<uint8_t>(Val);
  }
  case OpCode::I8x16__narrow_i16x8_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorNarrowOp<int16_t, int8_t>(Val1, Val2);
  }
  case OpCode::I8x16__narrow_i16x8_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorNarrowOp<int16_t, uint8_t>(Val1, Val2);
  }
  case OpCode::I8x16__shl: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorShlOp<uint8_t>(Val1, Val2);
  }
  case OpCode::I8x16__shr_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorShrOp<int8_t>(Val1, Val2);
  }
  case OpCode::I8x16__shr_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorShrOp<uint8_t>(Val1, Val2);
  }
  case OpCode::I8x16__add: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorAddOp<uint8_t>(Val1, Val2);
  }
  case OpCode::I8x16__add_sat_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorAddSatOp<int8_t>(Val1, Val2);
  }
  case OpCode::I8x16__add_sat_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorAddSatOp<uint8_t>(Val1, Val2);
  }
  case OpCode::I8x16__sub: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorSubOp<uint8_t>(Val1, Val2);
  }
  case OpCode::I8x16__sub_sat_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorSubSatOp<int8_t>(Val1, Val2);
  }
  case OpCode::I8x16__sub_sat_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorSubSatOp<uint8_t>(Val1, Val2);
  }
  case OpCode::I8x16__min_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMinOp<int8_t>(Val1, Val2);
  }
  case OpCode::I8x16__min_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMinOp<uint8_t>(Val1, Val2);
  }
  case OpCode::I8x16__max_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMaxOp<int8_t>(Val1, Val2);
  }
  case OpCode::I8x16__max_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMaxOp<uint8_t>(Val1, Val2);
  }
  case OpCode::I8x16__avgr_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorAvgrOp<uint8_t, uint16_t>(Val1, Val2);
  }

  case OpCode::I16x8__abs: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorAbsOp<int16_t>(Val);
  }
  case OpCode::I16x8__neg: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorNegOp<int16_t>(Val);
  }
  case OpCode::I16x8__any_true: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorAnyTrueOp<uint16_t>(Val);
  }
  case OpCode::I16x8__all_true: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorAllTrueOp<uint16_t>(Val);
  }
  case OpCode::I16x8__bitmask: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorBitMaskOp<uint16_t>(Val);
  }
  case OpCode::I16x8__narrow_i32x4_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorNarrowOp<int32_t, int16_t>(Val1, Val2);
  }
  case OpCode::I16x8__narrow_i32x4_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorNarrowOp<int32_t, uint16_t>(Val1, Val2);
  }
  case OpCode::I16x8__widen_low_i8x16_s: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorWidenLowOp<int8_t, int16_t>(Val);
  }
  case OpCode::I16x8__widen_high_i8x16_s: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorWidenHighOp<int8_t, int16_t>(Val);
  }
  case OpCode::I16x8__widen_low_i8x16_u: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorWidenLowOp<uint8_t, uint16_t>(Val);
  }
  case OpCode::I16x8__widen_high_i8x16_u: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorWidenHighOp<uint8_t, uint16_t>(Val);
  }
  case OpCode::I16x8__shl: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorShlOp<uint16_t>(Val1, Val2);
  }
  case OpCode::I16x8__shr_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorShrOp<int16_t>(Val1, Val2);
  }
  case OpCode::I16x8__shr_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorShrOp<uint16_t>(Val1, Val2);
  }
  case OpCode::I16x8__add: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorAddOp<uint16_t>(Val1, Val2);
  }
  case OpCode::I16x8__add_sat_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorAddSatOp<int16_t>(Val1, Val2);
  }
  case OpCode::I16x8__add_sat_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorAddSatOp<uint16_t>(Val1, Val2);
  }
  case OpCode::I16x8__sub: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorSubOp<uint16_t>(Val1, Val2);
  }
  case OpCode::I16x8__sub_sat_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorSubSatOp<int16_t>(Val1, Val2);
  }
  case OpCode::I16x8__sub_sat_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorSubSatOp<uint16_t>(Val1, Val2);
  }
  case OpCode::I16x8__mul: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMulOp<uint16_t>(Val1, Val2);
  }
  case OpCode::I16x8__min_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMinOp<int16_t>(Val1, Val2);
  }
  case OpCode::I16x8__min_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMinOp<uint16_t>(Val1, Val2);
  }
  case OpCode::I16x8__max_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMaxOp<int16_t>(Val1, Val2);
  }
  case OpCode::I16x8__max_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMaxOp<uint16_t>(Val1, Val2);
  }
  case OpCode::I16x8__avgr_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorAvgrOp<uint16_t, uint32_t>(Val1, Val2);
  }

  case OpCode::I32x4__abs: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorAbsOp<int32_t>(Val);
  }
  case OpCode::I32x4__neg: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorNegOp<int32_t>(Val);
  }
  case OpCode::I32x4__any_true: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorAnyTrueOp<uint32_t>(Val);
  }
  case OpCode::I32x4__all_true: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorAllTrueOp<uint32_t>(Val);
  }
  case OpCode::I32x4__bitmask: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorBitMaskOp<uint32_t>(Val);
  }
  case OpCode::I32x4__widen_low_i16x8_s: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorWidenLowOp<int16_t, int32_t>(Val);
  }
  case OpCode::I32x4__widen_high_i16x8_s: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorWidenHighOp<int16_t, int32_t>(Val);
  }
  case OpCode::I32x4__widen_low_i16x8_u: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorWidenLowOp<uint16_t, uint32_t>(Val);
  }
  case OpCode::I32x4__widen_high_i16x8_u: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorWidenHighOp<uint16_t, uint32_t>(Val);
  }
  case OpCode::I32x4__shl: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorShlOp<uint32_t>(Val1, Val2);
  }
  case OpCode::I32x4__shr_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorShrOp<int32_t>(Val1, Val2);
  }
  case OpCode::I32x4__shr_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorShrOp<uint32_t>(Val1, Val2);
  }
  case OpCode::I32x4__add: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorAddOp<uint32_t>(Val1, Val2);
  }
  case OpCode::I32x4__sub: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorSubOp<uint32_t>(Val1, Val2);
  }
  case OpCode::I32x4__mul: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMulOp<uint32_t>(Val1, Val2);
  }
  case OpCode::I32x4__min_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMinOp<int32_t>(Val1, Val2);
  }
  case OpCode::I32x4__min_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMinOp<uint32_t>(Val1, Val2);
  }
  case OpCode::I32x4__max_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMaxOp<int32_t>(Val1, Val2);
  }
  case OpCode::I32x4__max_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMaxOp<uint32_t>(Val1, Val2);
  }

  case OpCode::I64x2__neg: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorNegOp<int64_t>(Val);
  }
  case OpCode::I64x2__shl: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorShlOp<uint64_t>(Val1, Val2);
  }
  case OpCode::I64x2__shr_s: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorShrOp<int64_t>(Val1, Val2);
  }
  case OpCode::I64x2__shr_u: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorShrOp<uint64_t>(Val1, Val2);
  }
  case OpCode::I64x2__add: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorAddOp<uint64_t>(Val1, Val2);
  }
  case OpCode::I64x2__sub: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorSubOp<uint64_t>(Val1, Val2);
  }
  case OpCode::I64x2__mul: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMulOp<uint64_t>(Val1, Val2);
  }

  case OpCode::F32x4__abs: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorAbsOp<float>(Val);
  }
  case OpCode::F32x4__neg: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorNegOp<float>(Val);
  }
  case OpCode::F32x4__sqrt: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorSqrtOp<float>(Val);
  }
  case OpCode::F32x4__add: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorAddOp<float>(Val1, Val2);
  }
  case OpCode::F32x4__sub: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorSubOp<float>(Val1, Val2);
  }
  case OpCode::F32x4__mul: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMulOp<float>(Val1, Val2);
  }
  case OpCode::F32x4__div: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorDivOp<float>(Val1, Val2);
  }
  case OpCode::F32x4__min: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorFMinOp<float>(Val1, Val2);
  }
  case OpCode::F32x4__max: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorFMaxOp<float>(Val1, Val2);
  }
  case OpCode::F32x4__pmin: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMinOp<float>(Val1, Val2);
  }
  case OpCode::F32x4__pmax: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMaxOp<float>(Val1, Val2);
  }

  case OpCode::F64x2__abs: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorAbsOp<double>(Val);
  }
  case OpCode::F64x2__neg: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorNegOp<double>(Val);
  }
  case OpCode::F64x2__sqrt: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorSqrtOp<double>(Val);
  }
  case OpCode::F64x2__add: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorAddOp<double>(Val1, Val2);
  }
  case OpCode::F64x2__sub: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorSubOp<double>(Val1, Val2);
  }
  case OpCode::F64x2__mul: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMulOp<double>(Val1, Val2);
  }
  case OpCode::F64x2__div: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorDivOp<double>(Val1, Val2);
  }
  case OpCode::F64x2__min: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorFMinOp<double>(Val1, Val2);
  }
  case OpCode::F64x2__max: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorFMaxOp<double>(Val1, Val2);
  }
  case OpCode::F64x2__pmin: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMinOp<double>(Val1, Val2);
  }
  case OpCode::F64x2__pmax: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMaxOp<double>(Val1, Val2);
  }

  case OpCode::I32x4__trunc_sat_f32x4_s: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorTruncSatOp<float, int32_t>(Val);
  }
  case OpCode::I32x4__trunc_sat_f32x4_u: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorTruncSatOp<float, uint32_t>(Val);
  }
  case OpCode::F32x4__convert_i32x4_s: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorConvertOp<int32_t, float>(Val);
  }
  case OpCode::F32x4__convert_i32x4_u: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorConvertOp<uint32_t, float>(Val);
  }

  case OpCode::I8x16__mul: {
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();
    return runVectorMulOp<uint8_t>(Val1, Val2);
  }
  case OpCode::I32x4__dot_i16x8_s: {
    using int32x8_t [[gnu::vector_size(32)]] = int32_t;
    const ValVariant Val2 = StackMgr.pop();
    ValVariant &Val1 = StackMgr.getTop();

    auto &V2 = retrieveValue<int16x8_t>(Val2);
    auto &V1 = retrieveValue<int16x8_t>(Val1);
    auto &Result = retrieveValue<int32x4_t>(Val1);
    const auto M = __builtin_convertvector(V1, int32x8_t) *
                   __builtin_convertvector(V2, int32x8_t);
    const int32x4_t L = {M[0], M[2], M[4], M[6]};
    const int32x4_t R = {M[1], M[3], M[5], M[7]};
    Result = L + R;

    return {};
  }
  case OpCode::I64x2__any_true: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorAnyTrueOp<uint64_t>(Val);
  }
  case OpCode::I64x2__all_true: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorAllTrueOp<uint64_t>(Val);
  }
  case OpCode::F32x4__qfma:
  case OpCode::F32x4__qfms:
  case OpCode::F64x2__qfma:
  case OpCode::F64x2__qfms:
    /// XXX: Not in testsuite yet
    break;
  case OpCode::F32x4__ceil: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorCeilOp<float>(Val);
  }
  case OpCode::F32x4__floor: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorFloorOp<float>(Val);
  }
  case OpCode::F32x4__trunc: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorTruncOp<float>(Val);
  }
  case OpCode::F32x4__nearest: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorNearestOp<float>(Val);
  }
  case OpCode::F64x2__ceil: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorCeilOp<double>(Val);
  }
  case OpCode::F64x2__floor: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorFloorOp<double>(Val);
  }
  case OpCode::F64x2__trunc: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorTruncOp<double>(Val);
  }
  case OpCode::F64x2__nearest: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorNearestOp<double>(Val);
  }
  case OpCode::I64x2__trunc_sat_f64x2_s: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorTruncSatOp<double, int64_t>(Val);
  }
  case OpCode::I64x2__trunc_sat_f64x2_u: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorTruncSatOp<double, uint64_t>(Val);
  }
  case OpCode::F64x2__convert_i64x2_s: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorConvertOp<int64_t, double>(Val);
  }
  case OpCode::F64x2__convert_i64x2_u: {
    ValVariant &Val = StackMgr.getTop();
    return runVectorConvertOp<uint64_t, double>(Val);
  }

  default:
    __builtin_unreachable();
  }
  return Unexpect(ErrCode::InvalidOpCode);
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

    {
      CurrentStore = &StoreMgr;
      const auto &ModInst = **StoreMgr.getModule(Func.getModuleAddr());
      ExecutionContext.Memory = ModInst.MemoryPtr;
      ExecutionContext.Globals = ModInst.GlobalsPtr.data();
    }

    sigjmp_buf JumpBuffer;
    auto OldTrapJump = std::exchange(TrapJump, &JumpBuffer);

    const int Status = sigsetjmp(*TrapJump, true);
    if (Status == 0) {
      SignalEnabler Enabler;
      Wrapper(&ExecutionContext, CompiledFunc.get(), Args.data(), Rets.data());
    }

    TrapJump = std::move(OldTrapJump);

    if (Status != 0) {
      ErrCode Code = static_cast<ErrCode>(Status);
      if (Code != ErrCode::Terminated) {
        LOG(ERROR) << Code;
      }
      return Unexpect(Code);
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
