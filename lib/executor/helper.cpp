// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include "common/spdlog.h"
#include "system/fault.h"
#include "system/stacktrace.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Executor {

Executor::SavedThreadLocal::SavedThreadLocal(
    Executor &Ex, Runtime::StackManager &StackMgr,
    const Runtime::Instance::FunctionInstance &Func) noexcept {
  // Prepare the execution context.
  auto *ModInst =
      const_cast<Runtime::Instance::ModuleInstance *>(Func.getModule());
  SavedThis = This;
  This = &Ex;

  SavedExecutionContext = ExecutionContext;
  ExecutionContext.StopToken = &Ex.StopToken;
  ExecutionContext.Memories = ModInst->MemoryPtrs.data();
  ExecutionContext.Globals = ModInst->GlobalPtrs.data();
  if (Ex.Stat) {
    ExecutionContext.InstrCount = &Ex.Stat->getInstrCountRef();
    ExecutionContext.CostTable = Ex.Stat->getCostTable().data();
    ExecutionContext.Gas = &Ex.Stat->getTotalCostRef();
    ExecutionContext.GasLimit = Ex.Stat->getCostLimit();
  }

  SavedCurrentStack = CurrentStack;
  CurrentStack = &StackMgr;
}

Executor::SavedThreadLocal::~SavedThreadLocal() noexcept {
  CurrentStack = SavedCurrentStack;
  ExecutionContext = SavedExecutionContext;
  This = SavedThis;
}

