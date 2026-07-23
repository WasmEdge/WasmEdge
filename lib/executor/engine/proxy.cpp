// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"
#include "system/fault.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

thread_local Executor *Executor::This = nullptr;
thread_local Runtime::StackManager *Executor::CurrentStack = nullptr;
thread_local Executor::ExecutionContextStruct Executor::ExecutionContext;
thread_local Executor::PendingExnStruct Executor::PendingExn;
thread_local std::array<uint32_t, 256> Executor::StackTrace;
thread_local size_t Executor::StackTraceSize = 0;

namespace {

/// Helper for the call proxies: keep an escaped exception pending for the
/// post-call check in the compiled caller and skip the results.
Expect<void> callFromCompiled(Runtime::StackManager &StackMgr,
                              const Runtime::Instance::FunctionInstance &Func,
                              ValVariant *Rets, Expect<void> Res) noexcept {
  if (unlikely(!Res)) {
    if (Res.error() == ErrCode::Value::PendingException) {
      return {};
    }
    return Unexpect(Res.error());
  }
  const uint32_t ReturnsSize =
      static_cast<uint32_t>(Func.getFuncType().getReturnTypes().size());
  for (uint32_t I = 0; I < ReturnsSize; ++I) {
    Rets[ReturnsSize - 1 - I] = StackMgr.pop();
  }
  return {};
}

} // namespace

