// SPDX-License-Identifier: Apache-2.0

#include "interpreter/interpreter.h"

#include "common/log.h"
#include "system/fault.h"

namespace WasmEdge {
namespace Interpreter {

Expect<AST::InstrView::iterator>
Interpreter::enterFunction(Runtime::StoreManager &StoreMgr,
                           const Runtime::Instance::FunctionInstance &Func,
                           const AST::InstrView::iterator From) {
  /// Get function type
  const auto &FuncType = Func.getFuncType();
  const uint32_t ArgsN = FuncType.getParamTypes().size();
  const uint32_t RetsN = FuncType.getReturnTypes().size();

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
        spdlog::error(ErrCode::CostLimitExceeded);
        return Unexpect(ErrCode::CostLimitExceeded);
      }
      /// Start recording time of running host function.
      Stat->stopRecordWasm();
      Stat->startRecordHost();
    }

    /// Run host function.
    Span<ValVariant> Args = StackMgr.getTopSpan(ArgsN);
    std::vector<ValVariant> Rets(RetsN);
    auto Ret = HostFunc.run(MemoryInst, std::move(Args), Rets);

    if (Stat) {
      /// Stop recording time of running host function.
      Stat->stopRecordHost();
      Stat->startRecordWasm();
    }

    if (!Ret) {
      if (Ret.error() == ErrCode::ExecutionFailed) {
        spdlog::error(Ret.error());
      }
      return Unexpect(Ret);
    }

    /// Push returns back to stack.
    for (uint32_t I = 0; I < ArgsN; ++I) {
      ValVariant Val [[maybe_unused]] = StackMgr.pop();
    }
    for (auto &R : Rets) {
      StackMgr.push(std::move(R));
    }

    /// For host function case, the continuation will be the next.
    return From;
  } else if (Func.isCompiledFunction()) {
    /// Compiled function case: Push frame with locals and args.
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

    for (uint32_t I = 0; I < Rets.size(); ++I) {
      StackMgr.push(Rets[I]);
    }

    StackMgr.popFrame();
    /// For compiled function case, the continuation will be the next.
    return From;
  } else {
    /// Native function case: Push frame with locals and args.
    StackMgr.pushFrame(Func.getModuleAddr(), /// Module address
                       ArgsN,                /// Arguments num
                       RetsN                 /// Returns num
    );

    /// Push local variables to stack.
    for (auto &Def : Func.getLocals()) {
      for (uint32_t i = 0; i < Def.first; i++) {
        StackMgr.push(ValueFromType(Def.second));
      }
    }

    /// Enter function block []->[returns] with label{none}.
    StackMgr.pushLabel(0, RetsN, From - 1);
    /// For native function case, the continuation will be the start of
    /// function body.
    return Func.getInstrs().begin();
  }
}

std::pair<uint32_t, uint32_t>
Interpreter::getBlockArity(Runtime::StoreManager &StoreMgr,
                           const BlockType &BType) {
  uint32_t Locals = 0, Arity = 0;
  if (std::holds_alternative<ValType>(BType)) {
    Arity = (std::get<ValType>(BType) == ValType::None) ? 0 : 1;
  } else {
    /// Get function type at index x.
    const auto *ModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
    const auto *FuncType = *ModInst->getFuncType(std::get<uint32_t>(BType));
    Locals = FuncType->getParamTypes().size();
    Arity = FuncType->getReturnTypes().size();
  }
  return {Locals, Arity};
}

Expect<void> Interpreter::branchToLabel(Runtime::StoreManager &StoreMgr,
                                        const uint32_t Cnt,
                                        AST::InstrView::iterator &PC) {
  /// Get the L-th label from top of stack and the continuation instruction.
  const auto ContIt = StackMgr.getLabelWithCount(Cnt).Cont;

  /// Pop L + 1 labels and jump back.
  PC = StackMgr.popLabel(Cnt + 1);

  /// Jump to the continuation of Label if is a loop.
  if (ContIt) {
    /// Get result type for arity.
    auto BlockSig = getBlockArity(StoreMgr, (*ContIt)->getBlockType());

    /// Create Label{ loop-instruction } and push.
    StackMgr.pushLabel(BlockSig.first, BlockSig.first, PC, *ContIt);

    /// Move PC to loop start.
    PC = *ContIt;
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
} // namespace WasmEdge
