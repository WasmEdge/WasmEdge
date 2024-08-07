// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"
#include "system/fault.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

thread_local Executor *Executor::This = nullptr;
thread_local Runtime::StackManager *Executor::CurrentStack = nullptr;
thread_local Executor::ExecutionContextStruct Executor::ExecutionContext;

template <typename RetT, typename... ArgsT>
struct Executor::ProxyHelper<Expect<RetT> (Executor::*)(Runtime::StackManager &,
                                                        ArgsT...) noexcept> {
  template <Expect<RetT> (Executor::*Func)(Runtime::StackManager &,
                                           ArgsT...) noexcept>
  static auto proxy(ArgsT... Args) {
    Expect<RetT> Res = (This->*Func)(*CurrentStack, Args...);
    if (unlikely(!Res)) {
      Fault::emitFault(Res.error());
    }
    if constexpr (std::is_same_v<RetT, RefVariant>) {
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
      return *reinterpret_cast<__m128 *>((*Res).getRawData().data());
#else
      return (*Res).getRawData();
#endif // MSVC
    } else if constexpr (!std::is_void_v<RetT>) {
      return *Res;
    }
  }
};

#if defined(__clang_major__) && __clang_major__ >= 10
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-designator"
#endif

// Intrinsics table
const Executable::IntrinsicsTable Executor::Intrinsics = {
#if defined(_MSC_VER) && !defined(__clang__)
#define ENTRY(NAME, FUNC)                                                      \
  reinterpret_cast<void *>(&Executor::ProxyHelper<                             \
                           decltype(&Executor::FUNC)>::proxy<&Executor::FUNC>)
#else
#define ENTRY(NAME, FUNC)                                                      \
  [uint8_t(Executable::Intrinsics::NAME)] = reinterpret_cast<void *>(          \
      &Executor::ProxyHelper<decltype(&Executor::FUNC)>::proxy<                \
          &Executor::FUNC>)
#endif
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
    ENTRY(kTableGetFuncSymbol, tableGetFuncSymbol),
    ENTRY(kMemoryAtomicNotify, memoryAtomicNotify),
    ENTRY(kMemoryAtomicWait, memoryAtomicWait),
    ENTRY(kCallRef, callRef),
    ENTRY(kRefGetFuncSymbol, refGetFuncSymbol),
#undef ENTRY
};

#if defined(__clang_major__) && __clang_major__ >= 10
#pragma clang diagnostic pop
#endif

Expect<void> Executor::trap(Runtime::StackManager &,
                            const uint32_t Code) noexcept {
  return Unexpect(static_cast<ErrCategory>(Code >> 24), Code);
}

Expect<void> Executor::call(Runtime::StackManager &StackMgr,
                            const uint32_t FuncIdx, const ValVariant *Args,
                            ValVariant *Rets) noexcept {
  const auto *FuncInst = getFuncInstByIdx(StackMgr, FuncIdx);
  const auto &FuncType = FuncInst->getFuncType();
  const uint32_t ParamsSize =
      static_cast<uint32_t>(FuncType.getParamTypes().size());
  const uint32_t ReturnsSize =
      static_cast<uint32_t>(FuncType.getReturnTypes().size());

  for (uint32_t I = 0; I < ParamsSize; ++I) {
    StackMgr.push(Args[I]);
  }

  auto Instrs = FuncInst->getInstrs();
  AST::InstrView::iterator StartIt;
  if (auto Res = enterFunction(StackMgr, *FuncInst, Instrs.end())) {
    StartIt = *Res;
  } else {
    return Unexpect(Res);
  }
  if (auto Res = execute(StackMgr, StartIt, Instrs.end()); unlikely(!Res)) {
    return Unexpect(Res);
  }

  for (uint32_t I = 0; I < ReturnsSize; ++I) {
    Rets[ReturnsSize - 1 - I] = StackMgr.pop();
  }

  return {};
}