template <typename RetT, typename... ArgsT>
struct Executor::ProxyHelper<Expect<RetT> (Executor::*)(Runtime::StackManager &,
                                                        ArgsT...) noexcept> {
  template <Expect<RetT> (Executor::*Func)(Runtime::StackManager &,
                                           ArgsT...) noexcept>
  static auto proxy(ArgsT... Args) {
#if defined(__s390x__)
    // Required on s390x: materializing args prevents runtime failures in
    // release builds.
    auto Materialize = [](auto &&A) -> decltype(auto) {
      using T = std::decay_t<decltype(A)>;
      if constexpr (std::is_integral_v<T>) {
        volatile T Tmp = A;
        return Tmp;
      } else {
        return std::forward<decltype(A)>(A);
      }
    };
    Expect<RetT> Res = (This->*Func)(*CurrentStack, Materialize(Args)...);
#else
    Expect<RetT> Res = (This->*Func)(*CurrentStack, Args...);
#endif
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
    ENTRY(kStructNew, proxyStructNew),
    ENTRY(kStructGet, proxyStructGet),
    ENTRY(kStructSet, proxyStructSet),
    ENTRY(kArrayNew, proxyArrayNew),
    ENTRY(kArrayNewData, proxyArrayNewData),
    ENTRY(kArrayNewElem, proxyArrayNewElem),
    ENTRY(kArrayGet, proxyArrayGet),
    ENTRY(kArraySet, proxyArraySet),
    ENTRY(kArrayLen, proxyArrayLen),
    ENTRY(kArrayFill, proxyArrayFill),
    ENTRY(kArrayCopy, proxyArrayCopy),
    ENTRY(kArrayInitData, proxyArrayInitData),
    ENTRY(kArrayInitElem, proxyArrayInitElem),
    ENTRY(kRefTest, proxyRefTest),
    ENTRY(kRefCast, proxyRefCast),
    ENTRY(kTableInit, proxyTableInit),
    ENTRY(kElemDrop, proxyElemDrop),
    ENTRY(kTableCopy, proxyTableCopy),
    ENTRY(kTableGrow, proxyTableGrow),
    ENTRY(kTableFill, proxyTableFill),
    ENTRY(kMemGrow, proxyMemGrow),
    ENTRY(kMemInit, proxyMemInit),
    ENTRY(kDataDrop, proxyDataDrop),
    ENTRY(kMemCopy, proxyMemCopy),
    ENTRY(kMemFill, proxyMemFill),
    ENTRY(kMemAtomicNotify, proxyMemAtomicNotify),
    ENTRY(kMemAtomicWait, proxyMemAtomicWait),
    ENTRY(kTableGetFuncSymbol, proxyTableGetFuncSymbol),
    ENTRY(kRefGetFuncSymbol, proxyRefGetFuncSymbol),
    ENTRY(kFuncGetFuncSymbol, proxyFuncGetFuncSymbol),
    ENTRY(kThrow, proxyThrow),
    ENTRY(kThrowRef, proxyThrowRef),
    ENTRY(kCatchPop, proxyCatchPop),
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
  assuming(FuncInst);
  EXPECTED_TRY(checkLazyCompilation(FuncInst));
  const auto &FuncType = FuncInst->getFuncType();
  const uint32_t ParamsSize =
      static_cast<uint32_t>(FuncType.getParamTypes().size());

  for (uint32_t I = 0; I < ParamsSize; ++I) {
    StackMgr.push(Args[I]);
  }

  auto Instrs = FuncInst->getInstrs();
  auto Res = enterFunction(StackMgr, *FuncInst, Instrs.end(), false, true)
                 .and_then([&](AST::InstrView::iterator StartIt) {
                   return execute(StackMgr, StartIt, Instrs.end());
                 });
  return callFromCompiled(StackMgr, *FuncInst, Rets, std::move(Res));
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
  const auto &ExpDefType = *ModInst->unsafeGetType(FuncTypeIdx);
  const auto *FuncInst = retrieveFuncRef(*Ref);
  assuming(FuncInst);

  EXPECTED_TRY(checkLazyCompilation(FuncInst));

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

  for (uint32_t I = 0; I < ParamsSize; ++I) {
    StackMgr.push(Args[I]);
  }

  auto Instrs = FuncInst->getInstrs();
  auto Res = enterFunction(StackMgr, *FuncInst, Instrs.end(), false, true)
                 .and_then([&](AST::InstrView::iterator StartIt) {
                   return execute(StackMgr, StartIt, Instrs.end());
                 });
  return callFromCompiled(StackMgr, *FuncInst, Rets, std::move(Res));
}

Expect<void> Executor::proxyCallRef(Runtime::StackManager &StackMgr,
                                    const RefVariant Ref,
                                    const ValVariant *Args,
                                    ValVariant *Rets) noexcept {
  const auto *FuncInst = retrieveFuncRef(Ref);
  if (unlikely(!FuncInst)) {
    return Unexpect(ErrCode::Value::AccessNullFunc);
  }

  EXPECTED_TRY(checkLazyCompilation(FuncInst));

  const auto &FuncType = FuncInst->getFuncType();
  const uint32_t ParamsSize =
      static_cast<uint32_t>(FuncType.getParamTypes().size());

  for (uint32_t I = 0; I < ParamsSize; ++I) {
    StackMgr.push(Args[I]);
  }

  auto Instrs = FuncInst->getInstrs();
  auto Res = enterFunction(StackMgr, *FuncInst, Instrs.end(), false, true)
                 .and_then([&](AST::InstrView::iterator StartIt) {
                   return execute(StackMgr, StartIt, Instrs.end());
                 });
  return callFromCompiled(StackMgr, *FuncInst, Rets, std::move(Res));
}

Expect<RefVariant> Executor::proxyRefFunc(Runtime::StackManager &StackMgr,
                                          const uint32_t FuncIdx) noexcept {
  auto *FuncInst = getFuncInstByIdx(StackMgr, FuncIdx);
  assuming(FuncInst);
  return RefVariant(FuncInst->getDefType(), FuncInst);
}

Expect<RefVariant> Executor::proxyStructNew(Runtime::StackManager &StackMgr,
                                            const uint32_t TypeIdx,
                                            const ValVariant *Args,
                                            const uint32_t ArgSize) noexcept {
  if (Args == nullptr) {
    return structNew(StackMgr, TypeIdx);
  } else {
    return structNew(StackMgr, TypeIdx, Span<const ValVariant>(Args, ArgSize));
  }
}

Expect<void> Executor::proxyStructGet(Runtime::StackManager &StackMgr,
                                      const RefVariant Ref,
                                      const uint32_t TypeIdx,
                                      const uint32_t Off, const bool IsSigned,
                                      ValVariant *Ret) noexcept {
  EXPECTED_TRY(auto Val, structGet(StackMgr, Ref, TypeIdx, Off, IsSigned));
  *Ret = Val;
  return {};
}

Expect<void> Executor::proxyStructSet(Runtime::StackManager &StackMgr,
                                      const RefVariant Ref,
                                      const uint32_t TypeIdx,
                                      const uint32_t Off,
                                      const ValVariant *Val) noexcept {
  return structSet(StackMgr, Ref, *Val, TypeIdx, Off);
}

Expect<RefVariant> Executor::proxyArrayNew(Runtime::StackManager &StackMgr,
                                           const uint32_t TypeIdx,
                                           const uint32_t Length,
                                           const ValVariant *Args,
                                           const uint32_t ArgSize) noexcept {
  assuming(ArgSize == 0 || ArgSize == 1 || ArgSize == Length);
  if (ArgSize == 0) {
    return arrayNew(StackMgr, TypeIdx, Length);
  } else if (ArgSize == 1) {
    return arrayNew(StackMgr, TypeIdx, Length, {Args[0]});
  } else {
    return arrayNew(StackMgr, TypeIdx, Length,
                    Span<const ValVariant>(Args, ArgSize));
  }
}

Expect<RefVariant> Executor::proxyArrayNewData(Runtime::StackManager &StackMgr,
                                               const uint32_t TypeIdx,
                                               const uint32_t DataIdx,
                                               const uint32_t Start,
                                               const uint32_t Length) noexcept {
  return arrayNewData(StackMgr, TypeIdx, DataIdx, Start, Length);
}

Expect<RefVariant> Executor::proxyArrayNewElem(Runtime::StackManager &StackMgr,
                                               const uint32_t TypeIdx,
                                               const uint32_t ElemIdx,
                                               const uint32_t Start,
                                               const uint32_t Length) noexcept {
  return arrayNewElem(StackMgr, TypeIdx, ElemIdx, Start, Length);
}

Expect<void> Executor::proxyArrayGet(Runtime::StackManager &StackMgr,
                                     const RefVariant Ref,
                                     const uint32_t TypeIdx, const uint32_t Idx,
                                     const bool IsSigned,
                                     ValVariant *Ret) noexcept {
  EXPECTED_TRY(auto Val, arrayGet(StackMgr, Ref, TypeIdx, Idx, IsSigned));
  *Ret = Val;
  return {};
}

Expect<void> Executor::proxyArraySet(Runtime::StackManager &StackMgr,
                                     const RefVariant Ref,
                                     const uint32_t TypeIdx, const uint32_t Idx,
                                     const ValVariant *Val) noexcept {
  return arraySet(StackMgr, Ref, *Val, TypeIdx, Idx);
}

Expect<uint32_t> Executor::proxyArrayLen(Runtime::StackManager &,
                                         const RefVariant Ref) noexcept {
  auto *Inst = Ref.getPtr<Runtime::Instance::ArrayInstance>();
  if (Inst == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  return Inst->getLength();
}

Expect<void> Executor::proxyArrayFill(Runtime::StackManager &StackMgr,
                                      const RefVariant Ref,
                                      const uint32_t TypeIdx,
                                      const uint32_t Idx, const uint32_t Cnt,
                                      const ValVariant *Val) noexcept {
  return arrayFill(StackMgr, Ref, *Val, TypeIdx, Idx, Cnt);
}

Expect<void>
Executor::proxyArrayCopy(Runtime::StackManager &StackMgr,
                         const RefVariant DstRef, const uint32_t DstTypeIdx,
                         const uint32_t DstIdx, const RefVariant SrcRef,
                         const uint32_t SrcTypeIdx, const uint32_t SrcIdx,
                         const uint32_t Cnt) noexcept {
  return arrayCopy(StackMgr, DstRef, DstTypeIdx, DstIdx, SrcRef, SrcTypeIdx,
                   SrcIdx, Cnt);
}

Expect<void> Executor::proxyArrayInitData(
    Runtime::StackManager &StackMgr, const RefVariant Ref,
    const uint32_t TypeIdx, const uint32_t DataIdx, const uint32_t DstIdx,
    const uint32_t SrcIdx, const uint32_t Cnt) noexcept {
  return arrayInitData(StackMgr, Ref, TypeIdx, DataIdx, DstIdx, SrcIdx, Cnt);
}

Expect<void> Executor::proxyArrayInitElem(
    Runtime::StackManager &StackMgr, const RefVariant Ref,
    const uint32_t TypeIdx, const uint32_t ElemIdx, const uint32_t DstIdx,
    const uint32_t SrcIdx, const uint32_t Cnt) noexcept {
  return arrayInitElem(StackMgr, Ref, TypeIdx, ElemIdx, DstIdx, SrcIdx, Cnt);
}

Expect<uint32_t> Executor::proxyRefTest(Runtime::StackManager &StackMgr,
                                        const RefVariant Ref,
                                        ValType VTTest) noexcept {
  // Copy the value type here due to handling the externalized case.
  auto VT = Ref.getType();
  if (VT.isExternalized()) {
    VT = ValType(TypeCode::Ref, TypeCode::ExternRef);
  }
  const auto *ModInst = StackMgr.getModule();
  assuming(ModInst);
  Span<const AST::SubType *const> GotTypeList = ModInst->getTypeList();
  if (!VT.isAbsHeapType()) {
    auto *Inst = Ref.getPtr<Runtime::Instance::CompositeBase>();
    // Reference must not be nullptr here because the null references are typed
    // with the least abstract heap type.
    if (Inst->getModule()) {
      GotTypeList = Inst->getModule()->getTypeList();
    }
  }

  if (AST::TypeMatcher::matchType(ModInst->getTypeList(), VTTest, GotTypeList,
                                  VT)) {
    return static_cast<uint32_t>(1);
  } else {
    return static_cast<uint32_t>(0);
  }
}

Expect<RefVariant> Executor::proxyRefCast(Runtime::StackManager &StackMgr,
                                          const RefVariant Ref,
                                          ValType VTCast) noexcept {
  // Copy the value type here due to handling the externalized case.
  auto VT = Ref.getType();
  if (VT.isExternalized()) {
    VT = ValType(TypeCode::Ref, TypeCode::ExternRef);
  }
  const auto *ModInst = StackMgr.getModule();
  assuming(ModInst);
  Span<const AST::SubType *const> GotTypeList = ModInst->getTypeList();
  if (!VT.isAbsHeapType()) {
    auto *Inst = Ref.getPtr<Runtime::Instance::CompositeBase>();
    // Reference must not be nullptr here because the null references are typed
    // with the least abstract heap type.
    if (Inst->getModule()) {
      GotTypeList = Inst->getModule()->getTypeList();
    }
  }

  if (!AST::TypeMatcher::matchType(ModInst->getTypeList(), VTCast, GotTypeList,
                                   VT)) {
    return Unexpect(ErrCode::Value::CastFailed);
  }
  return Ref;
}

// For the runtime value of `uint64_t`, arguments are expected to be extended
// to 64-bit width in the LLVM compiler regardless of whether the address type
// is 32 or 64 bits. On the other hand, a `uint64_t` return should handle the
// conversion to a 32- or 64-bit value according to the address type in the LLVM
// compiler.

Expect<void> Executor::proxyTableInit(Runtime::StackManager &StackMgr,
                                      const uint32_t TableIdx,
                                      const uint32_t ElemIdx,
                                      const uint64_t DstOff,
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
                                      const uint64_t DstOff,
                                      const uint64_t SrcOff,
                                      const uint64_t Len) noexcept {
  auto *TabInstDst = getTabInstByIdx(StackMgr, TableIdxDst);
  assuming(TabInstDst);
  auto *TabInstSrc = getTabInstByIdx(StackMgr, TableIdxSrc);
  assuming(TabInstSrc);

  EXPECTED_TRY(auto Refs, TabInstSrc->getRefs(0, SrcOff + Len));
  return TabInstDst->setRefs(Refs, DstOff, SrcOff, Len);
}

Expect<uint64_t> Executor::proxyTableGrow(Runtime::StackManager &StackMgr,
                                          const uint32_t TableIdx,
                                          const RefVariant Val,
                                          const uint64_t NewSize) noexcept {
  auto *TabInst = getTabInstByIdx(StackMgr, TableIdx);
  assuming(TabInst);
  const auto AddrType = TabInst->getTableType().getLimit().getAddrType();
  const uint64_t CurrTableSize = TabInst->getSize();
  if (likely(TabInst->growTable(NewSize, Val))) {
    return CurrTableSize;
  } else {
    switch (AddrType) {
    case AddressType::I32:
      return static_cast<uint32_t>(-1);
    case AddressType::I64:
      return static_cast<uint64_t>(-1);
    default:
      assumingUnreachable();
    }
  }
}

Expect<void> Executor::proxyTableFill(Runtime::StackManager &StackMgr,
                                      const uint32_t TableIdx,
                                      const uint64_t Off, const RefVariant Ref,
                                      const uint64_t Len) noexcept {
  auto *TabInst = getTabInstByIdx(StackMgr, TableIdx);
  assuming(TabInst);
  return TabInst->fillRefs(Ref, Off, Len);
}

Expect<uint64_t> Executor::proxyMemGrow(Runtime::StackManager &StackMgr,
                                        const uint32_t MemIdx,
                                        const uint64_t NewSize) noexcept {
  auto *MemInst = getMemInstByIdx(StackMgr, MemIdx);
  assuming(MemInst);
  const auto AddrType = MemInst->getMemoryType().getLimit().getAddrType();
  const uint64_t CurrPageSize = MemInst->getPageSize();
  if (MemInst->growPage(NewSize)) {
    return CurrPageSize;
  } else {
    switch (AddrType) {
    case AddressType::I32:
      return static_cast<uint32_t>(-1);
    case AddressType::I64:
      return static_cast<uint64_t>(-1);
    default:
      assumingUnreachable();
    }
  }
}

Expect<void>
Executor::proxyMemInit(Runtime::StackManager &StackMgr, const uint32_t MemIdx,
                       const uint32_t DataIdx, const uint64_t DstOff,
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
                                    const uint64_t DstOff,
                                    const uint64_t SrcOff,
                                    const uint64_t Len) noexcept {
  auto *MemInstDst = getMemInstByIdx(StackMgr, DstMemIdx);
  assuming(MemInstDst);
  auto *MemInstSrc = getMemInstByIdx(StackMgr, SrcMemIdx);
  assuming(MemInstSrc);

  EXPECTED_TRY(auto Data, MemInstSrc->getBytes(SrcOff, Len));
  return MemInstDst->setBytes(Data, DstOff, 0, Len);
}

Expect<void> Executor::proxyMemFill(Runtime::StackManager &StackMgr,
                                    const uint32_t MemIdx, const uint64_t Off,
                                    const uint8_t Val,
                                    const uint64_t Len) noexcept {
  auto *MemInst = getMemInstByIdx(StackMgr, MemIdx);
  assuming(MemInst);
  return MemInst->fillBytes(Val, Off, Len);
}

Expect<uint64_t> Executor::proxyMemAtomicNotify(Runtime::StackManager &StackMgr,
                                                const uint32_t MemIdx,
                                                const uint64_t Offset,
                                                const uint64_t Count) noexcept {
  auto *MemInst = getMemInstByIdx(StackMgr, MemIdx);
  assuming(MemInst);
  return atomicNotify(*MemInst, Offset, Count);
}

Expect<uint64_t>
Executor::proxyMemAtomicWait(Runtime::StackManager &StackMgr,
                             const uint32_t MemIdx, const uint64_t Offset,
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
  const auto &ExpDefType = *ModInst->unsafeGetType(FuncTypeIdx);
  const auto *FuncInst = retrieveFuncRef(*Ref);
  assuming(FuncInst);
  bool IsMatch = false;
  // Check if the function type matches the expected type.
  if (FuncInst->getModule() == ModInst &&
      *ExpDefType.getTypeIndex() == FuncInst->getTypeIndex()) {
    // Fast path: If the function instance is in the same module instance, we
    // can bypass the expensive structural type matching (O(N)) by checking the
    // type index directly (O(1)).
    IsMatch = true;
  } else if (FuncInst->getModule()) {
    // If the type index is not the same, we still need to check the type
    // structure. This is because the type alias may have different type
    // indices but the same type structure.
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

  EXPECTED_TRY(checkLazyCompilation(FuncInst));

  if (unlikely(!FuncInst->isCompiledFunction())) {
    return nullptr;
  }
  return FuncInst->getSymbol().get();
}

Expect<void *> Executor::proxyRefGetFuncSymbol(Runtime::StackManager &,
                                               const RefVariant Ref) noexcept {
  const auto *FuncInst = retrieveFuncRef(Ref);
  assuming(FuncInst);
  if (likely(FuncInst->isCompiledFunction())) {
    return FuncInst->getSymbol().get();
  }
  EXPECTED_TRY(checkLazyCompilation(FuncInst));

  if (unlikely(!FuncInst->isCompiledFunction())) {
    return nullptr;
  }
  return FuncInst->getSymbol().get();
}

Expect<void *>
Executor::proxyFuncGetFuncSymbol(Runtime::StackManager &StackMgr,
                                 const uint32_t FuncIdx) noexcept {
  const auto *FuncInst = getFuncInstByIdx(StackMgr, FuncIdx);
  assuming(FuncInst);
  if (likely(FuncInst->isCompiledFunction())) {
    return FuncInst->getSymbol().get();
  }
  EXPECTED_TRY(checkLazyCompilation(FuncInst));

  if (unlikely(!FuncInst->isCompiledFunction())) {
    return nullptr;
  }
  return FuncInst->getSymbol().get();
}

Expect<void> Executor::proxyThrow(Runtime::StackManager &StackMgr,
                                  const uint32_t TagIdx, const ValVariant *Vals,
                                  const uint32_t Num) noexcept {
  auto *TagInst = getTagInstByIdx(StackMgr, TagIdx);
  assuming(TagInst);
  assuming(TagInst->getTagType().getAssocValSize() == Num);
  PendingExn.TagInst = TagInst;
  PendingExn.Inst = nullptr;
  PendingExn.setPayload(Span<const ValVariant>(Vals, Num));
  return {};
}

Expect<void> Executor::proxyThrowRef(Runtime::StackManager &,
                                     const RefVariant Ref) noexcept {
  const auto *ExnInst = Ref.getPtr<Runtime::Instance::ExceptionInstance>();
  if (unlikely(ExnInst == nullptr)) {
    return Unexpect(ErrCode::Value::AccessNullException);
  }
  PendingExn.TagInst = ExnInst->getTag();
  PendingExn.Inst = ExnInst;
  PendingExn.setPayload(ExnInst->getPayload());
  return {};
}

Expect<void> Executor::proxyCatchPop(Runtime::StackManager &StackMgr,
                                     ValVariant *Out, const uint32_t PopPayload,
                                     const uint32_t NeedRef) noexcept {
  auto *TagInst = PendingExn.TagInst;
  assuming(TagInst);
  uint32_t Idx = 0;
  if (PopPayload != 0) {
    Idx = TagInst->getTagType().getAssocValSize();
    std::copy_n(PendingExn.getPayload().begin(), Idx, Out);
  }
  if (NeedRef != 0) {
    const auto *Inst = PendingExn.Inst;
    if (Inst == nullptr) {
      auto *ModInst =
          const_cast<Runtime::Instance::ModuleInstance *>(StackMgr.getModule());
      assuming(ModInst);
      Inst = ModInst->newException(
          TagInst, std::vector<ValVariant>(PendingExn.getPayload()));
    }
    Out[Idx] = RefVariant(ValType(TypeCode::Ref, TypeCode::ExnRef), Inst);
  }
  PendingExn = {};
  return {};
}

} // namespace Executor
} // namespace WasmEdge
