// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"
#include "runtime/instance/gc.h"

#include <cstdint>
#include <cstring>

namespace WasmEdge {
namespace Executor {

Expect<void> Executor::runIfElseOp(Runtime::StackManager &StackMgr,
                                   const AST::Instruction &Instr,
                                   AST::InstrView::iterator &PC) noexcept {
  // Get condition.
  uint32_t Cond = StackMgr.pop<uint32_t>();

  // If non-zero, run if-statement; else, run else-statement.
  if (Cond == 0) {
    if (Instr.getJumpElse() == Instr.getJumpEnd()) {
      // No else-statement case. Jump to right before the End instruction.
      PC += (Instr.getJumpEnd() - 1);
    } else {
      if (Stat) {
        Stat->incInstrCount();
        if (unlikely(!Stat->addInstrCost(OpCode::Else))) {
          return Unexpect(ErrCode::Value::CostLimitExceeded);
        }
      }
      // Else-statement case. Jump to the Else instruction to continue.
      PC += Instr.getJumpElse();
    }
  }
  return {};
}

Expect<void> Executor::runThrowOp(Runtime::StackManager &StackMgr,
                                  const AST::Instruction &Instr,
                                  AST::InstrView::iterator &PC) noexcept {
  auto *TagInst = getTagInstByIdx(StackMgr, Instr.getTargetIndex());
  // The arguments will be popped from the stack in the throw function.
  return throwException(StackMgr, *TagInst, PC);
}

Expect<void> Executor::runThrowRefOp(Runtime::StackManager &StackMgr,
                                     const AST::Instruction &Instr,
                                     AST::InstrView::iterator &PC) noexcept {
  const auto Ref = StackMgr.pop<RefVariant>();
  if (Ref.isNull()) {
    spdlog::error(ErrCode::Value::AccessNullException);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::AccessNullException);
  }
  const auto *ExnInst = Ref.getPtr<Runtime::Instance::ExceptionInstance>();
  auto *TagInst = ExnInst->getTag();
  // Re-push the captured payload to mirror an initial `throw`, then unwind.
  StackMgr.pushSpan(ExnInst->getPayload());
  return throwException(StackMgr, *TagInst, PC, ExnInst);
}

Expect<void> Executor::runBrOp(Runtime::StackManager &StackMgr,
                               const AST::Instruction &Instr,
                               AST::InstrView::iterator &PC) noexcept {
  return branchToLabel(StackMgr, Instr.getJump(), PC);
}

Expect<void> Executor::runBrIfOp(Runtime::StackManager &StackMgr,
                                 const AST::Instruction &Instr,
                                 AST::InstrView::iterator &PC) noexcept {
  if (StackMgr.pop<uint32_t>() != 0) {
    return runBrOp(StackMgr, Instr, PC);
  }
  return {};
}

Expect<void> Executor::runBrOnNullOp(Runtime::StackManager &StackMgr,
                                     const AST::Instruction &Instr,
                                     AST::InstrView::iterator &PC) noexcept {
  if (StackMgr.peekTop<RefVariant>().isNull()) {
    StackMgr.pop<ValVariant>();
    return runBrOp(StackMgr, Instr, PC);
  }
  return {};
}

Expect<void> Executor::runBrOnNonNullOp(Runtime::StackManager &StackMgr,
                                        const AST::Instruction &Instr,
                                        AST::InstrView::iterator &PC) noexcept {
  if (!StackMgr.peekTop<RefVariant>().isNull()) {
    return runBrOp(StackMgr, Instr, PC);
  }
  StackMgr.pop<ValVariant>();
  return {};
}

Expect<void> Executor::runBrTableOp(Runtime::StackManager &StackMgr,
                                    const AST::Instruction &Instr,
                                    AST::InstrView::iterator &PC) noexcept {
  // Get the value on top of the stack.
  uint32_t Value = StackMgr.pop<uint32_t>();

  // Do branch.
  auto LabelTable = Instr.getLabelList();
  const auto LabelTableSize = static_cast<uint32_t>(LabelTable.size() - 1);
  if (Value < LabelTableSize) {
    return branchToLabel(StackMgr, LabelTable[Value], PC);
  }
  return branchToLabel(StackMgr, LabelTable[LabelTableSize], PC);
}

Expect<void> Executor::runBrOnCastOp(Runtime::StackManager &StackMgr,
                                     const AST::Instruction &Instr,
                                     AST::InstrView::iterator &PC,
                                     bool IsReverse) noexcept {
  // Get the value on top of the stack.
  const auto *ModInst = StackMgr.getModule();
  const auto Val = StackMgr.peekTop<RefVariant>();
  const auto &VT = Val.getType();
  Span<const AST::SubType *const> GotTypeList = ModInst->getTypeList();
  if (!VT.isAbsHeapType()) {
    // A concrete heap type is either a GC struct/array (payload is
    // GCInstance::RawData) or a typed function reference (payload is a
    // CompositeBase / FunctionInstance). Both layouts begin with the defining
    // `const ModuleInstance *` at offset 0, so copy that leading word out
    // directly instead of reinterpreting the payload as one concrete type
    // (which would be UB for the other; this mirrors proxyRefTest /
    // runRefTestOp). The reference must not be nullptr because null references
    // carry the least abstract heap type.
    const Runtime::Instance::ModuleInstance *RefMod = nullptr;
    static_assert(offsetof(Runtime::Instance::GCInstance::RawData, ModInst) ==
                      0,
                  "RawData must begin with the ModInst pointer");
    assuming(Val.getPtr<void>() != nullptr);
    std::memcpy(&RefMod, Val.getPtr<void>(), sizeof(RefMod));
    if (RefMod) {
      GotTypeList = RefMod->getTypeList();
    }
  }

  ValType NormalizedVT = VT;
  if (NormalizedVT.isExternalized()) {
    // An externalized reference must appear as an 'externref' to the matcher.
    // We preserve the nullability (Ref vs RefNull).
    NormalizedVT =
        ValType(VT.isNullableRefType() ? TypeCode::RefNull : TypeCode::Ref,
                TypeCode::ExternRef);
  }

  bool MatchResult = AST::TypeMatcher::matchType(ModInst->getTypeList(),
                                                 Instr.getBrCast().RType2,
                                                 GotTypeList, NormalizedVT);
  if (MatchResult != IsReverse) {
    return branchToLabel(StackMgr, Instr.getBrCast().Jump, PC);
  }
  return {};
}

Expect<void> Executor::runReturnOp(Runtime::StackManager &StackMgr,
                                   AST::InstrView::iterator &PC) noexcept {
  // Check stop token
  if (unlikely(StopToken.exchange(0, std::memory_order_relaxed))) {
    spdlog::error(ErrCode::Value::Interrupted);
    return Unexpect(ErrCode::Value::Interrupted);
  }
  PC = StackMgr.popFrame();
  return {};
}

Expect<void> Executor::runCallOp(Runtime::StackManager &StackMgr,
                                 const AST::Instruction &Instr,
                                 AST::InstrView::iterator &PC,
                                 bool IsTailCall) noexcept {
  // Get Function address.
  const auto *FuncInst = getFuncInstByIdx(StackMgr, Instr.getTargetIndex());
  EXPECTED_TRY(auto NextPC,
               enterFunction(StackMgr, *FuncInst, PC + 1, IsTailCall));
  PC = NextPC - 1;
  return {};
}

Expect<void> Executor::runCallRefOp(Runtime::StackManager &StackMgr,
                                    const AST::Instruction &Instr,
                                    AST::InstrView::iterator &PC,
                                    bool IsTailCall) noexcept {
  const auto Ref = StackMgr.pop<RefVariant>();
  const auto *FuncInst = retrieveFuncRef(Ref);
  if (FuncInst == nullptr) {
    spdlog::error(ErrCode::Value::AccessNullFunc);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::AccessNullFunc);
  }