Expect<AST::InstrView::iterator>
Executor::enterFunction(Runtime::StackManager &StackMgr,
                        const Runtime::Instance::FunctionInstance &Func,
                        const AST::InstrView::iterator RetIt, bool IsTailCall) {
  // RetIt: the return position when the entered function returns.

  // Check if the interruption occurs.
  if (unlikely(StopToken.exchange(0, std::memory_order_relaxed))) {
    spdlog::error(ErrCode::Value::Interrupted);
    return Unexpect(ErrCode::Value::Interrupted);
  }

  // Get function type for the params and returns num.
  const auto &FuncType = Func.getFuncType();
  const uint32_t ArgsN = static_cast<uint32_t>(FuncType.getParamTypes().size());
  const uint32_t RetsN =
      static_cast<uint32_t>(FuncType.getReturnTypes().size());

  // For the exception handler, remove the inactive handlers caused by the
  // branches.
  if (likely(RetIt)) {
    StackMgr.removeInactiveHandler(RetIt - 1);
  }

  if (Func.isHostFunction()) {
    // Host function case: Push args and call function.
    auto &HostFunc = Func.getHostFunc();

    // Generate CallingFrame from current frame.
    // The module instance will be nullptr if current frame is a dummy frame.
    // For this case, use the module instance of this host function.
    const auto *ModInst = StackMgr.getModule();
    if (ModInst == nullptr) {
      ModInst = Func.getModule();
    }
    Runtime::CallingFrame CallFrame(this, ModInst);

    // Check for call stack overflow before pushing frame.
    if (unlikely(StackMgr.getFrameDepth() >=
                 Conf.getRuntimeConfigure().getMaxCallDepth())) {
      spdlog::error(ErrCode::Value::CallStackExhausted);
      return Unexpect(ErrCode::Value::CallStackExhausted);
    }

    // Push frame.
    StackMgr.pushFrame(Func.getModule(), // Module instance
                       RetIt,            // Return PC
                       ArgsN,            // Only args, no locals in stack
                       RetsN,            // Returns num
                       IsTailCall        // For tail-call
    );

    // Do the statistics if the statistics turned on.
    if (Stat) {
      // Check host function cost.
      if (unlikely(!Stat->addCost(HostFunc.getCost()))) {
        spdlog::error(ErrCode::Value::CostLimitExceeded);
        return Unexpect(ErrCode::Value::CostLimitExceeded);
      }
      // Start recording time of running host function.
      Stat->stopRecordWasm();
      Stat->startRecordHost();
    }

    // Call pre-host-function
    HostFuncHelper.invokePreHostFunc();

    // Run host function.
    Span<ValVariant> Args = StackMgr.getTopSpan(ArgsN);
    for (uint32_t I = 0; I < ArgsN; I++) {
      // For the number type cases of the arguments, the unused bits should be
      // erased due to the security issue.
      cleanNumericVal(Args[I], FuncType.getParamTypes()[I]);
    }
    std::vector<ValVariant> Rets(RetsN);
    auto Ret = HostFunc.run(CallFrame, std::move(Args), Rets);

    // Call post-host-function
    HostFuncHelper.invokePostHostFunc();

    // Do the statistics if the statistics turned on.
    if (Stat) {
      // Stop recording time of running host function.
      Stat->stopRecordHost();
      Stat->startRecordWasm();
    }

    // Check the host function execution status.
    if (!Ret) {
      if (Ret.error() == ErrCode::Value::HostFuncError ||
          Ret.error().getCategory() != ErrCategory::WASM) {
        spdlog::error(Ret.error());
      }
      return Unexpect(Ret);
    }

    // Push returns back to stack.
    for (auto &R : Rets) {
      StackMgr.push(std::move(R));
    }

    // For host function case, the continuation will be the continuation from
    // the popped frame.
    return StackMgr.popFrame();
  } else if (Func.isCompiledFunction()) {
    // Compiled function case: Execute the function and jump to the
    // continuation.

    // Check for call stack overflow before pushing frame.
    if (unlikely(StackMgr.getFrameDepth() >=
                 Conf.getRuntimeConfigure().getMaxCallDepth())) {
      spdlog::error(ErrCode::Value::CallStackExhausted);
      return Unexpect(ErrCode::Value::CallStackExhausted);
    }

    // Push frame.
    StackMgr.pushFrame(Func.getModule(), // Module instance
                       RetIt,            // Return PC
                       ArgsN,            // Only args, no locals in stack
                       RetsN,            // Returns num
                       IsTailCall        // For tail-call
    );

    // Prepare arguments.
    Span<ValVariant> Args = StackMgr.getTopSpan(ArgsN);
    std::vector<ValVariant> Rets(RetsN);
    SavedThreadLocal Saved(*this, StackMgr, Func);

    ErrCode Err;
    try {
      // Get symbol and execute the function.
      Fault FaultHandler;
      uint32_t Code = PREPARE_FAULT(FaultHandler);
      if (Code != 0) {
        auto InnerStackTrace = FaultHandler.stacktrace();
        {
          std::array<void *, 256> Buffer;
          auto OuterStackTrace = stackTrace(Buffer);
          while (!OuterStackTrace.empty() && !InnerStackTrace.empty() &&
                 InnerStackTrace[InnerStackTrace.size() - 1] ==
                     OuterStackTrace[OuterStackTrace.size() - 1]) {
            InnerStackTrace = InnerStackTrace.first(InnerStackTrace.size() - 1);
            OuterStackTrace = OuterStackTrace.first(OuterStackTrace.size() - 1);
          }
        }
        StackTraceSize =
            compiledStackTrace(StackMgr, InnerStackTrace, StackTrace).size();
        Err = ErrCode(static_cast<ErrCategory>(Code >> 24), Code);
      } else {
        auto &Wrapper = FuncType.getSymbol();
        Wrapper(&ExecutionContext, Func.getSymbol().get(), Args.data(),
                Rets.data());
      }
    } catch (const ErrCode &E) {
      Err = E;
    }
    if (unlikely(Err)) {
      if (Err != ErrCode::Value::Terminated) {
        spdlog::error(Err);
      }
      StackTraceSize +=
          interpreterStackTrace(
              StackMgr, Span<uint32_t>{StackTrace}.subspan(StackTraceSize))
              .size();
      return Unexpect(Err);
    }

    // Push returns back to stack.
    for (uint32_t I = 0; I < Rets.size(); ++I) {
      StackMgr.push(Rets[I]);
    }

    // For compiled function case, the continuation will be the continuation
    // from the popped frame.
    return StackMgr.popFrame();
  } else {
    // Native function case: Jump to the start of the function body.

    // Check for call stack overflow before pushing frame.
    if (unlikely(StackMgr.getFrameDepth() >=
                 Conf.getRuntimeConfigure().getMaxCallDepth())) {
      spdlog::error(ErrCode::Value::CallStackExhausted);
      return Unexpect(ErrCode::Value::CallStackExhausted);
    }

    // Push local variables into the stack.
    for (auto &Def : Func.getLocals()) {
      for (uint32_t I = 0; I < Def.first; I++) {
        StackMgr.push(ValueFromType(Def.second));
      }
    }

    // Push frame.
    // The PC must -1 here because in the interpreter mode execution, the PC
    // will increase after the callee return.
    StackMgr.pushFrame(Func.getModule(),           // Module instance
                       RetIt - 1,                  // Return PC
                       ArgsN + Func.getLocalNum(), // Arguments num + local num
                       RetsN,                      // Returns num
                       IsTailCall                  // For tail-call
    );

    // For native function case, the continuation will be the start of the
    // function body.
    return Func.getInstrs().begin();
  }
}

