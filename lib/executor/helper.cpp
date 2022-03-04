// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include "common/log.h"
#include "system/fault.h"

#include <cstdint>
#include <numeric>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Executor {

Expect<AST::InstrView::iterator>
Executor::enterFunction(Runtime::StoreManager &StoreMgr,
                        Runtime::StackManager &StackMgr,
                        const Runtime::Instance::FunctionInstance &Func,
                        const AST::InstrView::iterator From) {
  if (unlikely(StopToken.exchange(0, std::memory_order_relaxed))) {
    spdlog::error(ErrCode::Interrupted);
    return Unexpect(ErrCode::Interrupted);
  }
  // Get function type
  const auto &FuncType = Func.getFuncType();
  const auto &ParamTypes = FuncType.getParamTypes();
  const auto &ReturnTypes = FuncType.getReturnTypes();
  const uint32_t ArgsN = static_cast<uint32_t>(ParamTypes.size());
  const uint32_t RetsN = static_cast<uint32_t>(ReturnTypes.size());

  if (Func.isHostFunction()) {
    // Host function case: Push args and call function.
    auto &HostFunc = Func.getHostFunc();

    // Get memory instance from current frame.
    // It'll be nullptr if current frame is dummy frame or no memory instance
    // in current module.
    auto *MemoryInst = getMemInstByIdx(StoreMgr, StackMgr, 0);

    if (Stat) {
      // Check host function cost.
      if (unlikely(!Stat->addCost(HostFunc.getCost()))) {
        spdlog::error(ErrCode::CostLimitExceeded);
        return Unexpect(ErrCode::CostLimitExceeded);
      }
      // Start recording time of running host function.
      Stat->stopRecordWasm();
      Stat->startRecordHost();
    }

    // Run host function.
    std::vector<ValVariant> Args(ArgsN);
    for (uint32_t I = 0; I < ArgsN; ++I) {
      Args[ArgsN - 1 - I] = StackMgr.pop(ParamTypes[ArgsN - 1 - I]);
    }
    std::vector<ValVariant> Rets(RetsN);
    auto Ret = HostFunc.run(MemoryInst, std::move(Args), Rets);

    if (Stat) {
      // Stop recording time of running host function.
      Stat->stopRecordHost();
      Stat->startRecordWasm();
    }

    if (!Ret) {
      if (Ret.error() == ErrCode::ExecutionFailed) {
        spdlog::error(Ret.error());
      }
      return Unexpect(Ret);
    }

    // Push returns back to stack.
    for (uint32_t I = 0; I < RetsN; ++I) {
      StackMgr.push(ReturnTypes[I], Rets[I]);
    }

    // For host function case, the continuation will be the next.
    return From;
  } else if (Func.isCompiledFunction()) {
    // Compiled function case: Push frame with locals and args.
    StackMgr.pushFrame(Func.getModuleAddr(), // Module address
                       From - 1,             // Return PC
                       0,                    // No Arguments in stack
                       RetsN                 // Returns num
    );

    std::vector<ValVariant> Args(ArgsN);
    for (uint32_t I = 0; I < ArgsN; ++I) {
      Args[ArgsN - 1 - I] = StackMgr.pop(ParamTypes[ArgsN - 1 - I]);
    }
    std::vector<ValVariant> Rets(RetsN);

    {
      CurrentStore = &StoreMgr;
      CurrentStack = &StackMgr;
      auto &ModInst = *(*StoreMgr.getModule(Func.getModuleAddr()));
      for (uint32_t I = 0; I < ModInst.getMemNum(); ++I) {
        auto MemoryPtr =
            reinterpret_cast<std::atomic<uint8_t *> *>(&ModInst.MemoryPtrs[I]);
        uint8_t *const DataPtr =
            (*(StoreMgr.getMemory(*ModInst.getMemAddr(I))))->getDataPtr();
        std::atomic_store_explicit(MemoryPtr, DataPtr,
                                   std::memory_order_relaxed);
      }
      ExecutionContext.Memories = ModInst.MemoryPtrs.data();
      ExecutionContext.Globals = ModInst.GlobalPtrs.data();
    }

    {
      Fault FaultHandler;
      if (auto Err = PREPARE_FAULT(FaultHandler);
          unlikely(Err != ErrCode::Success)) {
        if (Err != ErrCode::Terminated) {
          spdlog::error(Err);
        }
        return Unexpect(Err);
      }
      auto &Wrapper = FuncType.getSymbol();
      Wrapper(&ExecutionContext, Func.getSymbol().get(), Args.data(),
              Rets.data());
    }

    for (uint32_t I = 0; I < RetsN; ++I) {
      StackMgr.push(ReturnTypes[I], Rets[I]);
    }

    StackMgr.popFrame();
    // For compiled function case, the continuation will be the next.
    return From;
  } else {
    const uint32_t LocalN = std::accumulate(
        Func.getLocals().begin(), Func.getLocals().end(), UINT32_C(0),
        [](uint32_t N, const auto &Pair) -> uint32_t {
          return N + Pair.first;
        });
    // Native function case: Push frame with locals and args.
    StackMgr.pushFrame(Func.getModuleAddr(), // Module address
                       From - 1,             // Return PC
                       ArgsN + LocalN,       // Arguments num + local num
                       RetsN                 // Returns num
    );

    // Push local variables to stack.
    for (auto &Def : Func.getLocals()) {
      for (uint32_t i = 0; i < Def.first; i++) {
        StackMgr.push(Def.second, ValueFromType(Def.second));
      }
    }

    // For native function case, the continuation will be the start of
    // function body.
    return Func.getInstrs().begin();
  }
}

Expect<void> Executor::branchToLabel(Runtime::StackManager &StackMgr,
                                     uint32_t EraseBegin, uint32_t EraseEnd,
                                     int32_t PCOffset,
                                     AST::InstrView::iterator &PC) noexcept {
  // Check stop token
  if (unlikely(StopToken.exchange(0, std::memory_order_relaxed))) {
    spdlog::error(ErrCode::Interrupted);
    return Unexpect(ErrCode::Interrupted);
  }

  StackMgr.stackErase(EraseBegin, EraseEnd);
  PC += PCOffset;
  return {};
}

Runtime::Instance::TableInstance *
Executor::getTabInstByIdx(Runtime::StoreManager &StoreMgr,
                          Runtime::StackManager &StackMgr, const uint32_t Idx) {
  // When top frame is dummy frame, cannot find instance.
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
Executor::getMemInstByIdx(Runtime::StoreManager &StoreMgr,
                          Runtime::StackManager &StackMgr, const uint32_t Idx) {
  // When top frame is dummy frame, cannot find instance.
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
Executor::getGlobInstByIdx(Runtime::StoreManager &StoreMgr,
                           Runtime::StackManager &StackMgr,
                           const uint32_t Idx) {
  // When top frame is dummy frame, cannot find instance.
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
Executor::getElemInstByIdx(Runtime::StoreManager &StoreMgr,
                           Runtime::StackManager &StackMgr,
                           const uint32_t Idx) {
  // When top frame is dummy frame, cannot find instance.
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
Executor::getDataInstByIdx(Runtime::StoreManager &StoreMgr,
                           Runtime::StackManager &StackMgr,
                           const uint32_t Idx) {
  // When top frame is dummy frame, cannot find instance.
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

} // namespace Executor
} // namespace WasmEdge