  // Get Function address.
  EXPECTED_TRY(auto NextPC,
               enterFunction(StackMgr, *FuncInst, PC + 1, IsTailCall));
  PC = NextPC - 1;
  return {};
}

Expect<void> Executor::runCallIndirectOp(Runtime::StackManager &StackMgr,
                                         const AST::Instruction &Instr,
                                         AST::InstrView::iterator &PC,
                                         bool IsTailCall) noexcept {
  // Get Table Instance.
  const auto *TabInst = getTabInstByIdx(StackMgr, Instr.getSourceIndex());

  // Get function type at index x.
  const auto *ModInst = StackMgr.getModule();
  const auto &ExpDefType = **ModInst->getType(Instr.getTargetIndex());

  // Pop the value of index from the Stack.
  const auto AddrType = TabInst->getTableType().getLimit().getAddrType();
  uint64_t Idx = extractAddr(StackMgr.pop<ValVariant>(), AddrType);

  // If idx not small than tab.elem, trap.
  if (Idx >= TabInst->getSize()) {
    spdlog::error(ErrCode::Value::UndefinedElement);
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Idx},
                                           {ValTypeFromType<uint32_t>()}));
    return Unexpect(ErrCode::Value::UndefinedElement);
  }

  // Get function address. The bound is guaranteed.
  RefVariant Ref = *TabInst->getRefAddr(Idx);
  const auto *FuncInst = retrieveFuncRef(Ref);
  if (FuncInst == nullptr) {
    spdlog::error(ErrCode::Value::UninitializedElement);
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Idx},
                                           {ValTypeFromType<uint32_t>()}));
    return Unexpect(ErrCode::Value::UninitializedElement);
  }

  // Check function type.
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
    auto &ExpFuncType = ExpDefType.getCompositeType().getFuncType();
    auto &GotFuncType = FuncInst->getFuncType();
    spdlog::error(ErrCode::Value::IndirectCallTypeMismatch);
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Idx},
                                           {ValTypeFromType<uint32_t>()}));
    spdlog::error(ErrInfo::InfoMismatch(
        ExpFuncType.getParamTypes(), ExpFuncType.getReturnTypes(),
        GotFuncType.getParamTypes(), GotFuncType.getReturnTypes()));
    return Unexpect(ErrCode::Value::IndirectCallTypeMismatch);
  }

  // Enter the function.
  EXPECTED_TRY(auto NextPC,
               enterFunction(StackMgr, *FuncInst, PC + 1, IsTailCall));
  PC = NextPC - 1;
  return {};
}

Expect<void> Executor::runTryTableOp(Runtime::StackManager &StackMgr,
                                     const AST::Instruction &Instr,
                                     AST::InstrView::iterator &PC) noexcept {
  const auto &TryDesc = Instr.getTryCatch();
  StackMgr.pushHandler(PC, TryDesc.BlockParamNum, TryDesc.Catch);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
