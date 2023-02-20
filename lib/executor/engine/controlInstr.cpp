// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

Expect<void> Executor::runIfElseOp(Runtime::StackManager &StackMgr,
                                   const AST::Instruction &Instr,
                                   AST::InstrView::iterator &PC) noexcept {
  // Get condition.
  uint32_t Cond = StackMgr.pop().get<uint32_t>();

  // If non-zero, run if-statement; else, run else-statement.
  if (Cond == 0) {
    if (Instr.getJumpElse() == Instr.getJumpEnd()) {
      // No else-statement case. Jump to right before End instruction.
      PC += (Instr.getJumpEnd() - 1);
    } else {
      if (Stat) {
        Stat->incInstrCount();
        if (unlikely(!Stat->addInstrCost(OpCode::Else))) {
          return Unexpect(ErrCode::Value::CostLimitExceeded);
        }
      }
      // Have else-statement case. Jump to Else instruction to continue.
      PC += Instr.getJumpElse();
    }
  }
  return {};
}

Expect<void> Executor::runBrOp(Runtime::StackManager &StackMgr,
                               const AST::Instruction &Instr,
                               AST::InstrView::iterator &PC) noexcept {
  return branchToLabel(StackMgr, Instr.getJump().StackEraseBegin,
                       Instr.getJump().StackEraseEnd, Instr.getJump().PCOffset,
                       PC);
}

Expect<void> Executor::runBrIfOp(Runtime::StackManager &StackMgr,
                                 const AST::Instruction &Instr,
                                 AST::InstrView::iterator &PC) noexcept {
  if (StackMgr.pop().get<uint32_t>() != 0) {
    return runBrOp(StackMgr, Instr, PC);
  }
  return {};
}

Expect<void> Executor::runBrOnNull(Runtime::StackManager &StackMgr,
                                   const AST::Instruction &Instr,
                                   AST::InstrView::iterator &PC) noexcept {
  if (StackMgr.getTop().get<RefVariant>().isNull()) {
    StackMgr.pop();
    return runBrOp(StackMgr, Instr, PC);
  }
  return {};
}

Expect<void> Executor::runBrOnNonNull(Runtime::StackManager &StackMgr,
                                      const AST::Instruction &Instr,
                                      AST::InstrView::iterator &PC) noexcept {
  if (!StackMgr.getTop().get<RefVariant>().isNull()) {
    return runBrOp(StackMgr, Instr, PC);
  }
  StackMgr.pop();
  return {};
}

Expect<void> Executor::runBrTableOp(Runtime::StackManager &StackMgr,
                                    const AST::Instruction &Instr,
                                    AST::InstrView::iterator &PC) noexcept {
  // Get value on top of stack.
  uint32_t Value = StackMgr.pop().get<uint32_t>();

  // Do branch.
  auto LabelTable = Instr.getLabelList();
  const auto LabelTableSize = static_cast<uint32_t>(LabelTable.size() - 1);
  if (Value < LabelTableSize) {
    return branchToLabel(StackMgr, LabelTable[Value].StackEraseBegin,
                         LabelTable[Value].StackEraseEnd,
                         LabelTable[Value].PCOffset, PC);
  }
  return branchToLabel(StackMgr, LabelTable[LabelTableSize].StackEraseBegin,
                       LabelTable[LabelTableSize].StackEraseEnd,
                       LabelTable[LabelTableSize].PCOffset, PC);
}

bool Executor::canCast(Runtime::StackManager &StackMgr, const HeapType &HType,
                       bool AllowNull) const {
  const auto &Ref = StackMgr.getTop().get<RefVariant>();
  if (Ref.isNull()) {
    return AllowNull;
  }
  spdlog::error("cast cast to");
  spdlog::error(HType);
  switch (HType.getHTypeCode()) {
  case HeapTypeCode::Func:
    return true;
  case HeapTypeCode::NoFunc:
    return false;
  case HeapTypeCode::Extern:
    return true;
  case HeapTypeCode::NoExtern:
    return false;
  case HeapTypeCode::Any:
  case HeapTypeCode::Eq:
    return true;
  case HeapTypeCode::None:
    // The reference is non-null. Must not be none
    return false;
  case HeapTypeCode::I31: {
    const auto *HeapValue = Ref.asPtr<Runtime::Instance::HeapInstance>();
    return HeapValue->isI31();
  }
  case HeapTypeCode::Struct: {
    const auto *HeapValue = Ref.asPtr<Runtime::Instance::HeapInstance>();
    return HeapValue->isStruct();
  }

  case HeapTypeCode::Array: {
    const auto *HeapValue = Ref.asPtr<Runtime::Instance::HeapInstance>();
    return HeapValue->isArray();
  }

  case HeapTypeCode::Defined:
    spdlog::error("cast on defined type");
    const auto *HeapValue = Ref.asPtr<Runtime::Instance::HeapInstance>();
    const auto *ModInst = StackMgr.getModule();
    if (ModInst != HeapValue->getModInst()) {
      // Only type defined in the same module can be casted
      return false;
    }
    const auto TypeIdx = HeapValue->getTypeIdx();
    const auto TargetTypeIdx = HType.getDefinedTypeIdx();
    spdlog::error(TypeIdx);
    spdlog::error(TargetTypeIdx);
    if (TypeIdx == TargetTypeIdx) {
      return true;
    }
    auto CurTypeIdx = TypeIdx;
    spdlog::error(ModInst->Types[0].isType<AST::StructType>());
    spdlog::error(ModInst->Types[1].asStructType().getContent().size());
    while (!ModInst->Types[CurTypeIdx].getParentTypeIdx().empty()) {
      assuming(ModInst->Types[CurTypeIdx].getParentTypeIdx().size() == 1);
      CurTypeIdx = ModInst->Types[CurTypeIdx].getParentTypeIdx()[0];
      if (CurTypeIdx == TargetTypeIdx) {
        spdlog::error("find parent index");
        return true;
      }
    }
    spdlog::error("cannon find parent index");
    return false;
  }
}

