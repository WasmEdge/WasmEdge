// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"
#include "system/fault.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

thread_local Executor *Executor::This = nullptr;
thread_local Runtime::StoreManager *Executor::CurrentStore = nullptr;
thread_local Runtime::StackManager *Executor::CurrentStack = nullptr;
thread_local Executor::ExecutionContextStruct Executor::ExecutionContext;

template <typename RetT, typename... ArgsT>
struct Executor::ProxyHelper<Expect<RetT> (Executor::*)(
    Runtime::StoreManager &, Runtime::StackManager &, ArgsT...) noexcept> {
  template <Expect<RetT> (Executor::*Func)(
      Runtime::StoreManager &, Runtime::StackManager &, ArgsT...) noexcept>
  static auto proxy(ArgsT... Args)
#if !WASMEDGE_OS_WINDOWS
      noexcept
#endif
  {
    Expect<RetT> Res = (This->*Func)(*CurrentStore, *CurrentStack, Args...);
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

// Intrinsics table
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

Expect<void> Executor::trap(Runtime::StoreManager &, Runtime::StackManager &,
                            const uint8_t Code) noexcept {
  return Unexpect(ErrCode(Code));
}

Expect<void> Executor::call(Runtime::StoreManager &StoreMgr,
                            Runtime::StackManager &StackMgr,
                            const uint32_t FuncIdx, const ValVariant *Args,
                            ValVariant *Rets) noexcept {
  const auto *ModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
  const uint32_t FuncAddr = *ModInst->getFuncAddr(FuncIdx);
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
  if (auto Res = enterFunction(StoreMgr, StackMgr, *FuncInst, Instrs.end())) {
    StartIt = *Res;
  } else {
    return Unexpect(Res);
  }
  if (auto Res = execute(StoreMgr, StackMgr, StartIt, Instrs.end());
      unlikely(!Res)) {
    return Unexpect(Res);
  }

  for (uint32_t I = 0; I < ReturnsSize; ++I) {
    Rets[ReturnsSize - 1 - I] = StackMgr.pop();
  }

  return {};
}

Expect<void>
Executor::callIndirect(Runtime::StoreManager &StoreMgr,
                       Runtime::StackManager &StackMgr, const uint32_t TableIdx,
                       const uint32_t FuncTypeIdx, const uint32_t FuncIdx,
                       const ValVariant *Args, ValVariant *Rets) noexcept {
  const auto *TabInst = getTabInstByIdx(StoreMgr, StackMgr, TableIdx);
  assuming(TabInst);

  if (unlikely(FuncIdx >= TabInst->getSize())) {
    return Unexpect(ErrCode::UndefinedElement);
  }

  auto Ref = TabInst->getRefAddr(FuncIdx);
  assuming(Ref);
  if (unlikely(isNullRef(*Ref))) {
    return Unexpect(ErrCode::UninitializedElement);
  }
  const auto FuncAddr = retrieveFuncIdx(*Ref);

  const auto ModInst = StoreMgr.getModule(StackMgr.getModuleAddr());
  assuming(ModInst && *ModInst);
  const auto TargetFuncType = (*ModInst)->getFuncType(FuncTypeIdx);
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
  if (auto Res = enterFunction(StoreMgr, StackMgr, **FuncInst, Instrs.end())) {
    StartIt = *Res;
  } else {
    return Unexpect(Res);
  }
  if (auto Res = execute(StoreMgr, StackMgr, StartIt, Instrs.end());
      unlikely(!Res)) {
    return Unexpect(Res);
  }

  for (uint32_t I = 0; I < ReturnsSize; ++I) {
    Rets[ReturnsSize - 1 - I] = StackMgr.pop();
  }

  return {};
}

Expect<uint32_t> Executor::memGrow(Runtime::StoreManager &StoreMgr,
                                   Runtime::StackManager &StackMgr,
                                   const uint32_t MemIdx,
                                   const uint32_t NewSize) noexcept {
  auto *MemInst = getMemInstByIdx(StoreMgr, StackMgr, MemIdx);
  assuming(MemInst);
  const uint32_t CurrPageSize = MemInst->getPageSize();
  if (MemInst->growPage(NewSize)) {
    return CurrPageSize;
  } else {
    return static_cast<uint32_t>(-1);
  }
}

Expect<uint32_t> Executor::memSize(Runtime::StoreManager &StoreMgr,
                                   Runtime::StackManager &StackMgr,
                                   const uint32_t MemIdx) noexcept {
  auto *MemInst = getMemInstByIdx(StoreMgr, StackMgr, MemIdx);
  assuming(MemInst);
  return MemInst->getPageSize();
}

Expect<void> Executor::memCopy(Runtime::StoreManager &StoreMgr,
                               Runtime::StackManager &StackMgr,
                               const uint32_t DstMemIdx,
                               const uint32_t SrcMemIdx, const uint32_t DstOff,
                               const uint32_t SrcOff,
                               const uint32_t Len) noexcept {
  auto *MemInstDst = getMemInstByIdx(StoreMgr, StackMgr, DstMemIdx);
  assuming(MemInstDst);
  auto *MemInstSrc = getMemInstByIdx(StoreMgr, StackMgr, SrcMemIdx);
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

Expect<void> Executor::memFill(Runtime::StoreManager &StoreMgr,
                               Runtime::StackManager &StackMgr,
                               const uint32_t MemIdx, const uint32_t Off,
                               const uint8_t Val, const uint32_t Len) noexcept {
  auto *MemInst = getMemInstByIdx(StoreMgr, StackMgr, MemIdx);
  assuming(MemInst);
  if (auto Res = MemInst->fillBytes(Val, Off, Len); unlikely(!Res)) {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Executor::memInit(Runtime::StoreManager &StoreMgr,
                               Runtime::StackManager &StackMgr,
                               const uint32_t MemIdx, const uint32_t DataIdx,
                               const uint32_t DstOff, const uint32_t SrcOff,
                               const uint32_t Len) noexcept {
  auto *MemInst = getMemInstByIdx(StoreMgr, StackMgr, MemIdx);
  assuming(MemInst);
  auto *DataInst = getDataInstByIdx(StoreMgr, StackMgr, DataIdx);
  assuming(DataInst);

  if (auto Res = MemInst->setBytes(DataInst->getData(), DstOff, SrcOff, Len);
      unlikely(!Res)) {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Executor::dataDrop(Runtime::StoreManager &StoreMgr,
                                Runtime::StackManager &StackMgr,
                                const uint32_t DataIdx) noexcept {
  auto *DataInst = getDataInstByIdx(StoreMgr, StackMgr, DataIdx);
  assuming(DataInst);
  DataInst->clear();

  return {};
}

Expect<RefVariant> Executor::tableGet(Runtime::StoreManager &StoreMgr,
                                      Runtime::StackManager &StackMgr,
                                      const uint32_t TableIdx,
                                      const uint32_t Off) noexcept {
  auto *TabInst = getTabInstByIdx(StoreMgr, StackMgr, TableIdx);
  assuming(TabInst);
  if (auto Res = TabInst->getRefAddr(Off); unlikely(!Res)) {
    return Unexpect(Res);
  } else {
    return *Res;
  }
}

Expect<void> Executor::tableSet(Runtime::StoreManager &StoreMgr,
                                Runtime::StackManager &StackMgr,
                                const uint32_t TableIdx, const uint32_t Off,
                                const RefVariant Ref) noexcept {
  auto *TabInst = getTabInstByIdx(StoreMgr, StackMgr, TableIdx);
  assuming(TabInst);
  if (auto Res = TabInst->setRefAddr(Off, Ref); unlikely(!Res)) {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Executor::tableCopy(Runtime::StoreManager &StoreMgr,
                                 Runtime::StackManager &StackMgr,
                                 const uint32_t TableIdxDst,
                                 const uint32_t TableIdxSrc,
                                 const uint32_t DstOff, const uint32_t SrcOff,
                                 const uint32_t Len) noexcept {
  auto *TabInstDst = getTabInstByIdx(StoreMgr, StackMgr, TableIdxDst);
  assuming(TabInstDst);
  auto *TabInstSrc = getTabInstByIdx(StoreMgr, StackMgr, TableIdxSrc);
  assuming(TabInstSrc);

  if (auto Refs = TabInstSrc->getRefs(SrcOff, Len); unlikely(!Refs)) {
    return Unexpect(Refs);
  } else {
    if (auto Res = TabInstDst->setRefs(*Refs, DstOff, 0, Len); unlikely(!Res)) {
      return Unexpect(Res);
    }
  }

  return {};
}

Expect<uint32_t> Executor::tableGrow(Runtime::StoreManager &StoreMgr,
                                     Runtime::StackManager &StackMgr,
                                     const uint32_t TableIdx,
                                     const RefVariant Val,
                                     const uint32_t NewSize) noexcept {
  auto *TabInst = getTabInstByIdx(StoreMgr, StackMgr, TableIdx);
  assuming(TabInst);
  const uint32_t CurrTableSize = TabInst->getSize();
  if (likely(TabInst->growTable(NewSize, Val))) {
    return CurrTableSize;
  } else {
    return static_cast<uint32_t>(-1);
  }
}

Expect<uint32_t> Executor::tableSize(Runtime::StoreManager &StoreMgr,
                                     Runtime::StackManager &StackMgr,
                                     const uint32_t TableIdx) noexcept {
  auto *TabInst = getTabInstByIdx(StoreMgr, StackMgr, TableIdx);
  assuming(TabInst);
  return TabInst->getSize();
}

Expect<void> Executor::tableFill(Runtime::StoreManager &StoreMgr,
                                 Runtime::StackManager &StackMgr,
                                 const uint32_t TableIdx, const uint32_t Off,
                                 const RefVariant Ref,
                                 const uint32_t Len) noexcept {
  auto *TabInst = getTabInstByIdx(StoreMgr, StackMgr, TableIdx);
  assuming(TabInst);
  if (auto Res = TabInst->fillRefs(Ref, Off, Len); unlikely(!Res)) {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Executor::tableInit(Runtime::StoreManager &StoreMgr,
                                 Runtime::StackManager &StackMgr,
                                 const uint32_t TableIdx,
                                 const uint32_t ElemIdx, const uint32_t DstOff,
                                 const uint32_t SrcOff,
                                 const uint32_t Len) noexcept {
  auto *TabInst = getTabInstByIdx(StoreMgr, StackMgr, TableIdx);
  assuming(TabInst);
  auto *ElemInst = getElemInstByIdx(StoreMgr, StackMgr, ElemIdx);
  assuming(ElemInst);
  if (auto Res = TabInst->setRefs(ElemInst->getRefs(), DstOff, SrcOff, Len);
      unlikely(!Res)) {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Executor::elemDrop(Runtime::StoreManager &StoreMgr,
                                Runtime::StackManager &StackMgr,
                                const uint32_t ElemIdx) noexcept {
  auto *ElemInst = getElemInstByIdx(StoreMgr, StackMgr, ElemIdx);
  assuming(ElemInst);
  ElemInst->clear();

  return {};
}

Expect<RefVariant> Executor::refFunc(Runtime::StoreManager &StoreMgr,
                                     Runtime::StackManager &StackMgr,
                                     const uint32_t FuncIdx) noexcept {
  const auto ModInst = StoreMgr.getModule(StackMgr.getModuleAddr());
  assuming(ModInst && *ModInst);
  const auto FuncAddr = (*ModInst)->getFuncAddr(FuncIdx);
  assuming(FuncAddr);
  return FuncRef(*FuncAddr);
}

} // namespace Executor
} // namespace WasmEdge