Expect<void>
Executor::branchToLabel(Runtime::StackManager &StackMgr,
                        const AST::Instruction::JumpDescriptor &JumpDesc,
                        AST::InstrView::iterator &PC) noexcept {
  // Check the stop token.
  if (unlikely(StopToken.exchange(0, std::memory_order_relaxed))) {
    spdlog::error(ErrCode::Value::Interrupted);
    return Unexpect(ErrCode::Value::Interrupted);
  }

  StackMgr.eraseValueStack(JumpDesc.StackEraseBegin, JumpDesc.StackEraseEnd);
  // PC need to -1 here because the PC will increase in the next iteration.
  PC += (JumpDesc.PCOffset - 1);
  return {};
}

Expect<void> Executor::throwException(Runtime::StackManager &StackMgr,
                                      Runtime::Instance::TagInstance &TagInst,
                                      AST::InstrView::iterator &PC) noexcept {
  StackMgr.removeInactiveHandler(PC);
  auto AssocValSize = TagInst.getTagType().getAssocValSize();
  while (true) {
    // Pop the top handler.
    auto Handler = StackMgr.popTopHandler(AssocValSize);
    if (!Handler.has_value()) {
      break;
    }
    // Checking through the catch clause.
    for (const auto &C : Handler->CatchClause) {
      if (!C.IsAll && getTagInstByIdx(StackMgr, C.TagIndex) != &TagInst) {
        // For catching a specific tag, should check the equivalence of tag
        // address.
        continue;
      }
      if (C.IsRef) {
        // For catching a exception reference, push the reference value onto
        // stack.
        StackMgr.push(
            RefVariant(ValType(TypeCode::Ref, TypeCode::ExnRef), &TagInst));
      }
      // When being here, an exception is caught. Move the PC to the try block
      // and branch to the label.

      PC = Handler->Try;
      return branchToLabel(StackMgr, C.Jump, PC);
    }
  }
  spdlog::error(ErrCode::Value::UncaughtException);
  return Unexpect(ErrCode::Value::UncaughtException);
}

const AST::SubType *Executor::getDefTypeByIdx(Runtime::StackManager &StackMgr,
                                              const uint32_t Idx) const {
  const auto *ModInst = StackMgr.getModule();
  // When top frame is dummy frame, cannot find instance.
  if (unlikely(ModInst == nullptr)) {
    return nullptr;
  }
  return ModInst->unsafeGetType(Idx);
}

const WasmEdge::AST::CompositeType &
Executor::getCompositeTypeByIdx(Runtime::StackManager &StackMgr,
                                const uint32_t Idx) const noexcept {
  auto *DefType = getDefTypeByIdx(StackMgr, Idx);
  assuming(DefType);
  const auto &CompType = DefType->getCompositeType();
  assuming(!CompType.isFunc());
  return CompType;
}

