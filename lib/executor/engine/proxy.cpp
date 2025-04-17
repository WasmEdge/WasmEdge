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
  return RefVariant(FuncInst->getDefType(), FuncInst);
}

Expect<RefVariant> Executor::proxyStructNew(Runtime::StackManager &StackMgr,
                                            const uint32_t TypeIdx,
                                            const ValVariant *Args) noexcept {
  auto *DefType = getDefTypeByIdx(StackMgr, TypeIdx);
  assuming(DefType);
  const auto &CompType = DefType->getCompositeType();
  assuming(!CompType.isFunc());
  uint32_t N = static_cast<uint32_t>(CompType.getFieldTypes().size());
  std::vector<ValVariant> Vals(N);
  for (uint32_t I = 0; I < N; I++) {
    const auto &VType = CompType.getFieldTypes()[I].getStorageType();
    if (Args != nullptr) {
      Vals[I] = packVal(VType, Args[I]);
    } else {
      Vals[I] = VType.isRefType()
                    ? ValVariant(RefVariant(toBottomType(StackMgr, VType)))
                    : ValVariant(static_cast<uint128_t>(0U));
    }
  }
  auto *Inst =
      const_cast<Runtime::Instance::ModuleInstance *>(StackMgr.getModule())
          ->newStruct(TypeIdx, std::move(Vals));
  return RefVariant(Inst->getDefType(), Inst);
}

Expect<void> Executor::proxyStructGet(Runtime::StackManager &StackMgr,
                                      const RefVariant Ref,
                                      const uint32_t TypeIdx,
                                      const uint32_t Off, const bool IsSigned,
                                      ValVariant *Ret) noexcept {
  const auto *Inst = Ref.getPtr<Runtime::Instance::StructInstance>();
  if (Inst == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullStruct);
  }
  auto *DefType = getDefTypeByIdx(StackMgr, TypeIdx);
  assuming(DefType);
  const auto &CompType = DefType->getCompositeType();
  assuming(!CompType.isFunc());
  const auto &VType = CompType.getFieldTypes()[Off].getStorageType();
  *Ret = unpackVal(VType, Inst->getField(Off), IsSigned);
  return {};
}

Expect<void> Executor::proxyStructSet(Runtime::StackManager &StackMgr,
                                      const RefVariant Ref,
                                      const uint32_t TypeIdx,
                                      const uint32_t Off,
                                      const ValVariant *Val) noexcept {
  auto *Inst = Ref.getPtr<Runtime::Instance::StructInstance>();
  if (Inst == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullStruct);
  }
  auto *DefType = getDefTypeByIdx(StackMgr, TypeIdx);
  assuming(DefType);
  const auto &CompType = DefType->getCompositeType();
  assuming(!CompType.isFunc());
  const auto &VType = CompType.getFieldTypes()[Off].getStorageType();
  Inst->getField(Off) = packVal(VType, *Val);
  return {};
}

Expect<RefVariant> Executor::proxyArrayNew(Runtime::StackManager &StackMgr,
                                           const uint32_t TypeIdx,
                                           const uint32_t Length,
                                           const ValVariant *Args,
                                           const uint32_t ArgSize) noexcept {
  auto *DefType = getDefTypeByIdx(StackMgr, TypeIdx);
  assuming(DefType);
  const auto &CompType = DefType->getCompositeType();
  assuming(!CompType.isFunc());
  assuming(static_cast<uint32_t>(CompType.getFieldTypes().size()) == 1);
  const auto &VType = CompType.getFieldTypes()[0].getStorageType();
  assuming(ArgSize == 0 || ArgSize == 1 || ArgSize == Length);
  WasmEdge::Runtime::Instance::ArrayInstance *Inst = nullptr;
  Runtime::Instance::ModuleInstance *ModInst =
      const_cast<Runtime::Instance::ModuleInstance *>(StackMgr.getModule());
  if (ArgSize == 0) {
    auto InitVal = VType.isRefType()
                       ? ValVariant(RefVariant(toBottomType(StackMgr, VType)))
                       : ValVariant(static_cast<uint128_t>(0U));
    Inst = ModInst->newArray(TypeIdx, Length, InitVal);
  } else if (ArgSize == 1) {
    Inst = ModInst->newArray(TypeIdx, Length, packVal(VType, Args[0]));
  } else {
    Inst = ModInst->newArray(
        TypeIdx,
        packVals(VType, std::vector<ValVariant>(Args, Args + ArgSize)));
  }
  return RefVariant(Inst->getDefType(), Inst);
}

