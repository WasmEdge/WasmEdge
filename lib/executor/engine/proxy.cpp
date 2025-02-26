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
thread_local std::array<uint32_t, 256> Executor::StackTrace;
thread_local size_t Executor::StackTraceSize = 0;

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
    ENTRY(kTrap, proxyTrap),
    ENTRY(kCall, proxyCall),
    ENTRY(kCallIndirect, proxyCallIndirect),
    ENTRY(kCallRef, proxyCallRef),
    ENTRY(kRefFunc, proxyRefFunc),
    ENTRY(kTableGet, proxyTableGet),
    ENTRY(kTableSet, proxyTableSet),
    ENTRY(kTableInit, proxyTableInit),
    ENTRY(kElemDrop, proxyElemDrop),
    ENTRY(kTableCopy, proxyTableCopy),
    ENTRY(kTableGrow, proxyTableGrow),
    ENTRY(kTableSize, proxyTableSize),
    ENTRY(kTableFill, proxyTableFill),
    ENTRY(kMemGrow, proxyMemGrow),
    ENTRY(kMemSize, proxyMemSize),
    ENTRY(kMemInit, proxyMemInit),
    ENTRY(kDataDrop, proxyDataDrop),
    ENTRY(kMemCopy, proxyMemCopy),
    ENTRY(kMemFill, proxyMemFill),
    ENTRY(kMemAtomicNotify, proxyMemAtomicNotify),
    ENTRY(kMemAtomicWait, proxyMemAtomicWait),
    ENTRY(kTableGetFuncSymbol, proxyTableGetFuncSymbol),
    ENTRY(kRefGetFuncSymbol, proxyRefGetFuncSymbol),
#undef ENTRY
};

#if defined(__clang_major__) && __clang_major__ >= 10
#pragma clang diagnostic pop
#endif

Expect<void> Executor::proxyTrap(Runtime::StackManager &,
                                 const uint32_t Code) noexcept {
  return Unexpect(static_cast<ErrCategory>(Code >> 24), Code);
}

Expect<void> Executor::proxyCall(Runtime::StackManager &StackMgr,
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
  EXPECTED_TRY(auto StartIt, enterFunction(StackMgr, *FuncInst, Instrs.end()));
  EXPECTED_TRY(execute(StackMgr, StartIt, Instrs.end()));

  for (uint32_t I = 0; I < ReturnsSize; ++I) {
    Rets[ReturnsSize - 1 - I] = StackMgr.pop();
  }
  return {};
}

Expect<void> Executor::proxyCallIndirect(Runtime::StackManager &StackMgr,
                                         const uint32_t TableIdx,
                                         const uint32_t FuncTypeIdx,
                                         const uint32_t FuncIdx,
                                         const ValVariant *Args,
                                         ValVariant *Rets) noexcept {
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
  EXPECTED_TRY(auto StartIt, enterFunction(StackMgr, *FuncInst, Instrs.end()));
  EXPECTED_TRY(execute(StackMgr, StartIt, Instrs.end()));

  for (uint32_t I = 0; I < ReturnsSize; ++I) {
    Rets[ReturnsSize - 1 - I] = StackMgr.pop();
  }
  return {};
}

Expect<void> Executor::proxyCallRef(Runtime::StackManager &StackMgr,
                                    const RefVariant Ref,
                                    const ValVariant *Args,
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
  EXPECTED_TRY(auto StartIt, enterFunction(StackMgr, *FuncInst, Instrs.end()));
  EXPECTED_TRY(execute(StackMgr, StartIt, Instrs.end()));

  for (uint32_t I = 0; I < ReturnsSize; ++I) {
    Rets[ReturnsSize - 1 - I] = StackMgr.pop();
  }

  return {};
}

Expect<RefVariant> Executor::proxyRefFunc(Runtime::StackManager &StackMgr,
                                          const uint32_t FuncIdx) noexcept {
  auto *FuncInst = getFuncInstByIdx(StackMgr, FuncIdx);
  assuming(FuncInst);
  return RefVariant(FuncInst);
}

Expect<RefVariant> Executor::proxyTableGet(Runtime::StackManager &StackMgr,
                                           const uint32_t TableIdx,
                                           const uint32_t Off) noexcept {
  auto *TabInst = getTabInstByIdx(StackMgr, TableIdx);
  assuming(TabInst);
  return TabInst->getRefAddr(Off);
}

Expect<void> Executor::proxyTableSet(Runtime::StackManager &StackMgr,
                                     const uint32_t TableIdx,
                                     const uint32_t Off,
                                     const RefVariant Ref) noexcept {
  auto *TabInst = getTabInstByIdx(StackMgr, TableIdx);
  assuming(TabInst);
  return TabInst->setRefAddr(Off, Ref);
}

Expect<void> Executor::proxyTableInit(Runtime::StackManager &StackMgr,
                                      const uint32_t TableIdx,
                                      const uint32_t ElemIdx,
                                      const uint32_t DstOff,
                                      const uint32_t SrcOff,
                                      const uint32_t Len) noexcept {
  auto *TabInst = getTabInstByIdx(StackMgr, TableIdx);
  assuming(TabInst);
  auto *ElemInst = getElemInstByIdx(StackMgr, ElemIdx);
  assuming(ElemInst);
  return TabInst->setRefs(ElemInst->getRefs(), DstOff, SrcOff, Len);
}

