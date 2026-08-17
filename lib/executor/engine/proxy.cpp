// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"
#include "runtime/instance/array.h"
#include "runtime/instance/gc.h"
#include "system/fault.h"

#include <cstddef>
#include <cstdint>

namespace WasmEdge {
namespace Executor {

thread_local Executor *Executor::This = nullptr;
thread_local Runtime::StackManager *Executor::CurrentStack = nullptr;
thread_local Executor::ExecutorContext Executor::ExecutionContext;
thread_local Executor::PendingExnStruct Executor::PendingExn;
thread_local std::array<StackTraceEntry, 256> Executor::StackTrace;
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
  StackMgr.popSpan(Span<ValVariant>(Rets, ReturnsSize));
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
    ENTRY(kWriteBarrier, proxyWriteBarrier),
    ENTRY(kGCSafepoint, proxyGCSafepoint),
    ENTRY(kCoherentRefLoad, proxyCoherentRefLoad),
    ENTRY(kCoherentRefStore, proxyCoherentRefStore),
#undef ENTRY
};

#if defined(__clang_major__) && __clang_major__ >= 10
#pragma clang diagnostic pop
#endif

Expect<void> Executor::proxyTrap(Runtime::StackManager &,
                                 const uint32_t Code) noexcept {
  return Unexpect(static_cast<ErrCategory>(Code >> 24), Code);
}

Expect<void> Executor::proxyGCSafepoint(Runtime::StackManager &) noexcept {
  // Reached only when generated code's inline poll observed the stop flag set.
  // gcSafepoint self-scans this mutator's roots (incl. a conservative native
  // scan capturing AOT register/stack refs) and parks until the collection
  // releases. No error path: parking always succeeds or returns on teardown.
  getController().gcSafepoint();
  return {};
}

Expect<void> Executor::proxyCoherentRefLoad(Runtime::StackManager &,
                                            const ValVariant *Slot,
                                            ValVariant *Out) noexcept {
  // Read the 128-bit (type, pointer) ref slot as one coherent transaction so a
  // concurrent coherent store on another mutator can never yield a torn pair.
  // Out is a caller-private buffer, so a plain store into it is fine.
  *Out = GC::loadCoherent(*Slot);
  return {};
}

Expect<void> Executor::proxyCoherentRefStore(Runtime::StackManager &,
                                             ValVariant *Slot,
                                             const ValVariant *Val) noexcept {
  // Shade the overwritten and new references (SATB) -- writeBarrier reads the
  // old pointer word with a relaxed atomic load -- then publish the new pair
  // atomically. Mirrors GlobalInstance::setValue / table ref set for compiled
  // code so the marker and a concurrent coherent reader never see a torn slot.
  getAllocator().writeBarrier(*Slot);
  getAllocator().writeBarrier(*Val);
  GC::storeCoherent(*Slot, *Val);
  return {};
}

Expect<void>
Executor::proxyCall(Runtime::StackManager &StackMgr,
                    const Runtime::Instance::ModuleInstance *ModInst,
                    const uint32_t FuncIdx, const ValVariant *Args,
                    ValVariant *Rets) noexcept {
  const auto *FuncInst = getFuncInstByIdx(ModInst, FuncIdx);
  assuming(FuncInst);
  EXPECTED_TRY(checkLazyCompilation(FuncInst));
  const auto &FuncType = FuncInst->getFuncType();
  const uint32_t ParamsSize =
      static_cast<uint32_t>(FuncType.getParamTypes().size());

  for (uint32_t I = 0; I < ParamsSize; ++I) {
    StackMgr.push(Args[I]);
  }

  auto Instrs = FuncInst->getInstrs();
  auto Res =
      enterFunction(StackMgr, *FuncInst, Instrs.end(), false, true, ModInst)
          .and_then([&](AST::InstrView::iterator StartIt) {
            return execute(StackMgr, StartIt, Instrs.end());
          });
  return callFromCompiled(StackMgr, *FuncInst, Rets, std::move(Res));
}

Expect<void>
Executor::proxyCallIndirect(Runtime::StackManager &StackMgr,
                            const Runtime::Instance::ModuleInstance *ModInst,
                            const uint32_t TableIdx, const uint32_t FuncTypeIdx,
                            const uint64_t FuncIdx, const ValVariant *Args,
                            ValVariant *Rets) noexcept {
  const auto *TabInst = getTabInstByIdx(ModInst, TableIdx);
  assuming(TabInst);

  if (unlikely(FuncIdx >= TabInst->getSize())) {
    return Unexpect(ErrCode::Value::UndefinedElement);
  }

  auto Ref = TabInst->getRefAddr(FuncIdx);
  assuming(Ref);
  if (unlikely(Ref->isNull())) {
    return Unexpect(ErrCode::Value::UninitializedElement);
  }

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
  auto Res =
      enterFunction(StackMgr, *FuncInst, Instrs.end(), false, true, ModInst)
          .and_then([&](AST::InstrView::iterator StartIt) {
            return execute(StackMgr, StartIt, Instrs.end());
          });
  return callFromCompiled(StackMgr, *FuncInst, Rets, std::move(Res));
}

Expect<void>
Executor::proxyCallRef(Runtime::StackManager &StackMgr,
                       const Runtime::Instance::ModuleInstance *ModInst,
                       const RefVariant Ref, const ValVariant *Args,
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
  auto Res =
      enterFunction(StackMgr, *FuncInst, Instrs.end(), false, true, ModInst)
          .and_then([&](AST::InstrView::iterator StartIt) {
            return execute(StackMgr, StartIt, Instrs.end());
          });
  return callFromCompiled(StackMgr, *FuncInst, Rets, std::move(Res));
}

Expect<RefVariant>
Executor::proxyRefFunc(Runtime::StackManager &,
                       const Runtime::Instance::ModuleInstance *ModInst,
                       const uint32_t FuncIdx) noexcept {
  auto *FuncInst = getFuncInstByIdx(ModInst, FuncIdx);
  assuming(FuncInst);
  return RefVariant(FuncInst->getDefType(), FuncInst);
}

Expect<RefVariant>
Executor::proxyStructNew(Runtime::StackManager &,
                         const Runtime::Instance::ModuleInstance *ModInst,
                         const uint32_t TypeIdx, const ValVariant *Args,
                         const uint32_t ArgSize) noexcept {
  // AOT alloc intrinsic: request a native-stack scan (ScanNative == true). AOT
  // code keeps operand-stack roots natively (register / native-stack slots), so
  // the coordinator's self-scan must cover the native stack or a live ref could
  // be swept. Auto cycle (Manual == false): the manual-GC toggle still gates
  // it.
  getController().collect(false, true);
  if (Args == nullptr) {
    return structNew(ModInst, TypeIdx);
  } else {
    return structNew(ModInst, TypeIdx, Span<const ValVariant>(Args, ArgSize));
  }
}

Expect<void> Executor::proxyStructGet(
    Runtime::StackManager &, const Runtime::Instance::ModuleInstance *ModInst,
    const RefVariant Ref, const uint32_t TypeIdx, const uint32_t Off,
    const bool IsSigned, ValVariant *Ret) noexcept {
  EXPECTED_TRY(auto Val, structGet(ModInst, Ref, TypeIdx, Off, IsSigned));
  *Ret = Val;
  return {};
}

Expect<void>
Executor::proxyStructSet(Runtime::StackManager &,
                         const Runtime::Instance::ModuleInstance *ModInst,
                         const RefVariant Ref, const uint32_t TypeIdx,
                         const uint32_t Off, const ValVariant *Val) noexcept {
  return structSet(ModInst, Ref, *Val, TypeIdx, Off);
}

Expect<RefVariant> Executor::proxyArrayNew(
    Runtime::StackManager &, const Runtime::Instance::ModuleInstance *ModInst,
    const uint32_t TypeIdx, const uint32_t Length, const ValVariant *Args,
    const uint32_t ArgSize) noexcept {
  getController().collect(false,
                          true); // AOT: native-stack scan (see structNew)
  assuming(ArgSize == 0 || ArgSize == 1 || ArgSize == Length);
  if (ArgSize == 0) {
    return arrayNew(ModInst, TypeIdx, Length);
  } else if (ArgSize == 1) {
    return arrayNew(ModInst, TypeIdx, Length, {Args[0]});
  } else {
    return arrayNew(ModInst, TypeIdx, Length,
                    Span<const ValVariant>(Args, ArgSize));
  }
}

Expect<RefVariant> Executor::proxyArrayNewData(
    Runtime::StackManager &, const Runtime::Instance::ModuleInstance *ModInst,
    const uint32_t TypeIdx, const uint32_t DataIdx, const uint32_t Start,
    const uint32_t Length) noexcept {
  getController().collect(false,
                          true); // AOT: native-stack scan (see structNew)
  return arrayNewData(ModInst, TypeIdx, DataIdx, Start, Length);
}

Expect<RefVariant> Executor::proxyArrayNewElem(
    Runtime::StackManager &, const Runtime::Instance::ModuleInstance *ModInst,
    const uint32_t TypeIdx, const uint32_t ElemIdx, const uint32_t Start,
    const uint32_t Length) noexcept {
  getController().collect(false,
                          true); // AOT: native-stack scan (see structNew)
  return arrayNewElem(ModInst, TypeIdx, ElemIdx, Start, Length);
}

Expect<void> Executor::proxyArrayGet(
    Runtime::StackManager &, const Runtime::Instance::ModuleInstance *ModInst,
    const RefVariant Ref, const uint32_t TypeIdx, const uint32_t Idx,
    const bool IsSigned, ValVariant *Ret) noexcept {
  EXPECTED_TRY(auto Val, arrayGet(ModInst, Ref, TypeIdx, Idx, IsSigned));
  *Ret = Val;
  return {};
}

Expect<void>
Executor::proxyArraySet(Runtime::StackManager &,
                        const Runtime::Instance::ModuleInstance *ModInst,
                        const RefVariant Ref, const uint32_t TypeIdx,
                        const uint32_t Idx, const ValVariant *Val) noexcept {
  return arraySet(ModInst, Ref, *Val, TypeIdx, Idx);
}

Expect<uint32_t> Executor::proxyArrayLen(Runtime::StackManager &,
                                         const RefVariant Ref) noexcept {
  auto *Raw = Ref.getPtr<Runtime::Instance::GCInstance::RawData>();
  if (Raw == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  const Runtime::Instance::ArrayInstance Inst{Raw};
  return Inst.getLength();
}

Expect<void> Executor::proxyArrayFill(
    Runtime::StackManager &, const Runtime::Instance::ModuleInstance *ModInst,
    const RefVariant Ref, const uint32_t TypeIdx, const uint32_t Idx,
    const uint32_t Cnt, const ValVariant *Val) noexcept {
  return arrayFill(ModInst, Ref, *Val, TypeIdx, Idx, Cnt);
}

Expect<void> Executor::proxyArrayCopy(
    Runtime::StackManager &, const Runtime::Instance::ModuleInstance *ModInst,
    const RefVariant DstRef, const uint32_t DstTypeIdx, const uint32_t DstIdx,
    const RefVariant SrcRef, const uint32_t SrcTypeIdx, const uint32_t SrcIdx,
    const uint32_t Cnt) noexcept {
  return arrayCopy(ModInst, DstRef, DstTypeIdx, DstIdx, SrcRef, SrcTypeIdx,
                   SrcIdx, Cnt);
}

Expect<void> Executor::proxyArrayInitData(
    Runtime::StackManager &, const Runtime::Instance::ModuleInstance *ModInst,
    const RefVariant Ref, const uint32_t TypeIdx, const uint32_t DataIdx,
    const uint32_t DstIdx, const uint32_t SrcIdx, const uint32_t Cnt) noexcept {
  return arrayInitData(ModInst, Ref, TypeIdx, DataIdx, DstIdx, SrcIdx, Cnt);
}

Expect<void> Executor::proxyArrayInitElem(
    Runtime::StackManager &, const Runtime::Instance::ModuleInstance *ModInst,
    const RefVariant Ref, const uint32_t TypeIdx, const uint32_t ElemIdx,
    const uint32_t DstIdx, const uint32_t SrcIdx, const uint32_t Cnt) noexcept {
  return arrayInitElem(ModInst, Ref, TypeIdx, ElemIdx, DstIdx, SrcIdx, Cnt);
}

Expect<uint32_t>
Executor::proxyRefTest(Runtime::StackManager &,
                       const Runtime::Instance::ModuleInstance *ModInst,
                       const RefVariant Ref, ValType VTTest) noexcept {
  // Copy the value type here due to handling the externalized case.
  auto VT = Ref.getType();
  if (VT.isExternalized()) {
    VT = ValType(TypeCode::Ref, TypeCode::ExternRef);
  }
  assuming(ModInst);
  Span<const AST::SubType *const> GotTypeList = ModInst->getTypeList();
  if (!VT.isAbsHeapType()) {
    // Resolve the defining module from the payload's leading
    // `const ModuleInstance *` (see getInnerPtr). Null refs carry the least
    // abstract heap type, so the payload is non-null.
    const auto *RefMod =
        Ref.getInnerPtr<const Runtime::Instance::ModuleInstance>();
    if (RefMod) {
      GotTypeList = RefMod->getTypeList();
    }
  }

  if (AST::TypeMatcher::matchType(ModInst->getTypeList(), VTTest, GotTypeList,
                                  VT)) {
    return static_cast<uint32_t>(1);
  } else {
    return static_cast<uint32_t>(0);
  }
}

Expect<RefVariant>
Executor::proxyRefCast(Runtime::StackManager &,
                       const Runtime::Instance::ModuleInstance *ModInst,
                       const RefVariant Ref, ValType VTCast) noexcept {
  // Copy the value type here due to handling the externalized case.
  auto VT = Ref.getType();
  if (VT.isExternalized()) {
    VT = ValType(TypeCode::Ref, TypeCode::ExternRef);
  }
  assuming(ModInst);
  Span<const AST::SubType *const> GotTypeList = ModInst->getTypeList();
  if (!VT.isAbsHeapType()) {
    // Resolve the defining module from the payload's leading
    // `const ModuleInstance *` (see getInnerPtr). Null refs carry the least
    // abstract heap type, so the payload is non-null.
    const auto *RefMod =
        Ref.getInnerPtr<const Runtime::Instance::ModuleInstance>();
    if (RefMod) {
      GotTypeList = RefMod->getTypeList();
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

Expect<void> Executor::proxyTableInit(
    Runtime::StackManager &, const Runtime::Instance::ModuleInstance *ModInst,
    const uint32_t TableIdx, const uint32_t ElemIdx, const uint64_t DstOff,
    const uint32_t SrcOff, const uint32_t Len) noexcept {
  auto *TabInst = getTabInstByIdx(ModInst, TableIdx);
  assuming(TabInst);
  auto *ElemInst = getElemInstByIdx(ModInst, ElemIdx);
  assuming(ElemInst);

  EXPECTED_TRY(auto Refs, ElemInst->getRefs(SrcOff, Len));
  return TabInst->setRefs(Refs, DstOff, 0, Len);
}

Expect<void>
Executor::proxyElemDrop(Runtime::StackManager &,
                        const Runtime::Instance::ModuleInstance *ModInst,
                        const uint32_t ElemIdx) noexcept {
  auto *ElemInst = getElemInstByIdx(ModInst, ElemIdx);
  assuming(ElemInst);
  ElemInst->clear();
  return {};
}

Expect<void> Executor::proxyTableCopy(
    Runtime::StackManager &, const Runtime::Instance::ModuleInstance *ModInst,
    const uint32_t TableIdxDst, const uint32_t TableIdxSrc,
    const uint64_t DstOff, const uint64_t SrcOff, const uint64_t Len) noexcept {
  auto *TabInstDst = getTabInstByIdx(ModInst, TableIdxDst);
  assuming(TabInstDst);
  auto *TabInstSrc = getTabInstByIdx(ModInst, TableIdxSrc);
  assuming(TabInstSrc);

  EXPECTED_TRY(auto Refs, TabInstSrc->getRefs(SrcOff, Len));
  return TabInstDst->setRefs(Refs, DstOff, 0, Len);
}

Expect<uint64_t>
Executor::proxyTableGrow(Runtime::StackManager &,
                         const Runtime::Instance::ModuleInstance *ModInst,
                         const uint32_t TableIdx, const RefVariant Val,
                         const uint64_t NewSize) noexcept {
  auto *TabInst = getTabInstByIdx(ModInst, TableIdx);
  assuming(TabInst);
  const auto AddrType = TabInst->getTableType().getLimit().getAddrType();
  // Read the pre-grow size INSIDE growTable's exclusive window (H-5): a
  // controller-backed grow stops the world, so a size read out here could race a
  // concurrent serialized grow.
  uint64_t CurrTableSize = 0;
  if (likely(TabInst->growTable(NewSize, Val, &CurrTableSize))) {
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

Expect<void>
Executor::proxyTableFill(Runtime::StackManager &,
                         const Runtime::Instance::ModuleInstance *ModInst,
                         const uint32_t TableIdx, const uint64_t Off,
                         const RefVariant Ref, const uint64_t Len) noexcept {
  auto *TabInst = getTabInstByIdx(ModInst, TableIdx);
  assuming(TabInst);
  return TabInst->fillRefs(Ref, Off, Len);
}

Expect<uint64_t>
Executor::proxyMemGrow(Runtime::StackManager &,
                       const Runtime::Instance::ModuleInstance *ModInst,
                       const uint32_t MemIdx, const uint64_t NewSize) noexcept {
  auto *MemInst = getMemInstByIdx(ModInst, MemIdx);
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

Expect<void> Executor::proxyMemInit(
    Runtime::StackManager &, const Runtime::Instance::ModuleInstance *ModInst,
    const uint32_t MemIdx, const uint32_t DataIdx, const uint64_t DstOff,
    const uint32_t SrcOff, const uint32_t Len) noexcept {
  auto *MemInst = getMemInstByIdx(ModInst, MemIdx);
  assuming(MemInst);
  auto *DataInst = getDataInstByIdx(ModInst, DataIdx);
  assuming(DataInst);
  return MemInst->setBytes(DataInst->getData(), DstOff, SrcOff, Len);
}

Expect<void>
Executor::proxyDataDrop(Runtime::StackManager &,
                        const Runtime::Instance::ModuleInstance *ModInst,
                        const uint32_t DataIdx) noexcept {
  auto *DataInst = getDataInstByIdx(ModInst, DataIdx);
  assuming(DataInst);
  DataInst->clear();
  return {};
}

Expect<void> Executor::proxyMemCopy(
    Runtime::StackManager &, const Runtime::Instance::ModuleInstance *ModInst,
    const uint32_t DstMemIdx, const uint32_t SrcMemIdx, const uint64_t DstOff,
    const uint64_t SrcOff, const uint64_t Len) noexcept {
  auto *MemInstDst = getMemInstByIdx(ModInst, DstMemIdx);
  assuming(MemInstDst);
  auto *MemInstSrc = getMemInstByIdx(ModInst, SrcMemIdx);
  assuming(MemInstSrc);

  // Same memory: overlapping ranges need memmove (as runMemoryCopyOp);
  // setBytes()'s std::copy corrupts a forward-overlapping copy (dst > src).
  // Validate both ranges, then memmove.
  if (MemInstSrc == MemInstDst) {
    EXPECTED_TRY(MemInstSrc->getBytes(SrcOff, Len));
    EXPECTED_TRY(MemInstDst->getBytes(DstOff, Len));
    if (likely(Len > 0)) {
      std::memmove(MemInstDst->getDataPtr() + DstOff,
                   MemInstSrc->getDataPtr() + SrcOff, Len);
    }
    return {};
  }
  EXPECTED_TRY(auto Data, MemInstSrc->getBytes(SrcOff, Len));
  return MemInstDst->setBytes(Data, DstOff, 0, Len);
}

Expect<void>
Executor::proxyMemFill(Runtime::StackManager &,
                       const Runtime::Instance::ModuleInstance *ModInst,
                       const uint32_t MemIdx, const uint64_t Off,
                       const uint8_t Val, const uint64_t Len) noexcept {
  auto *MemInst = getMemInstByIdx(ModInst, MemIdx);
  assuming(MemInst);
  return MemInst->fillBytes(Val, Off, Len);
}

Expect<uint64_t>
Executor::proxyMemAtomicNotify(Runtime::StackManager &,
                               const Runtime::Instance::ModuleInstance *ModInst,
                               const uint32_t MemIdx, const uint64_t Offset,
                               const uint64_t Count) noexcept {
  auto *MemInst = getMemInstByIdx(ModInst, MemIdx);
  assuming(MemInst);
  return atomicNotify(*MemInst, Offset, Count);
}

Expect<uint64_t> Executor::proxyMemAtomicWait(
    Runtime::StackManager &, const Runtime::Instance::ModuleInstance *ModInst,
    const uint32_t MemIdx, const uint64_t Offset, const uint64_t Expected,
    const int64_t Timeout, const uint32_t BitWidth) noexcept {
  auto *MemInst = getMemInstByIdx(ModInst, MemIdx);
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
    Runtime::StackManager &, const Runtime::Instance::ModuleInstance *ModInst,
    const uint32_t TableIdx, const uint32_t FuncTypeIdx, const uint64_t FuncIdx,
    Runtime::Instance::ModuleInstance::ModuleContext **CalleeCtxOut) noexcept {
  *CalleeCtxOut = nullptr;
  const auto *TabInst = getTabInstByIdx(ModInst, TableIdx);
  assuming(TabInst);

  if (unlikely(FuncIdx >= TabInst->getSize())) {
    return Unexpect(ErrCode::Value::UndefinedElement);
  }

  auto Ref = TabInst->getRefAddr(FuncIdx);
  assuming(Ref);
  if (unlikely(Ref->isNull())) {
    return Unexpect(ErrCode::Value::UninitializedElement);
  }

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
  if (FuncInst->getModule() != ModInst) {
    *CalleeCtxOut =
        &const_cast<Runtime::Instance::ModuleInstance *>(FuncInst->getModule())
             ->ModCtx;
  }
  return FuncInst->getSymbol().get();
}

Expect<void *> Executor::proxyRefGetFuncSymbol(
    Runtime::StackManager &, const Runtime::Instance::ModuleInstance *ModInst,
    const RefVariant Ref,
    Runtime::Instance::ModuleInstance::ModuleContext **CalleeCtxOut) noexcept {
  *CalleeCtxOut = nullptr;
  const auto *FuncInst = retrieveFuncRef(Ref);
  assuming(FuncInst);
  if (likely(FuncInst->isCompiledFunction())) {
    if (FuncInst->getModule() != ModInst) {
      *CalleeCtxOut = &const_cast<Runtime::Instance::ModuleInstance *>(
                           FuncInst->getModule())
                           ->ModCtx;
    }
    return FuncInst->getSymbol().get();
  }
  EXPECTED_TRY(checkLazyCompilation(FuncInst));

  if (unlikely(!FuncInst->isCompiledFunction())) {
    return nullptr;
  }
  if (FuncInst->getModule() != ModInst) {
    *CalleeCtxOut =
        &const_cast<Runtime::Instance::ModuleInstance *>(FuncInst->getModule())
             ->ModCtx;
  }
  return FuncInst->getSymbol().get();
}

Expect<void *> Executor::proxyFuncGetFuncSymbol(
    Runtime::StackManager &, const Runtime::Instance::ModuleInstance *ModInst,
    const uint32_t FuncIdx) noexcept {
  const auto *FuncInst = getFuncInstByIdx(ModInst, FuncIdx);
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

Expect<void>
Executor::proxyThrow(Runtime::StackManager &,
                     const Runtime::Instance::ModuleInstance *ModInst,
                     const uint32_t TagIdx, const ValVariant *Vals,
                     const uint32_t Num) noexcept {
  auto *TagInst = getTagInstByIdx(ModInst, TagIdx);
  assuming(TagInst);
  assuming(TagInst->getTagType().getAssocValSize() == Num);
  PendingExn.TagInst = TagInst;
  PendingExn.Inst = nullptr;
  PendingExn.setPayload(getController(), Span<const ValVariant>(Vals, Num));
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
  PendingExn.setPayload(getController(), ExnInst->getPayload());
  return {};
}

Expect<void>
Executor::proxyCatchPop(Runtime::StackManager &,
                        const Runtime::Instance::ModuleInstance *ModInst,
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
      assuming(ModInst);
      Inst = const_cast<Runtime::Instance::ModuleInstance *>(ModInst)
                 ->newException(
                     getAllocator(), TagInst,
                     std::vector<ValVariant>(PendingExn.getPayload()));
    }
    Out[Idx] = RefVariant(ValType(TypeCode::Ref, TypeCode::ExnRef), Inst);
  }
  // Out points into the caller's compiled frame, so the copied refs are now
  // covered by the conservative native scan this Running thread performs at its
  // next safe point (selfScanInto). Dropping the aux roots afterwards is
  // therefore safe -- and dropping them BEFORE the copy would not be.
  PendingExn.clear(getController());
  return {};
}

Expect<void> Executor::proxyWriteBarrier(Runtime::StackManager &,
                                         const ValVariant *Val) noexcept {
  // GC write barrier for compiled code: compiled stores of a ref slot (e.g.
  // global.set) write the raw address directly, then call this to shade the
  // reference so a concurrent collection does not miss an object reachable only
  // through that slot. No-op while idle; matches the interpreter barriers in
  // setValue()/structSet()/etc.
  getAllocator().writeBarrier(*Val);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
