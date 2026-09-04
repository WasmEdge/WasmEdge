// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- component_canon_async.cpp - async canonical built-in functions ----===//
//
// Runtime behavior of the async canonical built-ins over the task runtime.
//
//===----------------------------------------------------------------------===//

#include "executor/component/canonical_abi.h"
#include "runtime/component/taskmgr.h"

#include "common/errcode.h"
#include "common/spdlog.h"
#include "executor/component/executor.h"
#include "executor/executor.h"

#include <algorithm>

namespace WasmEdge {
namespace Executor {
namespace Component {

using namespace std::literals;
using Runtime::Instance::ComponentInstance;
namespace InstComp = Runtime::Instance::Component;
namespace RtComp = Runtime::Component;

namespace {

void setFuncType(AST::FunctionType &FT, std::initializer_list<ValType> Params,
                 std::initializer_list<ValType> Results) noexcept {
  for (const auto &P : Params) {
    FT.getParamTypes().push_back(P);
  }
  for (const auto &R : Results) {
    FT.getReturnTypes().push_back(R);
  }
}

Expect<void> trapMsg(ErrCode::Value Code, std::string_view Msg) noexcept {
  spdlog::error(Code);
  spdlog::error("    {}"sv, Msg);
  return Unexpect(Code);
}

Expect<void> trapCannotLeave() noexcept {
  return trapMsg(ErrCode::Value::ComponentCannotLeave,
                 "cannot leave component instance"sv);
}

Expect<void> trapUnknownHandle(uint32_t Idx) noexcept {
  spdlog::error(ErrCode::Value::ComponentHandleUnknown);
  spdlog::error("    unknown handle index {}"sv, Idx);
  return Unexpect(ErrCode::Value::ComponentHandleUnknown);
}

Expect<void> trapCannotBlock() noexcept {
  return trapMsg(ErrCode::Value::ComponentCannotBlockSync,
                 "cannot block a synchronous task before returning"sv);
}

// Structural equality of two component value types across instances.
bool valTypeEq(const Runtime::Instance::ComponentInstance *AInst,
               const ComponentValType &A,
               const Runtime::Instance::ComponentInstance *BInst,
               const ComponentValType &B, uint32_t Depth = 0) noexcept;

bool defValTypeEq(const Runtime::Instance::ComponentInstance *AInst,
                  const AST::Component::DefValType &A,
                  const Runtime::Instance::ComponentInstance *BInst,
                  const AST::Component::DefValType &B,
                  uint32_t Depth) noexcept {
  if (A.isPrimValType() && B.isPrimValType()) {
    return A.getPrimValType() == B.getPrimValType();
  }
  if (A.isRecordTy() && B.isRecordTy()) {
    const auto &RA = A.getRecord().LabelTypes;
    const auto &RB = B.getRecord().LabelTypes;
    if (RA.size() != RB.size()) {
      return false;
    }
    for (size_t I = 0; I < RA.size(); ++I) {
      if (RA[I].getLabel() != RB[I].getLabel() ||
          !valTypeEq(AInst, RA[I].getValType(), BInst, RB[I].getValType(),
                     Depth + 1)) {
        return false;
      }
    }
    return true;
  }
  if (A.isVariantTy() && B.isVariantTy()) {
    const auto &VA = A.getVariant().Cases;
    const auto &VB = B.getVariant().Cases;
    if (VA.size() != VB.size()) {
      return false;
    }
    for (size_t I = 0; I < VA.size(); ++I) {
      if (VA[I].first != VB[I].first ||
          VA[I].second.has_value() != VB[I].second.has_value()) {
        return false;
      }
      if (VA[I].second.has_value() &&
          !valTypeEq(AInst, *VA[I].second, BInst, *VB[I].second, Depth + 1)) {
        return false;
      }
    }
    return true;
  }
  if (A.isListTy() && B.isListTy()) {
    return A.getList().Len == B.getList().Len &&
           valTypeEq(AInst, A.getList().ValTy, BInst, B.getList().ValTy,
                     Depth + 1);
  }
  if (A.isMapTy() && B.isMapTy()) {
    return valTypeEq(AInst, A.getMap().KeyTy, BInst, B.getMap().KeyTy,
                     Depth + 1) &&
           valTypeEq(AInst, A.getMap().ValTy, BInst, B.getMap().ValTy,
                     Depth + 1);
  }
  if (A.isTupleTy() && B.isTupleTy()) {
    const auto &TA = A.getTuple().Types;
    const auto &TB = B.getTuple().Types;
    if (TA.size() != TB.size()) {
      return false;
    }
    for (size_t I = 0; I < TA.size(); ++I) {
      if (!valTypeEq(AInst, TA[I], BInst, TB[I], Depth + 1)) {
        return false;
      }
    }
    return true;
  }
  if (A.isFlagsTy() && B.isFlagsTy()) {
    return A.getFlags().Labels == B.getFlags().Labels;
  }
  if (A.isEnumTy() && B.isEnumTy()) {
    return A.getEnum().Labels == B.getEnum().Labels;
  }
  if (A.isOptionTy() && B.isOptionTy()) {
    return valTypeEq(AInst, A.getOption().ValTy, BInst, B.getOption().ValTy,
                     Depth + 1);
  }
  if (A.isResultTy() && B.isResultTy()) {
    const auto &RA = A.getResult();
    const auto &RB = B.getResult();
    if (RA.ValTy.has_value() != RB.ValTy.has_value() ||
        RA.ErrTy.has_value() != RB.ErrTy.has_value()) {
      return false;
    }
    if (RA.ValTy.has_value() &&
        !valTypeEq(AInst, *RA.ValTy, BInst, *RB.ValTy, Depth + 1)) {
      return false;
    }
    if (RA.ErrTy.has_value() &&
        !valTypeEq(AInst, *RA.ErrTy, BInst, *RB.ErrTy, Depth + 1)) {
      return false;
    }
    return true;
  }
  if ((A.isOwnTy() && B.isOwnTy()) || (A.isBorrowTy() && B.isBorrowTy())) {
    const uint32_t IA = A.isOwnTy() ? A.getOwn().Idx : A.getBorrow().Idx;
    const uint32_t IB = B.isOwnTy() ? B.getOwn().Idx : B.getBorrow().Idx;
    return AInst != nullptr && BInst != nullptr &&
           AInst->getTypeResource(IA) == BInst->getTypeResource(IB);
  }
  if (A.isStreamTy() && B.isStreamTy()) {
    const auto &SA = A.getStream().ValTy;
    const auto &SB = B.getStream().ValTy;
    if (SA.has_value() != SB.has_value()) {
      return false;
    }
    return !SA.has_value() || valTypeEq(AInst, *SA, BInst, *SB, Depth + 1);
  }
  if (A.isFutureTy() && B.isFutureTy()) {
    const auto &FA = A.getFuture().ValTy;
    const auto &FB = B.getFuture().ValTy;
    if (FA.has_value() != FB.has_value()) {
      return false;
    }
    return !FA.has_value() || valTypeEq(AInst, *FA, BInst, *FB, Depth + 1);
  }
  return false;
}

bool valTypeEq(const Runtime::Instance::ComponentInstance *AInst,
               const ComponentValType &A,
               const Runtime::Instance::ComponentInstance *BInst,
               const ComponentValType &B, uint32_t Depth) noexcept {
  if (Depth > 100) {
    return false;
  }
  const bool AIdx = A.getCode() == ComponentTypeCode::TypeIndex;
  const bool BIdx = B.getCode() == ComponentTypeCode::TypeIndex;
  if (!AIdx && !BIdx) {
    return A.getCode() == B.getCode();
  }
  const auto *DA =
      AIdx && AInst != nullptr ? AInst->getType(A.getTypeIndex()) : nullptr;
  const auto *DB =
      BIdx && BInst != nullptr ? BInst->getType(B.getTypeIndex()) : nullptr;
  if (AIdx != BIdx) {
    // One side is a primitive: the other must resolve to the same prim.
    const auto *D = AIdx ? DA : DB;
    if (D == nullptr || !D->isDefValType() ||
        !D->getDefValType().isPrimValType()) {
      return false;
    }
    const auto P = D->getDefValType().getPrimValType();
    const auto C = AIdx ? B.getCode() : A.getCode();
    return static_cast<uint8_t>(P) == static_cast<uint8_t>(C);
  }
  if (AInst == BInst && A.getTypeIndex() == B.getTypeIndex()) {
    return true;
  }
  if (DA != nullptr && DA == DB) {
    return true;
  }
  if (DA == nullptr || DB == nullptr || !DA->isDefValType() ||
      !DB->isDefValType()) {
    return false;
  }
  return defValTypeEq(AInst, DA->getDefValType(), BInst, DB->getDefValType(),
                      Depth);
}

bool elemTypeEq(const Runtime::Instance::ComponentInstance *AInst,
                const std::optional<ComponentValType> &A,
                const Runtime::Instance::ComponentInstance *BInst,
                const std::optional<ComponentValType> &B) noexcept {
  if (A.has_value() != B.has_value()) {
    return false;
  }
  if (!A.has_value()) {
    return true;
  }
  return valTypeEq(AInst, *A, BInst, *B);
}

// True when the element type permits same-instance transfers.
bool noneOrNumberPrim(AST::Component::PrimValType P) noexcept {
  using PVT = AST::Component::PrimValType;
  switch (P) {
  case PVT::S8:
  case PVT::U8:
  case PVT::S16:
  case PVT::U16:
  case PVT::S32:
  case PVT::U32:
  case PVT::S64:
  case PVT::U64:
  case PVT::F32:
  case PVT::F64:
    return true;
  default:
    return false;
  }
}

bool noneOrNumberType(const Runtime::Instance::ComponentInstance *Inst,
                      const std::optional<ComponentValType> &T) noexcept {
  if (!T.has_value()) {
    return true;
  }
  if (T->getCode() == ComponentTypeCode::TypeIndex) {
    const auto *D =
        Inst != nullptr ? Inst->getType(T->getTypeIndex()) : nullptr;
    if (D == nullptr || !D->isDefValType() ||
        !D->getDefValType().isPrimValType()) {
      return false;
    }
    return noneOrNumberPrim(D->getDefValType().getPrimValType());
  }
  switch (T->getCode()) {
  case ComponentTypeCode::S8:
  case ComponentTypeCode::U8:
  case ComponentTypeCode::S16:
  case ComponentTypeCode::U16:
  case ComponentTypeCode::S32:
  case ComponentTypeCode::U32:
  case ComponentTypeCode::S64:
  case ComponentTypeCode::U64:
  case ComponentTypeCode::F32:
  case ComponentTypeCode::F64:
    return true;
  default:
    return false;
  }
}

// Build the lift/lower context for one side of a copy.
CanonicalABI::Context bufferCx(ComponentExecutor *Exec,
                               const InstComp::TransmitBuffer &B) noexcept {
  CanonicalABI::Context Cx{B.Opts, Exec};
  Cx.CrossComponent = true;
  if (B.ElemInst != nullptr && B.ElemInst != B.Opts.Inst) {
    const auto *EI = B.ElemInst;
    Cx.TypeResolver = [EI](uint32_t I) { return EI->getType(I); };
    Cx.ResourceResolver = [EI](uint32_t I) { return EI->getTypeResource(I); };
  }
  return Cx;
}

// Validate a guest buffer against its memory.
Expect<void> checkBuffer(ComponentExecutor *Exec,
                         const InstComp::TransmitBuffer &B) noexcept {
  constexpr uint32_t MaxLength = (1U << 28) - 1;
  if (B.Length > MaxLength) {
    return trapMsg(ErrCode::Value::ComponentStreamOpTooBig,
                   "stream read/write count too large"sv);
  }
  if (B.Elem.has_value() && B.Length > 0) {
    auto Cx = bufferCx(Exec, B);
    EXPECTED_TRY(auto Align, CanonicalABI::alignment(Cx, *B.Elem));
    EXPECTED_TRY(auto Size, CanonicalABI::elemSize(Cx, *B.Elem));
    if (Align != 0 && (B.Ptr % Align) != 0) {
      return trapMsg(ErrCode::Value::ComponentTrap,
                     "buffer pointer is not aligned"sv);
    }
    if (B.Opts.Mem == nullptr ||
        !B.Opts.Mem->checkAccessBound(B.Ptr, B.Length * Size)) {
      return trapMsg(ErrCode::Value::ComponentTrap,
                     "buffer region out of bounds of memory"sv);
    }
  }
  return {};
}

// Move N elements from Src into Dst, advancing both progress counters.
Expect<void> copyElements(ComponentExecutor *Exec,
                          InstComp::TransmitBuffer &Dst,
                          InstComp::TransmitBuffer &Src, uint32_t N) noexcept {
  if (!Src.Elem.has_value() || !Dst.Elem.has_value()) {
    Src.Progress += N;
    Dst.Progress += N;
    return {};
  }
  auto SrcCx = bufferCx(Exec, Src);
  auto DstCx = bufferCx(Exec, Dst);
  EXPECTED_TRY(auto SrcSize, CanonicalABI::elemSize(SrcCx, *Src.Elem));
  EXPECTED_TRY(auto DstSize, CanonicalABI::elemSize(DstCx, *Dst.Elem));
  // Read every element before writing, so same-memory copies act as memmove.
  std::vector<ComponentValVariant> Vals;
  Vals.reserve(N);
  for (uint32_t I = 0; I < N; ++I) {
    EXPECTED_TRY(auto V,
                 CanonicalABI::load(
                     SrcCx, Src.Ptr + (Src.Progress + I) * SrcSize, *Src.Elem));
    Vals.push_back(std::move(V));
  }
  for (uint32_t I = 0; I < N; ++I) {
    EXPECTED_TRY(CanonicalABI::store(DstCx, Vals[I], *Dst.Elem,
                                     Dst.Ptr + (Dst.Progress + I) * DstSize));
  }
  Src.Progress += N;
  Dst.Progress += N;
  return {};
}

InstComp::AsyncEventCode eventCodeOf(const InstComp::TransmitEnd &E) noexcept {
  switch (E.getKind()) {
  case InstComp::WaitableBase::Kind::StreamRead:
    return InstComp::AsyncEventCode::StreamRead;
  case InstComp::WaitableBase::Kind::StreamWrite:
    return InstComp::AsyncEventCode::StreamWrite;
  case InstComp::WaitableBase::Kind::FutureRead:
    return InstComp::AsyncEventCode::FutureRead;
  default:
    return InstComp::AsyncEventCode::FutureWrite;
  }
}

bool isFutureEnd(const InstComp::TransmitEnd &E) noexcept {
  return E.getKind() == InstComp::WaitableBase::Kind::FutureRead ||
         E.getKind() == InstComp::WaitableBase::Kind::FutureWrite;
}

// Queue the completion event; the state transition happens at collection.
void queueCopyEvent(InstComp::TransmitEnd *E, uint32_t Idx,
                    InstComp::TransmitResult Result,
                    bool ReclaimPending) noexcept {
  auto Shared = E->Shared;
  const bool Future = isFutureEnd(*E);
  E->setPendingEvent([E, Idx, Result, ReclaimPending, Shared,
                      Future]() -> InstComp::AsyncEvent {
    if (ReclaimPending && Shared->PendingEnd == E) {
      Shared->HasPending = false;
      Shared->PendingDone = false;
      Shared->PendingEnd = nullptr;
    }
    if (Future) {
      E->St = (Result == InstComp::TransmitResult::Dropped ||
               Result == InstComp::TransmitResult::Completed)
                  ? InstComp::TransmitEnd::State::Done
                  : InstComp::TransmitEnd::State::Idle;
    } else {
      E->St = Result == InstComp::TransmitResult::Dropped
                  ? InstComp::TransmitEnd::State::Done
                  : InstComp::TransmitEnd::State::Idle;
    }
    if (E->St == InstComp::TransmitEnd::State::Done) {
      E->DoneByDrop = Result == InstComp::TransmitResult::Dropped;
    }
    const uint32_t Payload =
        Future ? static_cast<uint32_t>(Result)
               : (static_cast<uint32_t>(Result) | (E->Buffer.Progress << 4));
    return {eventCodeOf(*E), Idx, Payload};
  });
}

} // namespace

CanonAsyncBuiltinHostFunc::CanonAsyncBuiltinHostFunc(
    ComponentExecutor *ExecIn, AsyncBuiltinInfo InfoIn) noexcept
    : HostFunctionBase(/*FuncCost=*/0), Exec(ExecIn), Info(std::move(InfoIn)) {
  const ValType I32V{TypeCode::I32};
  const ValType I64V{TypeCode::I64};
  auto &FT = DefType.getCompositeType().getFuncType();
  switch (Info.Code) {
  case ComponentCanonOpCode::Context__set:
    setFuncType(FT, {Info.ContextType}, {});
    break;
  case ComponentCanonOpCode::Context__get:
    setFuncType(FT, {}, {Info.ContextType});
    break;
  case ComponentCanonOpCode::Task__cancel:
  case ComponentCanonOpCode::Backpressure__inc:
  case ComponentCanonOpCode::Backpressure__dec:
    setFuncType(FT, {}, {});
    break;
  case ComponentCanonOpCode::Yield:
  case ComponentCanonOpCode::Waitable_set__new:
  case ComponentCanonOpCode::Thread__index:
    setFuncType(FT, {}, {I32V});
    break;
  case ComponentCanonOpCode::Stream__new:
  case ComponentCanonOpCode::Future__new:
    setFuncType(FT, {}, {I64V});
    break;
  case ComponentCanonOpCode::Stream__read:
  case ComponentCanonOpCode::Stream__write:
    setFuncType(FT, {I32V, I32V, I32V}, {I32V});
    break;
  case ComponentCanonOpCode::Future__read:
  case ComponentCanonOpCode::Future__write:
  case ComponentCanonOpCode::Waitable_set__wait:
  case ComponentCanonOpCode::Waitable_set__poll:
  case ComponentCanonOpCode::Error_context__new:
    setFuncType(FT, {I32V, I32V}, {I32V});
    break;
  case ComponentCanonOpCode::Stream__cancel_read:
  case ComponentCanonOpCode::Stream__cancel_write:
  case ComponentCanonOpCode::Future__cancel_read:
  case ComponentCanonOpCode::Future__cancel_write:
  case ComponentCanonOpCode::Subtask__cancel:
  case ComponentCanonOpCode::Thread__yield_then_resume:
  case ComponentCanonOpCode::Thread__suspend_then_resume:
  case ComponentCanonOpCode::Thread__yield_then_promote:
  case ComponentCanonOpCode::Thread__suspend_then_promote:
    setFuncType(FT, {I32V}, {I32V});
    break;
  case ComponentCanonOpCode::Thread__new_indirect:
    setFuncType(FT, {I32V, I32V}, {I32V});
    break;
  case ComponentCanonOpCode::Thread__resume_later:
    setFuncType(FT, {I32V}, {});
    break;
  case ComponentCanonOpCode::Thread__suspend:
    setFuncType(FT, {}, {I32V});
    break;
  case ComponentCanonOpCode::Waitable__join:
  case ComponentCanonOpCode::Error_context__debug_message:
    setFuncType(FT, {I32V, I32V}, {});
    break;
  case ComponentCanonOpCode::Task__return: {
    // Params = flatten of the declared result list.
    CanonicalABI::Context Cx{Info.Opts};
    std::vector<ValType> Flat;
    bool Indirect = false;
    for (const auto &T : Info.RetTypes) {
      auto Sub = CanonicalABI::flattenType(Cx, T);
      if (!Sub) {
        Indirect = true;
        break;
      }
      Flat.insert(Flat.end(), Sub->begin(), Sub->end());
    }
    if (Indirect || Flat.size() > CanonicalABI::MaxFlatParams) {
      Flat.clear();
      Flat.push_back(I32V);
    }
    for (const auto &P : Flat) {
      FT.getParamTypes().push_back(P);
    }
    break;
  }
  default:
    // stream/future drop, waitable-set.drop, subtask.drop, error-context.drop.
    setFuncType(FT, {I32V}, {});
    break;
  }
}

Expect<void> CanonAsyncBuiltinHostFunc::run(const Runtime::CallingFrame &,
                                            Span<const ValVariant> Args,
                                            Span<ValVariant> Rets) {
  auto &Rt = Exec->taskManager();
  const auto *Inst = Info.Opts.Inst;
  auto &Conc = Inst->concurrency();
  RtComp::Task *Tsk = Rt.currentTask();

  // Only built-ins that never leave the instance stay callable.
  switch (Info.Code) {
  case ComponentCanonOpCode::Backpressure__inc:
  case ComponentCanonOpCode::Backpressure__dec:
  case ComponentCanonOpCode::Context__get:
  case ComponentCanonOpCode::Context__set:
    break;
  default:
    if (!Conc.mayLeave()) {
      return trapCannotLeave();
    }
    break;
  }

  switch (Info.Code) {
  case ComponentCanonOpCode::Backpressure__inc:
    if (!Conc.incBackpressure()) {
      return trapMsg(ErrCode::Value::ComponentBackpressureOverflow,
                     "backpressure counter overflow"sv);
    }
    return {};
  case ComponentCanonOpCode::Backpressure__dec:
    if (!Conc.decBackpressure()) {
      return trapMsg(ErrCode::Value::ComponentBackpressureOverflow,
                     "backpressure counter underflow"sv);
    }
    return {};
  case ComponentCanonOpCode::Thread__index: {
    auto *Ctx = Rt.currentCtx();
    Rets[0] = Ctx != nullptr ? Ctx->Index : UINT32_C(0);
    return {};
  }
  case ComponentCanonOpCode::Context__get: {
    auto *Ctx = Rt.currentCtx();
    const uint64_t Slot =
        Ctx != nullptr ? Ctx->Storage[Info.ContextIdx & 1] : 0;
    if (Info.ContextType == ValType(TypeCode::I64)) {
      Rets[0] = Slot;
    } else {
      Rets[0] = static_cast<uint32_t>(Slot);
    }
    return {};
  }
  case ComponentCanonOpCode::Context__set: {
    if (auto *Ctx = Rt.currentCtx(); Ctx != nullptr) {
      Ctx->Storage[Info.ContextIdx & 1] =
          Info.ContextType == ValType(TypeCode::I64)
              ? Args[0].get<uint64_t>()
              : static_cast<uint64_t>(Args[0].get<uint32_t>());
    }
    return {};
  }

  case ComponentCanonOpCode::Yield: {
    if (Tsk == nullptr || !Exec->taskManager().mayBlock(*Tsk)) {
      // Yielding in a non-blocking context is a no-op.
      Rets[0] = UINT32_C(0);
      return {};
    }
    EXPECTED_TRY(auto Reason,
                 Exec->taskManager().taskWait(
                     *Tsk, []() { return true; }, Info.cancellable()));
    if (Reason == RtComp::ResumeReason::Abort) {
      return Unexpect(ErrCode::Value::ComponentAsyncAborted);
    }
    Rets[0] =
        Reason == RtComp::ResumeReason::Cancelled ? UINT32_C(1) : UINT32_C(0);
    return {};
  }

  case ComponentCanonOpCode::Task__return: {
    if (Tsk == nullptr || !Tsk->Opts.Async) {
      return trapMsg(ErrCode::Value::ComponentTaskReturnInvalid,
                     "invalid `task.return` signature and/or options for "
                     "current task"sv);
    }
    // The declared result types and lift options must equal the task's.
    const auto &TaskResults = Tsk->FT->getResultList();
    bool TypesMatch = TaskResults.size() == Info.RetTypes.size();
    if (TypesMatch) {
      for (size_t I = 0; I < Info.RetTypes.size(); ++I) {
        if (!valTypeEq(Inst, Info.RetTypes[I], Tsk->Opts.Inst,
                       TaskResults[I].getValType())) {
          TypesMatch = false;
          break;
        }
      }
    }
    if (!TypesMatch) {
      return trapMsg(ErrCode::Value::ComponentTaskReturnInvalid,
                     "invalid `task.return` signature and/or options for "
                     "current task"sv);
    }
    if ((Info.Opts.Mem != nullptr && Info.Opts.Mem != Tsk->Opts.Mem) ||
        Info.Opts.Enc != Tsk->Opts.Enc) {
      return trapMsg(ErrCode::Value::ComponentTaskReturnInvalid,
                     "invalid `task.return` signature and/or options for "
                     "current task"sv);
    }
    CanonicalABI::Context Cx{Info.Opts, Exec};
    CanonicalABI::FlatIter VI(Args);
    EXPECTED_TRY(auto Results,
                 CanonicalABI::liftFlatValues(Cx, VI, Info.RetTypes,
                                              CanonicalABI::MaxFlatParams));
    return Exec->taskManager().taskReturn(*Tsk, std::move(Results));
  }
  case ComponentCanonOpCode::Task__cancel: {
    if (Tsk == nullptr || !Tsk->Opts.Async) {
      return trapMsg(ErrCode::Value::ComponentTaskNotCancelled,
                     "`task.cancel` called by task which has not been "
                     "cancelled"sv);
    }
    return Exec->taskManager().taskCancel(*Tsk);
  }

  case ComponentCanonOpCode::Waitable_set__new:
    Rets[0] = Inst->handles().waitableSetAdd();
    return {};

  case ComponentCanonOpCode::Waitable_set__wait:
  case ComponentCanonOpCode::Waitable_set__poll: {
    const uint32_t SetIdx = Args[0].get<uint32_t>();
    const uint32_t Ptr = Args[1].get<uint32_t>();
    auto *WSet = Inst->handles().waitableSetGet(SetIdx);
    if (WSet == nullptr) {
      EXPECTED_TRY(trapUnknownHandle(SetIdx));
    }
    InstComp::AsyncEvent Ev;
    if (Info.Code == ComponentCanonOpCode::Waitable_set__wait) {
      if (Tsk == nullptr || !Exec->taskManager().mayBlock(*Tsk)) {
        return trapCannotBlock();
      }
      WSet->NumWaiting += 1;
      auto ReasonOrErr = Exec->taskManager().taskWait(
          *Tsk, [WSet]() { return WSet->hasPendingEvent(); },
          Info.cancellable(),
          /*AlwaysReleaseExcl=*/false, /*FastPath=*/true);
      EXPECTED_TRY(auto Reason, ReasonOrErr);
      if (Reason == RtComp::ResumeReason::Abort) {
        return Unexpect(ErrCode::Value::ComponentAsyncAborted);
      }
      WSet->NumWaiting -= 1;
      Ev = Reason == RtComp::ResumeReason::Cancelled
               ? InstComp::AsyncEvent{InstComp::AsyncEventCode::TaskCancelled,
                                      0, 0}
               : WSet->takePendingEvent();
    } else {
      if (Tsk != nullptr && Info.cancellable() &&
          Tsk->St == RtComp::Task::State::PendingCancel) {
        Tsk->St = RtComp::Task::State::CancelDelivered;
        Ev = {InstComp::AsyncEventCode::TaskCancelled, 0, 0};
      } else if (!WSet->hasPendingEvent()) {
        Ev = {InstComp::AsyncEventCode::None, 0, 0};
      } else {
        Ev = WSet->takePendingEvent();
      }
    }
    if (Info.Opts.Mem == nullptr || !Info.Opts.Mem->checkAccessBound(Ptr, 8)) {
      return trapMsg(ErrCode::Value::ComponentTrap,
                     "event payload out of bounds of memory"sv);
    }
    EXPECTED_TRY(Info.Opts.Mem->storeValue(Ev.P1, Ptr));
    EXPECTED_TRY(Info.Opts.Mem->storeValue(Ev.P2, Ptr + 4));
    Rets[0] = static_cast<uint32_t>(Ev.Code);
    return {};
  }

  case ComponentCanonOpCode::Waitable_set__drop: {
    const uint32_t SetIdx = Args[0].get<uint32_t>();
    auto *WSet = Inst->handles().waitableSetGet(SetIdx);
    if (WSet == nullptr) {
      EXPECTED_TRY(trapUnknownHandle(SetIdx));
    }
    if (!WSet->Elems.empty() || WSet->NumWaiting > 0) {
      return trapMsg(ErrCode::Value::ComponentWaitableSetNotEmpty,
                     "cannot drop waitable set with waiters"sv);
    }
    Inst->handles().waitableSetRemove(SetIdx);
    return {};
  }

  case ComponentCanonOpCode::Waitable__join: {
    const uint32_t WIdx = Args[0].get<uint32_t>();
    const uint32_t SetIdx = Args[1].get<uint32_t>();
    auto *W = Inst->handles().waitableGet(WIdx);
    if (W == nullptr) {
      EXPECTED_TRY(trapUnknownHandle(WIdx));
    }
    if (W->HasSyncWaiter) {
      return trapMsg(
          ErrCode::Value::ComponentWaitableInSetSyncUse,
          "waitable cannot be used synchronously while added to a waitable "
          "set"sv);
    }
    if (SetIdx == 0) {
      W->join(nullptr);
    } else {
      auto *WSet = Inst->handles().waitableSetGet(SetIdx);
      if (WSet == nullptr) {
        EXPECTED_TRY(trapUnknownHandle(SetIdx));
      }
      W->join(WSet);
    }
    return {};
  }

  case ComponentCanonOpCode::Subtask__cancel: {
    // A sync subtask.cancel in a task that cannot block traps first.
    if (!Info.async() &&
        (Tsk == nullptr || !Exec->taskManager().mayBlock(*Tsk))) {
      return trapCannotBlock();
    }
    const uint32_t Idx = Args[0].get<uint32_t>();
    auto *W = Inst->handles().waitableGet(Idx);
    if (W == nullptr || W->getKind() != InstComp::WaitableBase::Kind::Subtask) {
      EXPECTED_TRY(trapUnknownHandle(Idx));
    }
    auto *Sub = static_cast<InstComp::Subtask *>(W);
    if (Sub->resolveDelivered()) {
      return trapMsg(
          ErrCode::Value::ComponentSubtaskCancelTerminal,
          "`subtask.cancel` called after terminal status delivered"sv);
    }
    if (Sub->CancellationRequested) {
      return trapMsg(
          ErrCode::Value::ComponentSubtaskCancelTerminal,
          "`subtask.cancel` called after terminal status delivered"sv);
    }
    if (Sub->inWaitableSet() && !Info.async()) {
      return trapMsg(
          ErrCode::Value::ComponentWaitableInSetSyncUse,
          "waitable cannot be used synchronously while added to a waitable "
          "set"sv);
    }
    if (!Sub->resolved()) {
      Sub->CancellationRequested = true;
      if (Sub->OnCancel) {
        Sub->OnCancel();
      }
      if (!Sub->resolved()) {
        if (!Info.async()) {
          if (Tsk == nullptr || !Exec->taskManager().mayBlock(*Tsk)) {
            return trapCannotBlock();
          }
          W->HasSyncWaiter = true;
          auto ReasonOrErr = Exec->taskManager().taskWait(
              *Tsk, [W]() { return W->hasPendingEvent(); },
              /*Cancellable=*/false);
          EXPECTED_TRY(auto Reason, ReasonOrErr);
          if (Reason == RtComp::ResumeReason::Abort) {
            return Unexpect(ErrCode::Value::ComponentAsyncAborted);
          }
          W->HasSyncWaiter = false;
        } else {
          Rets[0] = InstComp::TransmitBlocked;
          return {};
        }
      }
    }
    if (W->hasPendingEvent()) {
      (void)W->takePendingEvent();
    }
    if (!Sub->Delivered) {
      for (const auto &[LInst, LIdx] : Sub->Lenders) {
        if (auto *Slot = LInst->handles().handleGet(LIdx);
            Slot != nullptr && Slot->Lends > 0) {
          Slot->Lends -= 1;
        }
      }
      Sub->Lenders.clear();
      Sub->Delivered = true;
    }
    Rets[0] = static_cast<uint32_t>(Sub->St);
    return {};
  }

  case ComponentCanonOpCode::Subtask__drop: {
    const uint32_t Idx = Args[0].get<uint32_t>();
    auto *W = Inst->handles().waitableGet(Idx);
    if (W == nullptr || W->getKind() != InstComp::WaitableBase::Kind::Subtask) {
      EXPECTED_TRY(trapUnknownHandle(Idx));
    }
    auto *Sub = static_cast<InstComp::Subtask *>(W);
    if (!Sub->resolveDelivered()) {
      return trapMsg(ErrCode::Value::ComponentSubtaskNotResolved,
                     "cannot drop a subtask which has not yet resolved"sv);
    }
    // The subtask can outlive its table slot; leave any waitable set now.
    W->join(nullptr);
    Inst->handles().waitableRemove(Idx);
    return {};
  }

  case ComponentCanonOpCode::Thread__new_indirect: {
    const uint32_t Fi = Args[0].get<uint32_t>();
    const uint32_t C = Args[1].get<uint32_t>();
    if (Info.Table == nullptr) {
      return trapMsg(ErrCode::Value::ComponentThreadStartInvalid,
                     "start function does not match expected type"sv);
    }
    EXPECTED_TRY(auto Ref, Info.Table->getRefAddr(Fi).map_error([](auto E) {
      spdlog::error("    thread.new-indirect index out of bounds"sv);
      return E;
    }));
    const auto *Fn = retrieveFuncRef(Ref);
    if (Fn == nullptr) {
      return trapMsg(
          ErrCode::Value::ComponentThreadStartInvalid,
          "the start function index points to an uninitialized function"sv);
    }
    const auto &FnTy = Fn->getFuncType();
    if (FnTy.getParamTypes() != std::vector<ValType>{ValType(TypeCode::I32)} ||
        !FnTy.getReturnTypes().empty()) {
      return trapMsg(ErrCode::Value::ComponentThreadStartInvalid,
                     "start function does not match expected type"sv);
    }
    auto *Ctx = Rt.newSpawnCtx();
    auto *FnMut = const_cast<Runtime::Instance::FunctionInstance *>(Fn);
    // The body captures the executor, not this host function.
    Rt.newSpawnThread(
        Tsk, Ctx,
        [Ex = Exec, FnMut, C, Ctx, Inst](RtComp::ResumeReason Reason) {
          if (Reason == RtComp::ResumeReason::Abort) {
            return;
          }
          std::array<ValVariant, 1> A{ValVariant(C)};
          std::array<ValType, 1> Ty{ValType(TypeCode::I32)};
          auto Res = Ex->core().invoke(FnMut, A, Ty);
          if (!Res &&
              Res.error().getEnum() != ErrCode::Value::ComponentAsyncAborted) {
            Ex->taskManager().noteTrap(Res.error(), Inst);
          }
          // Only a live instance is unregistered: a thread outlives its call.
          if (Ctx->Registered && !Ex->taskManager().aborting()) {
            Inst->concurrency().threadRemove(Ctx->Index);
          }
          Ctx->Registered = false;
        });
    Ctx->Index = Inst->concurrency().threadAdd(Ctx);
    Ctx->Registered = true;
    Rets[0] = Ctx->Index;
    return {};
  }
  case ComponentCanonOpCode::Thread__resume_later: {
    const uint32_t Idx = Args[0].get<uint32_t>();
    auto *Ctx = Inst->concurrency().threadGet(Idx);
    if (Ctx == nullptr) {
      EXPECTED_TRY(trapUnknownHandle(Idx));
    }
    auto *V = Ctx->Thread;
    const bool Suspended =
        V != nullptr ? (!V->ReadyFn && Rt.isParked(V)) : Ctx->Suspended;
    if (!Suspended) {
      return trapMsg(ErrCode::Value::ComponentThreadNotSuspended,
                     "cannot resume thread which is not suspended"sv);
    }
    if (V != nullptr) {
      V->ReadyFn = []() { return true; };
    } else {
      Ctx->Suspended = false;
    }
    return {};
  }
  case ComponentCanonOpCode::Thread__suspend: {
    if (Tsk == nullptr || !Exec->taskManager().mayBlock(*Tsk)) {
      return trapCannotBlock();
    }
    EXPECTED_TRY(auto Reason, Exec->taskManager().taskWait(*Tsk, nullptr,
                                                           Info.cancellable()));
    if (Reason == RtComp::ResumeReason::Abort) {
      return Unexpect(ErrCode::Value::ComponentAsyncAborted);
    }
    Rets[0] =
        Reason == RtComp::ResumeReason::Cancelled ? UINT32_C(1) : UINT32_C(0);
    return {};
  }
  case ComponentCanonOpCode::Thread__yield_then_resume:
  case ComponentCanonOpCode::Thread__suspend_then_resume: {
    const uint32_t Idx = Args[0].get<uint32_t>();
    auto *Ctx = Inst->concurrency().threadGet(Idx);
    if (Ctx == nullptr) {
      EXPECTED_TRY(trapUnknownHandle(Idx));
    }
    auto *V = Ctx->Thread;
    const bool Suspended =
        V != nullptr ? (!V->ReadyFn && Rt.isParked(V)) : Ctx->Suspended;
    if (!Suspended) {
      return trapMsg(ErrCode::Value::ComponentThreadNotSuspended,
                     "cannot resume thread which is not suspended"sv);
    }
    // The resumed thread becomes ready first, so a sync task may hand over.
    if (V != nullptr) {
      V->ReadyFn = []() { return true; };
    } else {
      Ctx->Suspended = false;
    }
    if (Tsk == nullptr || !Exec->taskManager().mayBlock(*Tsk)) {
      return trapCannotBlock();
    }
    const bool SuspendSelf =
        Info.Code == ComponentCanonOpCode::Thread__suspend_then_resume;
    EXPECTED_TRY(auto Reason,
                 Exec->taskManager().taskWait(
                     *Tsk,
                     SuspendSelf ? std::function<bool()>()
                                 : std::function<bool()>([]() { return true; }),
                     Info.cancellable()));
    if (Reason == RtComp::ResumeReason::Abort) {
      return Unexpect(ErrCode::Value::ComponentAsyncAborted);
    }
    Rets[0] =
        Reason == RtComp::ResumeReason::Cancelled ? UINT32_C(1) : UINT32_C(0);
    return {};
  }

  case ComponentCanonOpCode::Thread__yield_then_promote:
  case ComponentCanonOpCode::Thread__suspend_then_promote: {
    const uint32_t Idx = Args[0].get<uint32_t>();
    auto *Ctx = Inst->concurrency().threadGet(Idx);
    if (Ctx == nullptr) {
      EXPECTED_TRY(trapUnknownHandle(Idx));
    }
    // Hand over only to a ready waiter, else fall back to suspend / yield.
    auto *V = Ctx->Thread;
    const bool Waiting = V != nullptr && Rt.isParked(V);
    if (Waiting && V->ReadyFn && V->ReadyFn()) {
      V->ReadyFn = []() { return true; };
    } else if (V == nullptr && Ctx->Suspended) {
      Ctx->Suspended = false;
    }
    if (Tsk == nullptr || !Exec->taskManager().mayBlock(*Tsk)) {
      return trapCannotBlock();
    }
    const bool SuspendSelf =
        Info.Code == ComponentCanonOpCode::Thread__suspend_then_promote;
    EXPECTED_TRY(auto Reason,
                 Exec->taskManager().taskWait(
                     *Tsk,
                     SuspendSelf ? std::function<bool()>()
                                 : std::function<bool()>([]() { return true; }),
                     Info.cancellable()));
    if (Reason == RtComp::ResumeReason::Abort) {
      return Unexpect(ErrCode::Value::ComponentAsyncAborted);
    }
    Rets[0] =
        Reason == RtComp::ResumeReason::Cancelled ? UINT32_C(1) : UINT32_C(0);
    return {};
  }

  case ComponentCanonOpCode::Stream__new:
  case ComponentCanonOpCode::Future__new: {
    const bool IsStream = Info.Code == ComponentCanonOpCode::Stream__new;
    auto Shared = std::make_shared<InstComp::TransmitState>();
    Shared->IsStream = IsStream;
    Shared->ElemType = Info.Elem;
    Shared->ElemTypeInst = Inst;
    auto ReadEnd = std::make_shared<InstComp::TransmitEnd>(
        IsStream ? InstComp::WaitableBase::Kind::StreamRead
                 : InstComp::WaitableBase::Kind::FutureRead,
        Shared);
    auto WriteEnd = std::make_shared<InstComp::TransmitEnd>(
        IsStream ? InstComp::WaitableBase::Kind::StreamWrite
                 : InstComp::WaitableBase::Kind::FutureWrite,
        Shared);
    auto *ReadP = ReadEnd.get();
    auto *WriteP = WriteEnd.get();
    const uint32_t RIdx = Inst->handles().waitableAdd(std::move(ReadEnd));
    const uint32_t WIdx = Inst->handles().waitableAdd(std::move(WriteEnd));
    ReadP->TableIdx = RIdx;
    WriteP->TableIdx = WIdx;
    Rets[0] = (static_cast<uint64_t>(WIdx) << 32) | RIdx;
    return {};
  }

  case ComponentCanonOpCode::Stream__read:
  case ComponentCanonOpCode::Stream__write:
  case ComponentCanonOpCode::Future__read:
  case ComponentCanonOpCode::Future__write:
    return runCopy(Args, Rets);

  case ComponentCanonOpCode::Stream__cancel_read:
  case ComponentCanonOpCode::Stream__cancel_write:
  case ComponentCanonOpCode::Future__cancel_read:
  case ComponentCanonOpCode::Future__cancel_write:
    return runCancelCopy(Args, Rets);

  case ComponentCanonOpCode::Stream__drop_readable:
  case ComponentCanonOpCode::Stream__drop_writable:
  case ComponentCanonOpCode::Future__drop_readable:
  case ComponentCanonOpCode::Future__drop_writable:
    return runDropEnd(Args);

  case ComponentCanonOpCode::Error_context__new: {
    const uint32_t Ptr = Args[0].get<uint32_t>();
    const uint32_t Len = Args[1].get<uint32_t>();
    std::string Msg;
    if (Info.Opts.Mem != nullptr && Len > 0) {
      if (!Info.Opts.Mem->checkAccessBound(Ptr, Len)) {
        return trapMsg(ErrCode::Value::ComponentTrap,
                       "error-context message out of bounds"sv);
      }
      const auto SpanBytes = Info.Opts.Mem->getSpan<const char>(Ptr, Len);
      Msg.assign(SpanBytes.begin(), SpanBytes.end());
    }
    Rets[0] = Inst->handles().errorContextAdd(std::move(Msg));
    return {};
  }
  case ComponentCanonOpCode::Error_context__debug_message: {
    const uint32_t Idx = Args[0].get<uint32_t>();
    const uint32_t Ptr = Args[1].get<uint32_t>();
    auto *Ctx = Inst->handles().errorContextGet(Idx);
    if (Ctx == nullptr) {
      EXPECTED_TRY(trapUnknownHandle(Idx));
    }
    CanonicalABI::Context Cx{Info.Opts, Exec};
    return CanonicalABI::store(Cx, ComponentValVariant(*Ctx),
                               ComponentValType(ComponentTypeCode::String),
                               Ptr);
  }
  case ComponentCanonOpCode::Error_context__drop: {
    const uint32_t Idx = Args[0].get<uint32_t>();
    if (!Inst->handles().errorContextRemove(Idx)) {
      EXPECTED_TRY(trapUnknownHandle(Idx));
    }
    return {};
  }

  default:
    return trapMsg(ErrCode::Value::ComponentTrap,
                   "canonical built-in is not implemented"sv);
  }
}

Expect<void> CanonAsyncBuiltinHostFunc::runCopy(Span<const ValVariant> Args,
                                                Span<ValVariant> Rets) {
  const auto *Inst = Info.Opts.Inst;
  RtComp::Task *Tsk = Exec->taskManager().currentTask();
  const bool IsWrite = Info.Code == ComponentCanonOpCode::Stream__write ||
                       Info.Code == ComponentCanonOpCode::Future__write;
  const bool IsFuture = Info.Code == ComponentCanonOpCode::Future__read ||
                        Info.Code == ComponentCanonOpCode::Future__write;
  const auto WantKind =
      IsFuture ? (IsWrite ? InstComp::WaitableBase::Kind::FutureWrite
                          : InstComp::WaitableBase::Kind::FutureRead)
               : (IsWrite ? InstComp::WaitableBase::Kind::StreamWrite
                          : InstComp::WaitableBase::Kind::StreamRead);

  // A sync operation in a task that cannot block traps before the handle.
  if (!Info.async() &&
      (Tsk == nullptr || !Exec->taskManager().mayBlock(*Tsk))) {
    return trapCannotBlock();
  }

  const uint32_t Idx = Args[0].get<uint32_t>();
  const uint32_t Ptr = Args[1].get<uint32_t>();
  const uint32_t Len = IsFuture ? 1 : Args[2].get<uint32_t>();

  auto *W = Inst->handles().waitableGet(Idx);
  if (W == nullptr || W->getKind() != WantKind) {
    EXPECTED_TRY(trapUnknownHandle(Idx));
  }
  auto *E = static_cast<InstComp::TransmitEnd *>(W);
  auto Shared = E->Shared;
  if (!elemTypeEq(Inst, Info.Elem, Shared->ElemTypeInst, Shared->ElemType)) {
    return trapMsg(ErrCode::Value::ComponentHandleWrongType,
                   "stream or future element type mismatch"sv);
  }
  if (E->St == InstComp::TransmitEnd::State::Done) {
    if (IsFuture) {
      if (IsWrite) {
        return E->DoneByDrop
                   ? trapMsg(
                         ErrCode::Value::ComponentFutureWriteAfterSuccessOrDrop,
                         "cannot write to future after previous write "
                         "succeeded or readable end dropped"sv)
                   : trapMsg(ErrCode::Value::ComponentFutureWriteAfterSuccess,
                             "cannot write to future after previous write "
                             "succeeded"sv);
      }
      return trapMsg(ErrCode::Value::ComponentFutureReadAfterSuccess,
                     "cannot read from future after previous read "
                     "succeeded"sv);
    }
    if (IsWrite) {
      return trapMsg(ErrCode::Value::ComponentStreamWriteAfterDrop,
                     "cannot write to stream after being notified that the "
                     "readable end dropped"sv);
    }
    return trapMsg(ErrCode::Value::ComponentStreamReadAfterDrop,
                   "cannot read from stream after being notified that the "
                   "writable end dropped"sv);
  }
  if (E->copying()) {
    return trapMsg(
        ErrCode::Value::ComponentCopyBusy,
        "cannot have concurrent operations active on a future/stream"sv);
  }
  if (E->inWaitableSet() && !Info.async()) {
    return trapMsg(
        ErrCode::Value::ComponentWaitableInSetSyncUse,
        "waitable cannot be used synchronously while added to a waitable "
        "set"sv);
  }

  // Build and validate this side's buffer.
  E->Buffer = InstComp::TransmitBuffer{
      {Inst, Info.Opts.Mem, Info.Opts.Realloc, nullptr, nullptr, Info.Opts.Enc},
      Info.Elem,
      Inst,
      Ptr,
      Len,
      0};
  EXPECTED_TRY(checkBuffer(Exec, E->Buffer));

  E->St = InstComp::TransmitEnd::State::Copying;

  if (Shared->Dropped) {
    // The peer end is gone: the copy resolves immediately as dropped.
    queueCopyEvent(E, Idx, InstComp::TransmitResult::Dropped, false);
  } else if (!Shared->HasPending || Shared->PendingDone) {
    // Park this side; an exhausted uncollected side no longer joins.
    Shared->HasPending = true;
    Shared->PendingDone = false;
    Shared->PendingEnd = E;
  } else {
    auto *Peer = Shared->PendingEnd;
    const uint32_t PeerTableIdx = Peer->TableIdx;
    if (Peer->Buffer.Opts.Inst == Inst && !noneOrNumberType(Inst, Info.Elem)) {
      return trapMsg(ErrCode::Value::ComponentIntraCopy,
                     "cannot read from and write to intra-component "
                     "future"sv);
    }
    // Src is the writer's buffer, Dst the reader's; E is the current end.
    auto &Src = IsWrite ? E->Buffer : Peer->Buffer;
    auto &Dst = IsWrite ? Peer->Buffer : E->Buffer;
    // Mark the parked side finished; its Completed event waits for collection.
    auto CompletePeer = [&]() {
      Shared->PendingDone = true;
      queueCopyEvent(Peer, PeerTableIdx, InstComp::TransmitResult::Completed,
                     true);
    };
    // Transfer through the parked buffer until it fills up.
    auto TransferToPeer = [&]() -> Expect<void> {
      if (E->Buffer.remain() > 0) {
        const uint32_t N = std::min(E->Buffer.remain(), Peer->Buffer.remain());
        EXPECTED_TRY(copyElements(Exec, Dst, Src, N));
        queueCopyEvent(Peer, PeerTableIdx, InstComp::TransmitResult::Completed,
                       true);
        if (Peer->Buffer.remain() == 0) {
          Shared->PendingDone = true;
        }
      }
      queueCopyEvent(E, Idx, InstComp::TransmitResult::Completed, false);
      return {};
    };
    if (IsFuture) {
      // Single-element rendezvous: move the value and clear the pending slot.
      EXPECTED_TRY(copyElements(Exec, Dst, Src, 1));
      Shared->HasPending = false;
      Shared->PendingEnd = nullptr;
      queueCopyEvent(Peer, PeerTableIdx, InstComp::TransmitResult::Completed,
                     false);
      queueCopyEvent(E, Idx, InstComp::TransmitResult::Completed, false);
    } else if (IsWrite) {
      // Writer active, reader parked.
      if (Peer->Buffer.remain() > 0) {
        EXPECTED_TRY(TransferToPeer());
      } else if (E->Buffer.zeroLength() && Peer->Buffer.zeroLength()) {
        // Both zero-length: the writer completes, the reader stays pending.
        queueCopyEvent(E, Idx, InstComp::TransmitResult::Completed, false);
      } else {
        // The reader's buffer is full or zero: complete it and park the writer.
        CompletePeer();
        Shared->PendingEnd = E;
        Shared->PendingDone = false;
      }
    } else {
      // Reader active, writer parked.
      if (Peer->Buffer.remain() > 0) {
        EXPECTED_TRY(TransferToPeer());
      } else {
        // The pending writer is zero-length: complete it and park the reader.
        CompletePeer();
        Shared->PendingEnd = E;
        Shared->PendingDone = false;
      }
    }
  }

  if (!E->hasPendingEvent()) {
    if (!Info.async()) {
      if (Tsk == nullptr || !Exec->taskManager().mayBlock(*Tsk)) {
        return trapCannotBlock();
      }
      E->HasSyncWaiter = true;
      auto ReasonOrErr = Exec->taskManager().taskWait(
          *Tsk, [E]() { return E->hasPendingEvent(); }, /*Cancellable=*/false,
          /*AlwaysReleaseExcl=*/false, /*FastPath=*/true);
      EXPECTED_TRY(auto Reason, ReasonOrErr);
      if (Reason == RtComp::ResumeReason::Abort) {
        return Unexpect(ErrCode::Value::ComponentAsyncAborted);
      }
      E->HasSyncWaiter = false;
    } else {
      Rets[0] = InstComp::TransmitBlocked;
      return {};
    }
  }
  const auto Ev = E->takePendingEvent();
  Rets[0] = Ev.P2;
  return {};
}

Expect<void>
CanonAsyncBuiltinHostFunc::runCancelCopy(Span<const ValVariant> Args,
                                         Span<ValVariant> Rets) {
  const auto *Inst = Info.Opts.Inst;
  RtComp::Task *Tsk = Exec->taskManager().currentTask();
  const bool IsWrite =
      Info.Code == ComponentCanonOpCode::Stream__cancel_write ||
      Info.Code == ComponentCanonOpCode::Future__cancel_write;
  const bool IsFuture =
      Info.Code == ComponentCanonOpCode::Future__cancel_read ||
      Info.Code == ComponentCanonOpCode::Future__cancel_write;
  const auto WantKind =
      IsFuture ? (IsWrite ? InstComp::WaitableBase::Kind::FutureWrite
                          : InstComp::WaitableBase::Kind::FutureRead)
               : (IsWrite ? InstComp::WaitableBase::Kind::StreamWrite
                          : InstComp::WaitableBase::Kind::StreamRead);
  // A sync cancel in a task that cannot block traps before the handle.
  if (!Info.async() &&
      (Tsk == nullptr || !Exec->taskManager().mayBlock(*Tsk))) {
    return trapCannotBlock();
  }
  const uint32_t Idx = Args[0].get<uint32_t>();
  auto *W = Inst->handles().waitableGet(Idx);
  if (W == nullptr || W->getKind() != WantKind) {
    EXPECTED_TRY(trapUnknownHandle(Idx));
  }
  auto *E = static_cast<InstComp::TransmitEnd *>(W);
  if (E->St != InstComp::TransmitEnd::State::Copying || E->HasSyncWaiter) {
    return IsWrite
               ? trapMsg(ErrCode::Value::ComponentCancelWriteNotPending,
                         "stream or future write cancelled when no write is "
                         "pending"sv)
               : trapMsg(ErrCode::Value::ComponentCancelReadNotPending,
                         "stream or future read cancelled when no read is "
                         "pending"sv);
  }
  if (E->inWaitableSet() && !Info.async()) {
    return trapMsg(
        ErrCode::Value::ComponentWaitableInSetSyncUse,
        "waitable cannot be used synchronously while added to a waitable "
        "set"sv);
  }
  E->St = InstComp::TransmitEnd::State::CancellingCopy;
  auto Shared = E->Shared;
  if (!E->hasPendingEvent()) {
    // Cancel this side's parked rendezvous.
    if (Shared->HasPending && Shared->PendingEnd == E) {
      Shared->HasPending = false;
      Shared->PendingEnd = nullptr;
      queueCopyEvent(E, Idx, InstComp::TransmitResult::Cancelled, false);
    }
    if (!E->hasPendingEvent()) {
      if (!Info.async()) {
        if (Tsk == nullptr || !Exec->taskManager().mayBlock(*Tsk)) {
          return trapCannotBlock();
        }
        E->HasSyncWaiter = true;
        auto ReasonOrErr = Exec->taskManager().taskWait(
            *Tsk, [E]() { return E->hasPendingEvent(); },
            /*Cancellable=*/false);
        EXPECTED_TRY(auto Reason, ReasonOrErr);
        if (Reason == RtComp::ResumeReason::Abort) {
          return Unexpect(ErrCode::Value::ComponentAsyncAborted);
        }
        E->HasSyncWaiter = false;
      } else {
        Rets[0] = InstComp::TransmitBlocked;
        return {};
      }
    }
  }
  const auto Ev = E->takePendingEvent();
  uint32_t Payload = Ev.P2;
  // A cancelled stream copy reports its progress; a future keeps completion.
  if (!IsFuture &&
      (Payload & 0xFU) ==
          static_cast<uint32_t>(InstComp::TransmitResult::Completed)) {
    Payload = (Payload & ~0xFU) |
              static_cast<uint32_t>(InstComp::TransmitResult::Cancelled);
  }
  Rets[0] = Payload;
  return {};
}

Expect<void>
CanonAsyncBuiltinHostFunc::runDropEnd(Span<const ValVariant> Args) {
  const auto *Inst = Info.Opts.Inst;
  const bool IsWrite =
      Info.Code == ComponentCanonOpCode::Stream__drop_writable ||
      Info.Code == ComponentCanonOpCode::Future__drop_writable;
  const bool IsFuture =
      Info.Code == ComponentCanonOpCode::Future__drop_readable ||
      Info.Code == ComponentCanonOpCode::Future__drop_writable;
  const auto WantKind =
      IsFuture ? (IsWrite ? InstComp::WaitableBase::Kind::FutureWrite
                          : InstComp::WaitableBase::Kind::FutureRead)
               : (IsWrite ? InstComp::WaitableBase::Kind::StreamWrite
                          : InstComp::WaitableBase::Kind::StreamRead);
  const uint32_t Idx = Args[0].get<uint32_t>();
  auto *W = Inst->handles().waitableGet(Idx);
  if (W == nullptr || W->getKind() != WantKind) {
    EXPECTED_TRY(trapUnknownHandle(Idx));
  }
  auto *E = static_cast<InstComp::TransmitEnd *>(W);
  auto Shared = E->Shared;
  if (!elemTypeEq(Inst, Info.Elem, Shared->ElemTypeInst, Shared->ElemType)) {
    return trapMsg(ErrCode::Value::ComponentHandleWrongType,
                   "stream or future element type mismatch"sv);
  }
  if (E->copying()) {
    // The readable end reports "remove", the writable end "drop".
    return IsWrite ? trapMsg(ErrCode::Value::ComponentStreamDropBusy,
                             "cannot drop busy stream"sv)
                   : trapMsg(ErrCode::Value::ComponentStreamRemoveBusy,
                             "cannot remove busy stream"sv);
  }
  if (IsFuture && IsWrite && E->St != InstComp::TransmitEnd::State::Done) {
    return trapMsg(
        ErrCode::Value::ComponentFutureWriteEndNoValue,
        "cannot drop future write end without first writing a value"sv);
  }
  // Shared drop: notify the parked peer, if any.
  if (!Shared->Dropped) {
    Shared->Dropped = true;
    if (Shared->HasPending) {
      auto *Peer = Shared->PendingEnd;
      Shared->HasPending = false;
      Shared->PendingEnd = nullptr;
      queueCopyEvent(Peer, Peer->TableIdx, InstComp::TransmitResult::Dropped,
                     false);
    }
  }
  Inst->handles().waitableRemove(Idx);
  return {};
}

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