Expect<void> Executor::proxyElemDrop(Runtime::StackManager &StackMgr,
                                     const uint32_t ElemIdx) noexcept {
  auto *ElemInst = getElemInstByIdx(StackMgr, ElemIdx);
  assuming(ElemInst);
  ElemInst->clear();
  return {};
}

Expect<void> Executor::proxyTableCopy(Runtime::StackManager &StackMgr,
                                      const uint32_t TableIdxDst,
                                      const uint32_t TableIdxSrc,
                                      const uint32_t DstOff,
                                      const uint32_t SrcOff,
                                      const uint32_t Len) noexcept {
  auto *TabInstDst = getTabInstByIdx(StackMgr, TableIdxDst);
  assuming(TabInstDst);
  auto *TabInstSrc = getTabInstByIdx(StackMgr, TableIdxSrc);
  assuming(TabInstSrc);

  EXPECTED_TRY(auto Refs, TabInstSrc->getRefs(0, SrcOff + Len));
  return TabInstDst->setRefs(Refs, DstOff, SrcOff, Len);
}

Expect<uint32_t> Executor::proxyTableGrow(Runtime::StackManager &StackMgr,
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

Expect<uint32_t> Executor::proxyTableSize(Runtime::StackManager &StackMgr,
                                          const uint32_t TableIdx) noexcept {
  auto *TabInst = getTabInstByIdx(StackMgr, TableIdx);
  assuming(TabInst);
  return TabInst->getSize();
}

Expect<void> Executor::proxyTableFill(Runtime::StackManager &StackMgr,
                                      const uint32_t TableIdx,
                                      const uint32_t Off, const RefVariant Ref,
                                      const uint32_t Len) noexcept {
  auto *TabInst = getTabInstByIdx(StackMgr, TableIdx);
  assuming(TabInst);
  return TabInst->fillRefs(Ref, Off, Len);
}

Expect<uint32_t> Executor::proxyMemGrow(Runtime::StackManager &StackMgr,
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

Expect<uint32_t> Executor::proxyMemSize(Runtime::StackManager &StackMgr,
                                        const uint32_t MemIdx) noexcept {
  auto *MemInst = getMemInstByIdx(StackMgr, MemIdx);
  assuming(MemInst);
  return MemInst->getPageSize();
}

Expect<void>
Executor::proxyMemInit(Runtime::StackManager &StackMgr, const uint32_t MemIdx,
                       const uint32_t DataIdx, const uint32_t DstOff,
                       const uint32_t SrcOff, const uint32_t Len) noexcept {
  auto *MemInst = getMemInstByIdx(StackMgr, MemIdx);
  assuming(MemInst);
  auto *DataInst = getDataInstByIdx(StackMgr, DataIdx);
  assuming(DataInst);
  return MemInst->setBytes(DataInst->getData(), DstOff, SrcOff, Len);
}

Expect<void> Executor::proxyDataDrop(Runtime::StackManager &StackMgr,
                                     const uint32_t DataIdx) noexcept {
  auto *DataInst = getDataInstByIdx(StackMgr, DataIdx);
  assuming(DataInst);
  DataInst->clear();
  return {};
}

Expect<void> Executor::proxyMemCopy(Runtime::StackManager &StackMgr,
                                    const uint32_t DstMemIdx,
                                    const uint32_t SrcMemIdx,
                                    const uint32_t DstOff,
                                    const uint32_t SrcOff,
                                    const uint32_t Len) noexcept {
  auto *MemInstDst = getMemInstByIdx(StackMgr, DstMemIdx);
  assuming(MemInstDst);
  auto *MemInstSrc = getMemInstByIdx(StackMgr, SrcMemIdx);
  assuming(MemInstSrc);

  EXPECTED_TRY(auto Data, MemInstSrc->getBytes(SrcOff, Len));
  return MemInstDst->setBytes(Data, DstOff, 0, Len);
}

Expect<void> Executor::proxyMemFill(Runtime::StackManager &StackMgr,
                                    const uint32_t MemIdx, const uint32_t Off,
                                    const uint8_t Val,
                                    const uint32_t Len) noexcept {
  auto *MemInst = getMemInstByIdx(StackMgr, MemIdx);
  assuming(MemInst);
  return MemInst->fillBytes(Val, Off, Len);
}

Expect<uint32_t> Executor::proxyMemAtomicNotify(Runtime::StackManager &StackMgr,
                                                const uint32_t MemIdx,
                                                const uint32_t Offset,
                                                const uint32_t Count) noexcept {
  auto *MemInst = getMemInstByIdx(StackMgr, MemIdx);
  assuming(MemInst);
  return atomicNotify(*MemInst, Offset, Count);
}

Expect<uint32_t>
Executor::proxyMemAtomicWait(Runtime::StackManager &StackMgr,
                             const uint32_t MemIdx, const uint32_t Offset,
                             const uint64_t Expected, const int64_t Timeout,
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

Expect<void *> Executor::proxyTableGetFuncSymbol(
    Runtime::StackManager &StackMgr, const uint32_t TableIdx,
    const uint32_t FuncTypeIdx, const uint32_t FuncIdx) noexcept {
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

Expect<void *> Executor::proxyRefGetFuncSymbol(Runtime::StackManager &,
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