const ValType &
Executor::getStructStorageTypeByIdx(Runtime::StackManager &StackMgr,
                                    const uint32_t Idx,
                                    const uint32_t Off) const noexcept {
  const auto &CompType = getCompositeTypeByIdx(StackMgr, Idx);
  assuming(static_cast<uint32_t>(CompType.getFieldTypes().size()) > Off);
  return CompType.getFieldTypes()[Off].getStorageType();
}

const ValType &
Executor::getArrayStorageTypeByIdx(Runtime::StackManager &StackMgr,
                                   const uint32_t Idx) const noexcept {
  const auto &CompType = getCompositeTypeByIdx(StackMgr, Idx);
  assuming(static_cast<uint32_t>(CompType.getFieldTypes().size()) == 1);
  return CompType.getFieldTypes()[0].getStorageType();
}

Runtime::Instance::FunctionInstance *
Executor::getFuncInstByIdx(Runtime::StackManager &StackMgr,
                           const uint32_t Idx) const {
  const auto *ModInst = StackMgr.getModule();
  // When top frame is dummy frame, cannot find instance.
  if (unlikely(ModInst == nullptr)) {
    return nullptr;
  }
  return ModInst->unsafeGetFunction(Idx);
}

Runtime::Instance::TableInstance *
Executor::getTabInstByIdx(Runtime::StackManager &StackMgr,
                          const uint32_t Idx) const {
  const auto *ModInst = StackMgr.getModule();
  // When top frame is dummy frame, cannot find instance.
  if (unlikely(ModInst == nullptr)) {
    return nullptr;
  }
  return ModInst->unsafeGetTable(Idx);
}

Runtime::Instance::MemoryInstance *
Executor::getMemInstByIdx(Runtime::StackManager &StackMgr,
                          const uint32_t Idx) const {
  const auto *ModInst = StackMgr.getModule();
  // When top frame is dummy frame, cannot find instance.
  if (unlikely(ModInst == nullptr)) {
    return nullptr;
  }
  return ModInst->unsafeGetMemory(Idx);
}

Runtime::Instance::TagInstance *
Executor::getTagInstByIdx(Runtime::StackManager &StackMgr,
                          const uint32_t Idx) const {
  const auto *ModInst = StackMgr.getModule();
  // When top frame is dummy frame, cannot find instance.
  if (unlikely(ModInst == nullptr)) {
    return nullptr;
  }
  return ModInst->unsafeGetTag(Idx);
}

Runtime::Instance::GlobalInstance *
Executor::getGlobInstByIdx(Runtime::StackManager &StackMgr,
                           const uint32_t Idx) const {
  const auto *ModInst = StackMgr.getModule();
  // When top frame is dummy frame, cannot find instance.
  if (unlikely(ModInst == nullptr)) {
    return nullptr;
  }
  return ModInst->unsafeGetGlobal(Idx);
}

Runtime::Instance::ElementInstance *
Executor::getElemInstByIdx(Runtime::StackManager &StackMgr,
                           const uint32_t Idx) const {
  const auto *ModInst = StackMgr.getModule();
  // When top frame is dummy frame, cannot find instance.
  if (unlikely(ModInst == nullptr)) {
    return nullptr;
  }
  return ModInst->unsafeGetElem(Idx);
}

Runtime::Instance::DataInstance *
Executor::getDataInstByIdx(Runtime::StackManager &StackMgr,
                           const uint32_t Idx) const {
  const auto *ModInst = StackMgr.getModule();
  // When top frame is dummy frame, cannot find instance.
  if (unlikely(ModInst == nullptr)) {
    return nullptr;
  }
  return ModInst->unsafeGetData(Idx);
}