Expect<void *> Executor::tableGetFuncSymbol(Runtime::StackManager &StackMgr,
                                            const uint32_t TableIdx,
                                            const uint32_t FuncTypeIdx,
                                            const uint32_t FuncIdx) noexcept {
  const auto *TabInst = getTabInstByIdx(StackMgr, TableIdx);
  assuming(TabInst);

  if (unlikely(FuncIdx >= TabInst->getSize())) {
    return Unexpect(ErrCode::Value::UndefinedElement);
  }

  auto Ref = TabInst->getRefAddr(FuncIdx);
  assuming(Ref);
  if (unlikely(Ref->isNull())) {
    return Unexpect(ErrCode::Value::UninitializedElement);
  }

  const auto *ModInst = StackMgr.getModule();
  assuming(ModInst);
  const auto &ExpDefType = **ModInst->getType(FuncTypeIdx);
  const auto *FuncInst = retrieveFuncRef(*Ref);
  assuming(FuncInst);
  bool IsMatch = false;
  if (FuncInst->getModule()) {
    IsMatch = AST::TypeMatcher::matchType(
        ModInst->getTypeList(), *ExpDefType.getTypeIndex(),
        FuncInst->getModule()->getTypeList(), FuncInst->getTypeIndex());
  } else {
    // Independent host module instance case. Matching the composite type
    // directly.
    IsMatch = AST::TypeMatcher::matchType(
        ModInst->getTypeList(), ExpDefType.getCompositeType(),
        FuncInst->getHostFunc().getDefinedType().getCompositeType());
  }
  if (!IsMatch) {
    return Unexpect(ErrCode::Value::IndirectCallTypeMismatch);
  }

  if (unlikely(!FuncInst->isCompiledFunction())) {
    return nullptr;
  }

  return FuncInst->getSymbol().get();
}

Expect<void>
Executor::callIndirect(Runtime::StackManager &StackMgr, const uint32_t TableIdx,
                       const uint32_t FuncTypeIdx, const uint32_t FuncIdx,
                       const ValVariant *Args, ValVariant *Rets) noexcept {
  const auto *TabInst = getTabInstByIdx(StackMgr, TableIdx);
  assuming(TabInst);

  if (unlikely(FuncIdx >= TabInst->getSize())) {
    return Unexpect(ErrCode::Value::UndefinedElement);
  }

  auto Ref = TabInst->getRefAddr(FuncIdx);
  assuming(Ref);
  if (unlikely(Ref->isNull())) {
    return Unexpect(ErrCode::Value::UninitializedElement);
  }

  const auto *ModInst = StackMgr.getModule();
  assuming(ModInst);
  const auto &ExpDefType = **ModInst->getType(FuncTypeIdx);
  const auto *FuncInst = retrieveFuncRef(*Ref);
  assuming(FuncInst);
  bool IsMatch = false;
  if (FuncInst->getModule()) {
    IsMatch = AST::TypeMatcher::matchType(
        ModInst->getTypeList(), *ExpDefType.getTypeIndex(),
        FuncInst->getModule()->getTypeList(), FuncInst->getTypeIndex());
  } else {
    // Independent host module instance case. Matching the composite type
    // directly.
    IsMatch = AST::TypeMatcher::matchType(
        ModInst->getTypeList(), ExpDefType.getCompositeType(),
        FuncInst->getHostFunc().getDefinedType().getCompositeType());
  }
  if (!IsMatch) {
    return Unexpect(ErrCode::Value::IndirectCallTypeMismatch);
  }

  const auto &FuncType = FuncInst->getFuncType();
  const uint32_t ParamsSize =
      static_cast<uint32_t>(FuncType.getParamTypes().size());
  const uint32_t ReturnsSize =
      static_cast<uint32_t>(FuncType.getReturnTypes().size());

  for (uint32_t I = 0; I < ParamsSize; ++I) {
    StackMgr.push(Args[I]);
  }

  auto Instrs = FuncInst->getInstrs();
  AST::InstrView::iterator StartIt;
  if (auto Res = enterFunction(StackMgr, *FuncInst, Instrs.end())) {
    StartIt = *Res;
  } else {
    return Unexpect(Res);
  }
  if (auto Res = execute(StackMgr, StartIt, Instrs.end()); unlikely(!Res)) {
    return Unexpect(Res);
  }

  for (uint32_t I = 0; I < ReturnsSize; ++I) {
    Rets[ReturnsSize - 1 - I] = StackMgr.pop();
  }

  return {};
}

Expect<uint32_t> Executor::memGrow(Runtime::StackManager &StackMgr,
                                   const uint32_t MemIdx,
                                   const uint32_t NewSize) noexcept {
  auto *MemInst = getMemInstByIdx(StackMgr, MemIdx);
  assuming(MemInst);
  const uint32_t CurrPageSize = MemInst->getPageSize();
  if (MemInst->growPage(NewSize)) {
    return CurrPageSize;
  } else {
    return static_cast<uint32_t>(-1);
  }
}

Expect<uint32_t> Executor::memSize(Runtime::StackManager &StackMgr,
                                   const uint32_t MemIdx) noexcept {
  auto *MemInst = getMemInstByIdx(StackMgr, MemIdx);
  assuming(MemInst);
  return MemInst->getPageSize();
}

