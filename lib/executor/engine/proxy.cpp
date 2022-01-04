// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"
#include "system/fault.h"

namespace WasmEdge {
namespace Executor {

thread_local Executor *Executor::This = nullptr;

template <typename RetT, typename... ArgsT>
struct Executor::ProxyHelper<Expect<RetT> (Executor::*)(Runtime::StoreManager &,
                                                        ArgsT...) noexcept> {
  template <Expect<RetT> (Executor::*Func)(Runtime::StoreManager &,
                                           ArgsT...) noexcept>
  static auto proxy(ArgsT... Args)
#if !WASMEDGE_OS_WINDOWS
      noexcept
#endif
  {
    Expect<RetT> Res = (This->*Func)(*This->CurrentStore, Args...);
    if (unlikely(!Res)) {
      Fault::emitFault(Res.error());
    }
    if constexpr (std::is_same_v<RetT, RefVariant>) {
      // Take raw value for matching calling conventions
      return Res->template get<UnknownRef>().Value;
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
const AST::Module::IntrinsicsTable Executor::Intrinsics = {
#define ENTRY(NAME, FUNC)                                                      \
  [uint8_t(AST::Module::Intrinsics::NAME)] = reinterpret_cast<void *>(         \
      &Executor::ProxyHelper<decltype(&Executor::FUNC)>::proxy<                \
          &Executor::FUNC>)
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

#if defined(__clang_major__) && __clang_major__ >= 10
#pragma clang diagnostic pop
#endif

Expect<void> Executor::trap(Runtime::StoreManager &,
                            const uint8_t Code) noexcept {
  return Unexpect(ErrCode(Code));
}

Expect<void> Executor::call(Runtime::StoreManager &StoreMgr,
                            const uint32_t FuncIndex, const ValVariant *Args,
                            ValVariant *Rets) noexcept {
  const auto *ModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
  const uint32_t FuncAddr = *ModInst->getFuncAddr(FuncIndex);
  const auto *FuncInst = *StoreMgr.getFunction(FuncAddr);
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
  if (auto Res = enterFunction(StoreMgr, *FuncInst, Instrs.end())) {
    StartIt = *Res;
  } else {
    return Unexpect(Res);
  }
  if (auto Res = execute(StoreMgr, StartIt, Instrs.end()); unlikely(!Res)) {
    return Unexpect(Res);
  }

  for (uint32_t I = 0; I < ReturnsSize; ++I) {
    Rets[ReturnsSize - 1 - I] = StackMgr.pop();
  }

  return {};
}

Expect<void> Executor::callIndirect(Runtime::StoreManager &StoreMgr,
                                    const uint32_t TableIndex,
                                    const uint32_t FuncTypeIndex,
                                    const uint32_t FuncIndex,
                                    const ValVariant *Args,
                                    ValVariant *Rets) noexcept {
  const auto *TabInst = getTabInstByIdx(StoreMgr, TableIndex);
  assuming(TabInst);

  if (unlikely(FuncIndex >= TabInst->getSize())) {
    return Unexpect(ErrCode::UndefinedElement);
  }

  auto Ref = TabInst->getRefAddr(FuncIndex);
  assuming(Ref);
  if (unlikely(isNullRef(*Ref))) {
    return Unexpect(ErrCode::UninitializedElement);
  }
  const auto FuncAddr = retrieveFuncIdx(*Ref);

  const auto ModInst = StoreMgr.getModule(StackMgr.getModuleAddr());
  assuming(ModInst && *ModInst);
  const auto TargetFuncType = (*ModInst)->getFuncType(FuncTypeIndex);
  assuming(TargetFuncType && *TargetFuncType);
  const auto FuncInst = StoreMgr.getFunction(FuncAddr);
  assuming(FuncInst && *FuncInst);
  const auto &FuncType = (*FuncInst)->getFuncType();
  if (unlikely(**TargetFuncType != FuncType)) {
    return Unexpect(ErrCode::IndirectCallTypeMismatch);
  }

  const uint32_t ParamsSize =
      static_cast<uint32_t>(FuncType.getParamTypes().size());
  const uint32_t ReturnsSize =
      static_cast<uint32_t>(FuncType.getReturnTypes().size());

  for (uint32_t I = 0; I < ParamsSize; ++I) {
    StackMgr.push(Args[I]);
  }

  auto Instrs = (*FuncInst)->getInstrs();
  AST::InstrView::iterator StartIt;
  if (auto Res = enterFunction(StoreMgr, **FuncInst, Instrs.end())) {
    StartIt = *Res;
  } else {
    return Unexpect(Res);
  }
  if (auto Res = execute(StoreMgr, StartIt, Instrs.end()); unlikely(!Res)) {
    return Unexpect(Res);
  }

  for (uint32_t I = 0; I < ReturnsSize; ++I) {
    Rets[ReturnsSize - 1 - I] = StackMgr.pop();
  }

  return {};
}

Expect<uint32_t> Executor::memGrow(Runtime::StoreManager &StoreMgr,
                                   const uint32_t NewSize) noexcept {
  auto *MemInst = getMemInstByIdx(StoreMgr, 0);
  assuming(MemInst);
  const uint32_t CurrPageSize = MemInst->getPageSize();
  if (MemInst->growPage(NewSize)) {
    return CurrPageSize;
  } else {
    return static_cast<uint32_t>(-1);
  }
}

Expect<uint32_t> Executor::memSize(Runtime::StoreManager &StoreMgr) noexcept {
  auto &MemInst = *getMemInstByIdx(StoreMgr, 0);
  return MemInst.getPageSize();
}

Expect<void> Executor::memCopy(Runtime::StoreManager &StoreMgr,
                               const uint32_t Dst, const uint32_t Src,
                               const uint32_t Len) noexcept {
  auto *MemInst = getMemInstByIdx(StoreMgr, 0);
  assuming(MemInst);

  if (auto Data = MemInst->getBytes(Src, Len); unlikely(!Data)) {
    return Unexpect(Data);
  } else {
    if (auto Res = MemInst->setBytes(*Data, Dst, 0, Len); unlikely(!Res)) {
      return Unexpect(Res);
    }
  }

  return {};
}

Expect<void> Executor::memFill(Runtime::StoreManager &StoreMgr,
                               const uint32_t Off, const uint8_t Val,
                               const uint32_t Len) noexcept {
  auto *MemInst = getMemInstByIdx(StoreMgr, 0);
  assuming(MemInst);
  if (auto Res = MemInst->fillBytes(Val, Off, Len); unlikely(!Res)) {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Executor::memInit(Runtime::StoreManager &StoreMgr,
                               const uint32_t DataIdx, const uint32_t Dst,
                               const uint32_t Src,
                               const uint32_t Len) noexcept {
  auto *MemInst = getMemInstByIdx(StoreMgr, 0);
  assuming(MemInst);
  auto *DataInst = getDataInstByIdx(StoreMgr, DataIdx);
  assuming(DataInst);

  if (auto Res = MemInst->setBytes(DataInst->getData(), Dst, Src, Len);
      unlikely(!Res)) {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Executor::dataDrop(Runtime::StoreManager &StoreMgr,
                                const uint32_t DataIdx) noexcept {
  auto *DataInst = getDataInstByIdx(StoreMgr, DataIdx);
  assuming(DataInst);
  DataInst->clear();

  return {};
}

Expect<RefVariant> Executor::tableGet(Runtime::StoreManager &StoreMgr,
                                      const uint32_t TableIndex,
                                      const uint32_t Idx) noexcept {
  auto *TabInst = getTabInstByIdx(StoreMgr, TableIndex);
  assuming(TabInst);
  if (auto Res = TabInst->getRefAddr(Idx); unlikely(!Res)) {
    return Unexpect(Res);
  } else {
    return *Res;
  }
}

Expect<void> Executor::tableSet(Runtime::StoreManager &StoreMgr,
                                const uint32_t TableIndex, const uint32_t Idx,
                                const RefVariant Ref) noexcept {
  auto *TabInst = getTabInstByIdx(StoreMgr, TableIndex);
  assuming(TabInst);
  if (auto Res = TabInst->setRefAddr(Idx, Ref); unlikely(!Res)) {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Executor::tableCopy(Runtime::StoreManager &StoreMgr,
                                 const uint32_t TableIndexSrc,
                                 const uint32_t TableIndexDst,
                                 const uint32_t Dst, const uint32_t Src,
                                 const uint32_t Len) noexcept {
  auto *TabInstSrc = getTabInstByIdx(StoreMgr, TableIndexSrc);
  assuming(TabInstSrc);
  auto *TabInstDst = getTabInstByIdx(StoreMgr, TableIndexDst);
  assuming(TabInstDst);
  if (auto Refs = TabInstSrc->getRefs(Src, Len); unlikely(!Refs)) {
    return Unexpect(Refs);
  } else {
    if (auto Res = TabInstDst->setRefs(*Refs, Dst, 0, Len); unlikely(!Res)) {
      return Unexpect(Res);
    }
  }

  return {};
}

Expect<uint32_t> Executor::tableGrow(Runtime::StoreManager &StoreMgr,
                                     const uint32_t TableIndex,
                                     const RefVariant Val,
                                     const uint32_t NewSize) noexcept {
  auto *TabInst = getTabInstByIdx(StoreMgr, TableIndex);
  assuming(TabInst);
  const uint32_t CurrTableSize = TabInst->getSize();
  if (likely(TabInst->growTable(NewSize, Val))) {
    return CurrTableSize;
  } else {
    return static_cast<uint32_t>(-1);
  }
}

Expect<uint32_t> Executor::tableSize(Runtime::StoreManager &StoreMgr,
                                     const uint32_t TableIndex) noexcept {
  auto *TabInst = getTabInstByIdx(StoreMgr, TableIndex);
  assuming(TabInst);
  return TabInst->getSize();
}

Expect<void> Executor::tableFill(Runtime::StoreManager &StoreMgr,
                                 const uint32_t TableIndex, const uint32_t Off,
                                 const RefVariant Ref,
                                 const uint32_t Len) noexcept {
  auto *TabInst = getTabInstByIdx(StoreMgr, TableIndex);
  assuming(TabInst);
  if (auto Res = TabInst->fillRefs(Ref, Off, Len); unlikely(!Res)) {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Executor::tableInit(Runtime::StoreManager &StoreMgr,
                                 const uint32_t TableIndex,
                                 const uint32_t ElementIndex,
                                 const uint32_t Dst, const uint32_t Src,
                                 const uint32_t Len) noexcept {
  auto *TabInst = getTabInstByIdx(StoreMgr, TableIndex);
  assuming(TabInst);
  auto *ElemInst = getElemInstByIdx(StoreMgr, ElementIndex);
  assuming(ElemInst);
  if (auto Res = TabInst->setRefs(ElemInst->getRefs(), Dst, Src, Len);
      unlikely(!Res)) {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Executor::elemDrop(Runtime::StoreManager &StoreMgr,
                                const uint32_t ElementIndex) noexcept {
  auto *ElemInst = getElemInstByIdx(StoreMgr, ElementIndex);
  assuming(ElemInst);
  ElemInst->clear();

  return {};
}

Expect<RefVariant> Executor::refFunc(Runtime::StoreManager &StoreMgr,
                                     const uint32_t FuncIndex) noexcept {
  const auto ModInst = StoreMgr.getModule(StackMgr.getModuleAddr());
  assuming(ModInst && *ModInst);
  const auto FuncAddr = (*ModInst)->getFuncAddr(FuncIndex);
  assuming(FuncAddr);
  return FuncRef(*FuncAddr);
}

} // namespace Executor
} // namespace WasmEdge