TypeCode Executor::toBottomType(Runtime::StackManager &StackMgr,
                                const ValType &Type) const {
  if (Type.isRefType()) {
    if (Type.isAbsHeapType()) {
      switch (Type.getHeapTypeCode()) {
      case TypeCode::NullFuncRef:
      case TypeCode::FuncRef:
        return TypeCode::NullFuncRef;
      case TypeCode::NullExternRef:
      case TypeCode::ExternRef:
        return TypeCode::NullExternRef;
      case TypeCode::NullRef:
      case TypeCode::AnyRef:
      case TypeCode::EqRef:
      case TypeCode::I31Ref:
      case TypeCode::StructRef:
      case TypeCode::ArrayRef:
        return TypeCode::NullRef;
      case TypeCode::NullExnRef:
      case TypeCode::ExnRef:
        return TypeCode::NullExnRef;
      default:
        assumingUnreachable();
      }
    } else {
      const auto &CompType =
          (*StackMgr.getModule()->getType(Type.getTypeIndex()))
              ->getCompositeType();
      if (CompType.isFunc()) {
        return TypeCode::NullFuncRef;
      } else {
        return TypeCode::NullRef;
      }
    }
  } else {
    return Type.getCode();
  }
}

void Executor::cleanNumericVal(ValVariant &Val,
                               const ValType &Type) const noexcept {
  if (Type.isNumType()) {
    switch (Type.getCode()) {
    case TypeCode::I32: {
      uint32_t V = Val.get<uint32_t>();
      Val.emplace<uint128_t>(static_cast<uint128_t>(0U));
      Val.emplace<uint32_t>(V);
      break;
    }
    case TypeCode::F32: {
      float V = Val.get<float>();
      Val.emplace<uint128_t>(static_cast<uint128_t>(0U));
      Val.emplace<float>(V);
      break;
    }
    case TypeCode::I64: {
      uint64_t V = Val.get<uint64_t>();
      Val.emplace<uint128_t>(static_cast<uint128_t>(0U));
      Val.emplace<uint64_t>(V);
      break;
    }
    case TypeCode::F64: {
      double V = Val.get<double>();
      Val.emplace<uint128_t>(static_cast<uint128_t>(0U));
      Val.emplace<double>(V);
      break;
    }
    default:
      break;
    }
  }
}

ValVariant Executor::packVal(const ValType &Type,
                             const ValVariant &Val) const noexcept {
  if (Type.isPackType()) {
    switch (Type.getCode()) {
    case TypeCode::I8:
      if constexpr (Endian::native == Endian::little) {
        return ValVariant(Val.get<uint32_t>() & 0xFFU);
      } else {
        return ValVariant(Val.get<uint32_t>() << 24);
      }
    case TypeCode::I16:
      if constexpr (Endian::native == Endian::little) {
        return ValVariant(Val.get<uint32_t>() & 0xFFFFU);
      } else {
        return ValVariant(Val.get<uint32_t>() << 16);
      }
    default:
      assumingUnreachable();
    }
  }
  return Val;
}

std::vector<ValVariant>
Executor::packVals(const ValType &Type,
                   std::vector<ValVariant> &&Vals) const noexcept {
  for (uint32_t I = 0; I < Vals.size(); I++) {
    Vals[I] = packVal(Type, Vals[I]);
  }
  return std::move(Vals);
}

ValVariant Executor::unpackVal(const ValType &Type, const ValVariant &Val,
                               bool IsSigned) const noexcept {
  if (Type.isPackType()) {
    uint32_t Num = Val.get<uint32_t>();
    switch (Type.getCode()) {
    case TypeCode::I8:
      if constexpr (Endian::native == Endian::big) {
        Num >>= 24;
      }
      if (IsSigned) {
        return static_cast<uint32_t>(static_cast<int8_t>(Num));
      } else {
        return static_cast<uint32_t>(static_cast<uint8_t>(Num));
      }
    case TypeCode::I16:
      if constexpr (Endian::native == Endian::big) {
        Num >>= 16;
      }
      if (IsSigned) {
        return static_cast<uint32_t>(static_cast<int16_t>(Num));
      } else {
        return static_cast<uint32_t>(static_cast<uint16_t>(Num));
      }
    default:
      assumingUnreachable();
    }
  }
  return Val;
}
} // namespace Executor
} // namespace WasmEdge