Expect<RefVariant> Executor::proxyArrayNewData(Runtime::StackManager &StackMgr,
                                               const uint32_t TypeIdx,
                                               const uint32_t DataIdx,
                                               const uint32_t Start,
                                               const uint32_t Length) noexcept {
  auto *DefType = getDefTypeByIdx(StackMgr, TypeIdx);
  assuming(DefType);
  auto *DataInst = getDataInstByIdx(StackMgr, DataIdx);
  assuming(DataInst);
  const auto &CompType = DefType->getCompositeType();
  assuming(!CompType.isFunc());
  assuming(static_cast<uint32_t>(CompType.getFieldTypes().size()) == 1);
  const uint32_t BSize =
      CompType.getFieldTypes()[0].getStorageType().getBitWidth() / 8;

  if (static_cast<uint64_t>(Start) + static_cast<uint64_t>(Length) * BSize >
      DataInst->getData().size()) {
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  auto *Inst =
      const_cast<Runtime::Instance::ModuleInstance *>(StackMgr.getModule())
          ->newArray(TypeIdx, Length, 0U);
  for (uint32_t Idx = 0; Idx < Length; Idx++) {
    // The value has been packed.
    Inst->getData(Idx) = DataInst->loadValue(Start + Idx * BSize, BSize);
  }
  return RefVariant(Inst->getDefType(), Inst);
}

Expect<RefVariant> Executor::proxyArrayNewElem(Runtime::StackManager &StackMgr,
                                               const uint32_t TypeIdx,
                                               const uint32_t ElemIdx,
                                               const uint32_t Start,
                                               const uint32_t Length) noexcept {
  auto *DefType = getDefTypeByIdx(StackMgr, TypeIdx);
  assuming(DefType);
  auto *ElemInst = getElemInstByIdx(StackMgr, ElemIdx);
  assuming(ElemInst);
  const auto &CompType = DefType->getCompositeType();
  assuming(!CompType.isFunc());
  assuming(static_cast<uint32_t>(CompType.getFieldTypes().size()) == 1);
  const auto &VType = CompType.getFieldTypes()[0].getStorageType();

  auto ElemSrc = ElemInst->getRefs();
  if (static_cast<uint64_t>(Start) + static_cast<uint64_t>(Length) >
      ElemSrc.size()) {
    return Unexpect(ErrCode::Value::TableOutOfBounds);
  }
  std::vector<ValVariant> Refs(ElemSrc.begin() + Start,
                               ElemSrc.begin() + Start + Length);
  auto *Inst =
      const_cast<Runtime::Instance::ModuleInstance *>(StackMgr.getModule())
          ->newArray(TypeIdx, packVals(VType, std::move(Refs)));
  return RefVariant(Inst->getDefType(), Inst);
}

Expect<void> Executor::proxyArrayGet(Runtime::StackManager &StackMgr,
                                     const RefVariant Ref,
                                     const uint32_t TypeIdx,
                                     const uint32_t Index, const bool IsSigned,
                                     ValVariant *Ret) noexcept {
  const auto *Inst = Ref.getPtr<Runtime::Instance::ArrayInstance>();
  if (Inst == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  auto *DefType = getDefTypeByIdx(StackMgr, TypeIdx);
  assuming(DefType);
  const auto &CompType = DefType->getCompositeType();
  assuming(!CompType.isFunc());
  assuming(static_cast<uint32_t>(CompType.getFieldTypes().size()) == 1);
  const auto &VType = CompType.getFieldTypes()[0].getStorageType();
  if (Index >= Inst->getLength()) {
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  *Ret = unpackVal(VType, Inst->getData(Index), IsSigned);
  return {};
}

Expect<void> Executor::proxyArraySet(Runtime::StackManager &StackMgr,
                                     const RefVariant Ref,
                                     const uint32_t TypeIdx,
                                     const uint32_t Index,
                                     const ValVariant *Val) noexcept {
  auto *Inst = Ref.getPtr<Runtime::Instance::ArrayInstance>();
  if (Inst == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  auto *DefType = getDefTypeByIdx(StackMgr, TypeIdx);
  assuming(DefType);
  const auto &CompType = DefType->getCompositeType();
  assuming(!CompType.isFunc());
  assuming(static_cast<uint32_t>(CompType.getFieldTypes().size()) == 1);
  const auto &VType = CompType.getFieldTypes()[0].getStorageType();
  if (Index >= Inst->getLength()) {
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  Inst->getData(Index) = packVal(VType, *Val);
  return {};
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
                                      const uint32_t Off, const uint32_t Cnt,
                                      const ValVariant *Val) noexcept {
  auto *Inst = Ref.getPtr<Runtime::Instance::ArrayInstance>();
  if (Inst == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  auto *DefType = getDefTypeByIdx(StackMgr, TypeIdx);
  assuming(DefType);
  const auto &CompType = DefType->getCompositeType();
  assuming(!CompType.isFunc());
  assuming(static_cast<uint32_t>(CompType.getFieldTypes().size()) == 1);
  const auto &VType = CompType.getFieldTypes()[0].getStorageType();

  if (static_cast<uint64_t>(Off) + static_cast<uint64_t>(Cnt) >
      Inst->getLength()) {
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  auto Arr = Inst->getArray();
  std::fill(Arr.begin() + Off, Arr.begin() + Off + Cnt, packVal(VType, *Val));
  return {};
}

Expect<void>
Executor::proxyArrayCopy(Runtime::StackManager &StackMgr,
                         const RefVariant DstRef, const uint32_t DstTypeIdx,
                         const uint32_t DstOff, const RefVariant SrcRef,
                         const uint32_t SrcTypeIdx, const uint32_t SrcOff,
                         const uint32_t Cnt) noexcept {
  auto *SrcInst = SrcRef.getPtr<Runtime::Instance::ArrayInstance>();
  if (SrcInst == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  auto *SrcDefType = getDefTypeByIdx(StackMgr, SrcTypeIdx);
  assuming(SrcDefType);
  const auto &SrcCompType = SrcDefType->getCompositeType();
  assuming(!SrcCompType.isFunc());
  assuming(static_cast<uint32_t>(SrcCompType.getFieldTypes().size()) == 1);
  const auto &SrcVType = SrcCompType.getFieldTypes()[0].getStorageType();

  auto *DstInst = DstRef.getPtr<Runtime::Instance::ArrayInstance>();
  if (DstInst == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  auto *DstDefType = getDefTypeByIdx(StackMgr, DstTypeIdx);
  assuming(DstDefType);
  const auto &DstCompType = DstDefType->getCompositeType();
  assuming(!DstCompType.isFunc());
  assuming(static_cast<uint32_t>(DstCompType.getFieldTypes().size()) == 1);
  const auto &DstVType = DstCompType.getFieldTypes()[0].getStorageType();

  if (static_cast<uint64_t>(SrcOff) + static_cast<uint64_t>(Cnt) >
      SrcInst->getLength()) {
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  if (static_cast<uint64_t>(DstOff) + static_cast<uint64_t>(Cnt) >
      DstInst->getLength()) {
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }

  auto SrcArr = SrcInst->getArray();
  auto DstArr = DstInst->getArray();
  if (DstOff <= SrcOff) {
    std::transform(SrcArr.begin() + SrcOff, SrcArr.begin() + SrcOff + Cnt,
                   DstArr.begin() + DstOff, [&](const ValVariant &V) {
                     return packVal(DstVType, unpackVal(SrcVType, V));
                   });
  } else {
    std::transform(std::make_reverse_iterator(SrcArr.begin() + SrcOff + Cnt),
                   std::make_reverse_iterator(SrcArr.begin() + SrcOff),
                   std::make_reverse_iterator(DstArr.begin() + DstOff + Cnt),
                   [&](const ValVariant &V) {
                     return packVal(DstVType, unpackVal(SrcVType, V));
                   });
  }
  return {};
}

Expect<void> Executor::proxyArrayInitData(
    Runtime::StackManager &StackMgr, const RefVariant Ref,
    const uint32_t TypeIdx, const uint32_t DataIdx, const uint32_t DstOff,
    const uint32_t SrcOff, const uint32_t Cnt) noexcept {
  auto *Inst = Ref.getPtr<Runtime::Instance::ArrayInstance>();
  if (Inst == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  auto *DefType = getDefTypeByIdx(StackMgr, TypeIdx);
  assuming(DefType);
  auto *DataInst = getDataInstByIdx(StackMgr, DataIdx);
  assuming(DataInst);
  const auto &CompType = DefType->getCompositeType();
  assuming(!CompType.isFunc());
  assuming(static_cast<uint32_t>(CompType.getFieldTypes().size()) == 1);
  const uint32_t BSize =
      CompType.getFieldTypes()[0].getStorageType().getBitWidth() / 8;

  if (static_cast<uint64_t>(DstOff) + static_cast<uint64_t>(Cnt) >
      Inst->getLength()) {
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  if (static_cast<uint64_t>(SrcOff) + static_cast<uint64_t>(Cnt) * BSize >
      DataInst->getData().size()) {
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }

  for (uint32_t Idx = 0; Idx < Cnt; Idx++) {
    // The value has been packed.
    Inst->getData(DstOff + Idx) =
        DataInst->loadValue(SrcOff + Idx * BSize, BSize);
  }
  return {};
}

Expect<void> Executor::proxyArrayInitElem(
    Runtime::StackManager &StackMgr, const RefVariant Ref,
    const uint32_t TypeIdx, const uint32_t ElemIdx, const uint32_t DstOff,
    const uint32_t SrcOff, const uint32_t Cnt) noexcept {
  auto *Inst = Ref.getPtr<Runtime::Instance::ArrayInstance>();
  if (Inst == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  auto *DefType = getDefTypeByIdx(StackMgr, TypeIdx);
  assuming(DefType);
  auto *ElemInst = getElemInstByIdx(StackMgr, ElemIdx);
  assuming(ElemInst);
  const auto &CompType = DefType->getCompositeType();
  assuming(!CompType.isFunc());
  assuming(static_cast<uint32_t>(CompType.getFieldTypes().size()) == 1);
  const auto &VType = CompType.getFieldTypes()[0].getStorageType();

  auto ElemSrc = ElemInst->getRefs();
  if (static_cast<uint64_t>(DstOff) + static_cast<uint64_t>(Cnt) >
      Inst->getLength()) {
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  if (static_cast<uint64_t>(SrcOff) + static_cast<uint64_t>(Cnt) >
      ElemSrc.size()) {
    return Unexpect(ErrCode::Value::TableOutOfBounds);
  }

  auto Arr = Inst->getArray();
  // The value has been packed.
  std::transform(ElemSrc.begin() + SrcOff, ElemSrc.begin() + SrcOff + Cnt,
                 Arr.begin() + DstOff,
                 [&](const RefVariant &V) { return packVal(VType, V); });
  return {};
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