Expect<void> Executor::runBrCastOp(Runtime::StackManager &StackMgr,
                                   const AST::Instruction &Instr,
                                   AST::InstrView::iterator &PC, bool AllowNull,
                                   bool IsFailed) noexcept {
  spdlog::error(OpCodeStr[Instr.getOpCode()]);
  spdlog::error(Instr.getJumpHeapType());
  if (IsFailed == !canCast(StackMgr, Instr.getJumpHeapType(), AllowNull)) {
    return runBrOp(StackMgr, Instr, PC);
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
  const auto *ModInst = StackMgr.getModule();
  const auto *FuncInst = *ModInst->getFunc(Instr.getTargetIndex());
  if (auto Res = enterFunction(StackMgr, *FuncInst, PC + 1, IsTailCall); !Res) {
    return Unexpect(Res);
  } else {
    PC = (*Res) - 1;
  }
  return {};
}

Expect<void> Executor::runCallRefOp(Runtime::StackManager &StackMgr,
                                    AST::InstrView::iterator &PC,
                                    bool IsTailCall) noexcept {

  const auto Ref = StackMgr.pop().get<RefVariant>();
  if (Ref.isNull()) {
    spdlog::error(ErrCode::Value::CastNullptrToNonNull);
    return Unexpect(ErrCode::Value::CastNullptrToNonNull);
  }

  // Get Function address.
  const auto *FuncInst = Ref.asPtr<Runtime::Instance::FunctionInstance>();
  if (auto Res = enterFunction(StackMgr, *FuncInst, PC + 1, IsTailCall); !Res) {
    return Unexpect(Res);
  } else {
    PC = (*Res) - 1;
  }
  return {};
}

Expect<void> Executor::runCallIndirectOp(Runtime::StackManager &StackMgr,
                                         const AST::Instruction &Instr,
                                         AST::InstrView::iterator &PC,
                                         bool IsTailCall) noexcept {
  // Get Table Instance
  const auto *TabInst = getTabInstByIdx(StackMgr, Instr.getSourceIndex());

  // Get function type at index x.
  const auto *ModInst = StackMgr.getModule();
  const auto *TargetFuncType = *ModInst->getFuncType(Instr.getTargetIndex());

  // Pop the value i32.const i from the Stack.
  uint32_t Idx = StackMgr.pop().get<uint32_t>();

  RefVariant Ref;
  if (auto Res = TabInst->getRefAddr(Idx)) {
    Ref = *Res;
  } else {
    return Unexpect(Res.error());
  }
  if (Ref.isNull()) {
    spdlog::error(ErrCode::Value::UninitializedElement);
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Idx},
                                           {ValTypeFromType<uint32_t>()}));
    return Unexpect(ErrCode::Value::UninitializedElement);
  }

  // Check function type.
  const auto *FuncInst = retrieveFuncRef(Ref);
  const auto &FuncType = FuncInst->getFuncType();
  if (*TargetFuncType != FuncType) {
    spdlog::error(ErrCode::Value::IndirectCallTypeMismatch);
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Idx},
                                           {ValTypeFromType<uint32_t>()}));
    spdlog::error(ErrInfo::InfoMismatch(
        TargetFuncType->getParamTypes(), TargetFuncType->getReturnTypes(),
        FuncType.getParamTypes(), FuncType.getReturnTypes()));
    return Unexpect(ErrCode::Value::IndirectCallTypeMismatch);
  }

  // Enter the function.
  if (auto Res = enterFunction(StackMgr, *FuncInst, PC + 1, IsTailCall); !Res) {
    return Unexpect(Res);
  } else {
    PC = (*Res) - 1;
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