Expect<void> Executor::memCopy(Runtime::StackManager &StackMgr,
                               const uint32_t DstMemIdx,
                               const uint32_t SrcMemIdx, const uint32_t DstOff,
                               const uint32_t SrcOff,
                               const uint32_t Len) noexcept {
  auto *MemInstDst = getMemInstByIdx(StackMgr, DstMemIdx);
  assuming(MemInstDst);
  auto *MemInstSrc = getMemInstByIdx(StackMgr, SrcMemIdx);
  assuming(MemInstSrc);

  if (auto Data = MemInstSrc->getBytes(SrcOff, Len); unlikely(!Data)) {
    return Unexpect(Data);
  } else {
    if (auto Res = MemInstDst->setBytes(*Data, DstOff, 0, Len);
        unlikely(!Res)) {
      return Unexpect(Res);
    }
  }

  return {};
}

Expect<void> Executor::memFill(Runtime::StackManager &StackMgr,
                               const uint32_t MemIdx, const uint32_t Off,
                               const uint8_t Val, const uint32_t Len) noexcept {
  auto *MemInst = getMemInstByIdx(StackMgr, MemIdx);
  assuming(MemInst);
  if (auto Res = MemInst->fillBytes(Val, Off, Len); unlikely(!Res)) {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Executor::memInit(Runtime::StackManager &StackMgr,
                               const uint32_t MemIdx, const uint32_t DataIdx,
                               const uint32_t DstOff, const uint32_t SrcOff,
                               const uint32_t Len) noexcept {
  auto *MemInst = getMemInstByIdx(StackMgr, MemIdx);
  assuming(MemInst);
  auto *DataInst = getDataInstByIdx(StackMgr, DataIdx);
  assuming(DataInst);

  if (auto Res = MemInst->setBytes(DataInst->getData(), DstOff, SrcOff, Len);
      unlikely(!Res)) {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Executor::dataDrop(Runtime::StackManager &StackMgr,
                                const uint32_t DataIdx) noexcept {
  auto *DataInst = getDataInstByIdx(StackMgr, DataIdx);
  assuming(DataInst);
  DataInst->clear();

  return {};
}

Expect<RefVariant> Executor::tableGet(Runtime::StackManager &StackMgr,
                                      const uint32_t TableIdx,
                                      const uint32_t Off) noexcept {
  auto *TabInst = getTabInstByIdx(StackMgr, TableIdx);
  assuming(TabInst);
  if (auto Res = TabInst->getRefAddr(Off); unlikely(!Res)) {
    return Unexpect(Res);
  } else {
    return *Res;
  }
}

Expect<void> Executor::tableSet(Runtime::StackManager &StackMgr,
                                const uint32_t TableIdx, const uint32_t Off,
                                const RefVariant Ref) noexcept {
  auto *TabInst = getTabInstByIdx(StackMgr, TableIdx);
  assuming(TabInst);
  if (auto Res = TabInst->setRefAddr(Off, Ref); unlikely(!Res)) {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Executor::tableCopy(Runtime::StackManager &StackMgr,
                                 const uint32_t TableIdxDst,
                                 const uint32_t TableIdxSrc,
                                 const uint32_t DstOff, const uint32_t SrcOff,
                                 const uint32_t Len) noexcept {
  auto *TabInstDst = getTabInstByIdx(StackMgr, TableIdxDst);
  assuming(TabInstDst);
  auto *TabInstSrc = getTabInstByIdx(StackMgr, TableIdxSrc);
  assuming(TabInstSrc);

  if (auto Refs = TabInstSrc->getRefs(0, SrcOff + Len); unlikely(!Refs)) {
    return Unexpect(Refs);
  } else {
    if (auto Res = TabInstDst->setRefs(*Refs, DstOff, SrcOff, Len);
        unlikely(!Res)) {
      return Unexpect(Res);
    }
  }

  return {};
}

Expect<uint32_t> Executor::tableGrow(Runtime::StackManager &StackMgr,
                                     const uint32_t TableIdx,
                                     const RefVariant Val,
                                     const uint32_t NewSize) noexcept {
  auto *TabInst = getTabInstByIdx(StackMgr, TableIdx);
  assuming(TabInst);
  const uint32_t CurrTableSize = TabInst->getSize();
  if (likely(TabInst->growTable(NewSize, Val))) {
    return CurrTableSize;
  } else {
    return static_cast<uint32_t>(-1);
  }
}

Expect<uint32_t> Executor::tableSize(Runtime::StackManager &StackMgr,
                                     const uint32_t TableIdx) noexcept {
  auto *TabInst = getTabInstByIdx(StackMgr, TableIdx);
  assuming(TabInst);
  return TabInst->getSize();
}

Expect<void> Executor::tableFill(Runtime::StackManager &StackMgr,
                                 const uint32_t TableIdx, const uint32_t Off,
                                 const RefVariant Ref,
                                 const uint32_t Len) noexcept {
  auto *TabInst = getTabInstByIdx(StackMgr, TableIdx);
  assuming(TabInst);
  if (auto Res = TabInst->fillRefs(Ref, Off, Len); unlikely(!Res)) {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Executor::tableInit(Runtime::StackManager &StackMgr,
                                 const uint32_t TableIdx,
                                 const uint32_t ElemIdx, const uint32_t DstOff,
                                 const uint32_t SrcOff,
                                 const uint32_t Len) noexcept {
  auto *TabInst = getTabInstByIdx(StackMgr, TableIdx);
  assuming(TabInst);
  auto *ElemInst = getElemInstByIdx(StackMgr, ElemIdx);
  assuming(ElemInst);
  if (auto Res = TabInst->setRefs(ElemInst->getRefs(), DstOff, SrcOff, Len);
      unlikely(!Res)) {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Executor::elemDrop(Runtime::StackManager &StackMgr,
                                const uint32_t ElemIdx) noexcept {
  auto *ElemInst = getElemInstByIdx(StackMgr, ElemIdx);
  assuming(ElemInst);
  ElemInst->clear();

  return {};
}

Expect<RefVariant> Executor::refFunc(Runtime::StackManager &StackMgr,
                                     const uint32_t FuncIdx) noexcept {
  auto *FuncInst = getFuncInstByIdx(StackMgr, FuncIdx);
  assuming(FuncInst);
  return RefVariant(FuncInst);
}

Expect<uint32_t> Executor::memoryAtomicNotify(Runtime::StackManager &StackMgr,
                                              const uint32_t MemIdx,
                                              const uint32_t Offset,
                                              const uint32_t Count) noexcept {
  auto *MemInst = getMemInstByIdx(StackMgr, MemIdx);
  assuming(MemInst);

  return atomicNotify(*MemInst, Offset, Count);
}

Expect<uint32_t> Executor::memoryAtomicWait(Runtime::StackManager &StackMgr,
                                            const uint32_t MemIdx,
                                            const uint32_t Offset,
                                            const uint64_t Expected,
                                            const int64_t Timeout,
                                            const uint32_t BitWidth) noexcept {
  auto *MemInst = getMemInstByIdx(StackMgr, MemIdx);
  assuming(MemInst);

  if (BitWidth == 64) {
    return atomicWait<uint64_t>(*MemInst, Offset, Expected, Timeout);
  } else if (BitWidth == 32) {
    return atomicWait<uint32_t>(*MemInst, Offset,
                                static_cast<uint32_t>(Expected), Timeout);
  }

  assumingUnreachable();
}

Expect<void> Executor::callRef(Runtime::StackManager &StackMgr,
                               const RefVariant Ref, const ValVariant *Args,
                               ValVariant *Rets) noexcept {
  const auto *FuncInst = retrieveFuncRef(Ref);
  const auto &FuncType = FuncInst->getFuncType();
  const uint32_t ParamsSize =
      static_cast<uint32_t>(FuncType.getParamTypes().size());
  const uint32_t ReturnsSize =
      static_cast<uint32_t>(FuncType.getReturnTypes().size());

  for (uint32_t I = 0; I < ParamsSize; ++I) {
    StackMgr.push(Args[I]);
  }

  auto Instrs = FuncInst->getInstrs();
  AST::InstrView::iterator StartIt;
  if (auto Res = enterFunction(StackMgr, *FuncInst, Instrs.end())) {
    StartIt = *Res;
  } else {
    return Unexpect(Res);
  }
  if (auto Res = execute(StackMgr, StartIt, Instrs.end()); unlikely(!Res)) {
    return Unexpect(Res);
  }

  for (uint32_t I = 0; I < ReturnsSize; ++I) {
    Rets[ReturnsSize - 1 - I] = StackMgr.pop();
  }

  return {};
}

Expect<void *> Executor::refGetFuncSymbol(Runtime::StackManager &,
                                          const RefVariant Ref) noexcept {
  const auto *FuncInst = retrieveFuncRef(Ref);
  assuming(FuncInst);
  if (unlikely(!FuncInst->isCompiledFunction())) {
    return nullptr;
  }
  return FuncInst->getSymbol().get();
}

} // namespace Executor
} // namespace WasmEdge
