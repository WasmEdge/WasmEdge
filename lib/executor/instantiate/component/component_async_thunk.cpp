// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- component_async_thunk.cpp - async canonical built-in functions ----===//
//
// Runtime behavior of the async-model canonical built-ins over the task
// runtime (Concurrency.md / CanonicalABI.md): backpressure, context-local
// storage, task return/cancel, waitable sets, subtasks, stream/future
// rendezvous copies, and error contexts.
//
//===----------------------------------------------------------------------===//

#include "executor/component/async_runtime.h"
#include "executor/component/async_thunk.h"

#include "common/errcode.h"
#include "common/spdlog.h"
#include "executor/executor.h"

#include <algorithm>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;
using Runtime::Instance::ComponentInstance;
namespace AC = Runtime::Instance::Component;

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

// Structural equality of two component value types within (possibly
// different) instances' type-index spaces.
bool valTypeEq(const ComponentInstance *AInst, const ComponentValType &A,
               const ComponentInstance *BInst, const ComponentValType &B,
               uint32_t Depth = 0) noexcept;

bool defValTypeEq(const ComponentInstance *AInst,
                  const AST::Component::DefValType &A,
                  const ComponentInstance *BInst,
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

bool valTypeEq(const ComponentInstance *AInst, const ComponentValType &A,
               const ComponentInstance *BInst, const ComponentValType &B,
               uint32_t Depth) noexcept {
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

bool elemTypeEq(const ComponentInstance *AInst,
                const std::optional<ComponentValType> &A,
                const ComponentInstance *BInst,
                const std::optional<ComponentValType> &B) noexcept {
  if (A.has_value() != B.has_value()) {
    return false;
  }
  if (!A.has_value()) {
    return true;
  }
  return valTypeEq(AInst, *A, BInst, *B);
}

// True when the element type permits same-instance transfers (the spec's
// temporary `none_or_number_type` restriction).
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

bool noneOrNumberType(const ComponentInstance *Inst,
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
CanonicalABI::CanonCtx bufferCx(Executor *Exec,
                                const AC::GuestBufferDesc &B) noexcept {
  CanonicalABI::CanonCtx Cx{Exec, B.Mem, B.Realloc, B.Inst,
                            {},   {},    nullptr,   B.Enc};
  Cx.CrossComponent = true;
  if (B.ElemInst != nullptr && B.ElemInst != B.Inst) {
    const auto *EI = B.ElemInst;
    Cx.TypeResolver = [EI](uint32_t I) { return EI->getType(I); };
    Cx.ResourceResolver = [EI](uint32_t I) { return EI->getTypeResource(I); };
  }
  return Cx;
}

// Validate a guest buffer against its memory (spec BufferGuestImpl ctor).
Expect<void> checkBuffer(Executor *Exec,
                         const AC::GuestBufferDesc &B) noexcept {
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
    if (B.Mem == nullptr || !B.Mem->checkAccessBound(B.Ptr, B.Length * Size)) {
      return trapMsg(ErrCode::Value::ComponentTrap,
                     "buffer region out of bounds of memory"sv);
    }
  }
  return {};
}

// Move N elements from Src into Dst (lift from the source side, lower into
// the destination side). Advances both progress counters.
Expect<void> copyElements(Executor *Exec, AC::GuestBufferDesc &Dst,
                          AC::GuestBufferDesc &Src, uint32_t N) noexcept {
  if (!Src.Elem.has_value() || !Dst.Elem.has_value()) {
    Src.Progress += N;
    Dst.Progress += N;
    return {};
  }
  auto SrcCx = bufferCx(Exec, Src);
  auto DstCx = bufferCx(Exec, Dst);
  EXPECTED_TRY(auto SrcSize, CanonicalABI::elemSize(SrcCx, *Src.Elem));
  EXPECTED_TRY(auto DstSize, CanonicalABI::elemSize(DstCx, *Dst.Elem));
  // Read every element before writing any (spec BufferGuestImpl.read/write),
  // so overlapping same-memory transfers behave like memmove.
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

AC::AsyncEventCode eventCodeOf(const AC::CopyEndObj &E) noexcept {
  switch (E.getKind()) {
  case AC::WaitableObj::Kind::StreamRead:
    return AC::AsyncEventCode::StreamRead;
  case AC::WaitableObj::Kind::StreamWrite:
    return AC::AsyncEventCode::StreamWrite;
  case AC::WaitableObj::Kind::FutureRead:
    return AC::AsyncEventCode::FutureRead;
  default:
    return AC::AsyncEventCode::FutureWrite;
  }
}

bool isFutureEnd(const AC::CopyEndObj &E) noexcept {
  return E.getKind() == AC::WaitableObj::Kind::FutureRead ||
         E.getKind() == AC::WaitableObj::Kind::FutureWrite;
}

// Queue the completion event on a copy end. The state transition and (for
// the parked side) the release of the shared pending slot happen when the
// event is collected, mirroring the spec's lazily-evaluated event thunks.
void queueCopyEvent(AC::CopyEndObj *E, uint32_t Idx, AC::AsyncCopyResult Result,
                    bool ReclaimPending) noexcept {
  auto Shared = E->Shared;
  const bool Future = isFutureEnd(*E);
  E->setPendingEvent([E, Idx, Result, ReclaimPending, Shared,
                      Future]() -> AC::AsyncEvent {
    if (ReclaimPending && Shared->PendingEnd == E) {
      Shared->HasPending = false;
      Shared->PendingDone = false;
      Shared->PendingEnd = nullptr;
    }
    if (Future) {
      E->St = (Result == AC::AsyncCopyResult::Dropped ||
               Result == AC::AsyncCopyResult::Completed)
                  ? AC::CopyState::Done
                  : AC::CopyState::Idle;
    } else {
      E->St = Result == AC::AsyncCopyResult::Dropped ? AC::CopyState::Done
                                                     : AC::CopyState::Idle;
    }
    if (E->St == AC::CopyState::Done) {
      E->DoneByDrop = Result == AC::AsyncCopyResult::Dropped;
    }
    const uint32_t Payload =
        Future ? static_cast<uint32_t>(Result)
               : (static_cast<uint32_t>(Result) | (E->Buffer.Progress << 4));
    return {eventCodeOf(*E), Idx, Payload};
  });
}

} // namespace

CanonAsyncBuiltinHostFunc::CanonAsyncBuiltinHostFunc(
    Executor *ExecIn, AsyncBuiltinInfo InfoIn) noexcept
    : HostFunctionBase(/*FuncCost=*/0), Exec(ExecIn), Info(std::move(InfoIn)) {
  const ValType I32V{TypeCode::I32};
  const ValType I64V{TypeCode::I64};
  auto &FT = DefType.getCompositeType().getFuncType();
  switch (Info.Code) {
  case ComponentCanonOpCode::Backpressure__set:
  case ComponentCanonOpCode::Context__set:
    setFuncType(FT, {I32V}, {});
    break;
  case ComponentCanonOpCode::Task__cancel:
  case ComponentCanonOpCode::Backpressure__inc:
  case ComponentCanonOpCode::Backpressure__dec:
    setFuncType(FT, {}, {});
    break;
  case ComponentCanonOpCode::Context__get:
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
    // Params = flatten of the declared result list (spec canon_task_return
    // lifts with MAX_FLAT_PARAMS).
    CanonicalABI::CanonCtx Cx{nullptr, nullptr, nullptr, Info.Inst,
                              {},      {},      nullptr};
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
    // stream/future drop, waitable-set.drop, subtask.drop,
    // error-context.drop.
    setFuncType(FT, {I32V}, {});
    break;
  }
}

Expect<void> CanonAsyncBuiltinHostFunc::run(const Runtime::CallingFrame &,
                                            Span<const ValVariant> Args,
                                            Span<ValVariant> Rets) {
  auto &Rt = Exec->getComponentAsyncRuntime();
  const auto *Inst = Info.Inst;
  auto &Conc = Inst->getConcurrency();
  ComponentTask *Task = Rt.currentTask();

  // may_leave gate. Built-ins that never leave the instance stay callable
  // inside the may-not-leave regions: backpressure.inc/dec, context.get/set,
  // and thread.index.
  switch (Info.Code) {
  case ComponentCanonOpCode::Backpressure__inc:
  case ComponentCanonOpCode::Backpressure__dec:
  case ComponentCanonOpCode::Context__get:
  case ComponentCanonOpCode::Context__set:
  case ComponentCanonOpCode::Thread__index:
    break;
  default:
    if (!Conc.MayLeave) {
      return trapCannotLeave();
    }
    break;
  }

  switch (Info.Code) {
  case ComponentCanonOpCode::Backpressure__set:
    Conc.Backpressure = Args[0].get<uint32_t>() != 0 ? 1 : 0;
    return {};
  case ComponentCanonOpCode::Backpressure__inc:
    Conc.Backpressure += 1;
    if (Conc.Backpressure == (INT64_C(1) << 16)) {
      return trapMsg(ErrCode::Value::ComponentBackpressureOverflow,
                     "backpressure counter overflow"sv);
    }
    return {};
  case ComponentCanonOpCode::Backpressure__dec:
    Conc.Backpressure -= 1;
    if (Conc.Backpressure < 0) {
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
    Rets[0] = Ctx != nullptr
                  ? static_cast<uint32_t>(Ctx->Storage[Info.CtxIdx & 1])
                  : UINT32_C(0);
    return {};
  }
  case ComponentCanonOpCode::Context__set: {
    if (auto *Ctx = Rt.currentCtx(); Ctx != nullptr) {
      Ctx->Storage[Info.CtxIdx & 1] = Args[0].get<uint32_t>();
    }
    return {};
  }

  case ComponentCanonOpCode::Yield: {
    if (Task == nullptr || !Task->mayBlock()) {
      // Yielding in a non-blocking context is a no-op.
      Rets[0] = UINT32_C(0);
      return {};
    }
    EXPECTED_TRY(auto Sig, Exec->componentTaskWait(
                               *Task, []() { return true; }, Info.Cancellable));
    if (Sig == ResumeSignal::Abort) {
      return Unexpect(ErrCode::Value::ComponentAsyncAborted);
    }
    Rets[0] = Sig == ResumeSignal::Cancelled ? UINT32_C(1) : UINT32_C(0);
    return {};
  }

  case ComponentCanonOpCode::Task__return: {
    if (Task == nullptr || !(Task->OptAsync || Task->AlwaysTaskReturn)) {
      return trapMsg(ErrCode::Value::ComponentTaskReturnInvalid,
                     "invalid `task.return` signature and/or options for "
                     "current task"sv);
    }
    // Declared result types must match the task's; the lift options of the
    // built-in must equal the task's lift options.
    const auto &TaskResults = Task->FT->getResultList();
    bool TypesMatch = TaskResults.size() == Info.RetTypes.size();
    if (TypesMatch) {
      for (size_t I = 0; I < Info.RetTypes.size(); ++I) {
        if (!valTypeEq(Inst, Info.RetTypes[I], Task->Inst,
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
    if ((Info.Mem != nullptr && Info.Mem != Task->Mem) ||
        Info.Enc != Task->Enc) {
      return trapMsg(ErrCode::Value::ComponentTaskReturnInvalid,
                     "invalid `task.return` signature and/or options for "
                     "current task"sv);
    }
    CanonicalABI::CanonCtx Cx{Exec, Info.Mem, Info.Realloc, Inst,
                              {},   {},       nullptr,      Info.Enc};
    CanonicalABI::FlatIter VI(Args);
    EXPECTED_TRY(auto Results,
                 CanonicalABI::liftFlatValues(Cx, VI, Info.RetTypes,
                                              CanonicalABI::MaxFlatParams));
    return Exec->componentTaskReturn(*Task, std::move(Results));
  }
  case ComponentCanonOpCode::Task__cancel: {
    if (Task == nullptr || !Task->OptAsync) {
      return trapMsg(ErrCode::Value::ComponentTaskNotCancelled,
                     "`task.cancel` called by task which has not been "
                     "cancelled"sv);
    }
    return Exec->componentTaskCancel(*Task);
  }

  case ComponentCanonOpCode::Waitable_set__new:
    Rets[0] = Inst->waitableSetAdd();
    return {};

  case ComponentCanonOpCode::Waitable_set__wait:
  case ComponentCanonOpCode::Waitable_set__poll: {
    const uint32_t SetIdx = Args[0].get<uint32_t>();
    const uint32_t Ptr = Args[1].get<uint32_t>();
    auto *WSet = Inst->waitableSetGet(SetIdx);
    if (WSet == nullptr) {
      EXPECTED_TRY(trapUnknownHandle(SetIdx));
    }
    AC::AsyncEvent Ev;
    if (Info.Code == ComponentCanonOpCode::Waitable_set__wait) {
      if (Task == nullptr || !Task->mayBlock()) {
        return trapCannotBlock();
      }
      WSet->NumWaiting += 1;
      auto SigOrErr = Exec->componentTaskWait(
          *Task, [WSet]() { return WSet->hasPendingEvent(); }, Info.Cancellable,
          /*AlwaysReleaseExcl=*/false, /*FastPath=*/true);
      EXPECTED_TRY(auto Sig, SigOrErr);
      if (Sig == ResumeSignal::Abort) {
        return Unexpect(ErrCode::Value::ComponentAsyncAborted);
      }
      WSet->NumWaiting -= 1;
      Ev = Sig == ResumeSignal::Cancelled
               ? AC::AsyncEvent{AC::AsyncEventCode::TaskCancelled, 0, 0}
               : WSet->takePendingEvent();
    } else {
      if (Task != nullptr && Info.Cancellable &&
          Task->St == ComponentTask::State::PendingCancel) {
        Task->St = ComponentTask::State::CancelDelivered;
        Ev = {AC::AsyncEventCode::TaskCancelled, 0, 0};
      } else if (!WSet->hasPendingEvent()) {
        Ev = {AC::AsyncEventCode::None, 0, 0};
      } else {
        Ev = WSet->takePendingEvent();
      }
    }
    if (Info.Mem == nullptr || !Info.Mem->checkAccessBound(Ptr, 8)) {
      return trapMsg(ErrCode::Value::ComponentTrap,
                     "event payload out of bounds of memory"sv);
    }
    EXPECTED_TRY(Info.Mem->storeValue(Ev.P1, Ptr));
    EXPECTED_TRY(Info.Mem->storeValue(Ev.P2, Ptr + 4));
    Rets[0] = static_cast<uint32_t>(Ev.Code);
    return {};
  }

  case ComponentCanonOpCode::Waitable_set__drop: {
    const uint32_t SetIdx = Args[0].get<uint32_t>();
    auto *WSet = Inst->waitableSetGet(SetIdx);
    if (WSet == nullptr) {
      EXPECTED_TRY(trapUnknownHandle(SetIdx));
    }
    if (!WSet->Elems.empty() || WSet->NumWaiting > 0) {
      return trapMsg(ErrCode::Value::ComponentWaitableSetNotEmpty,
                     "cannot drop waitable set with waiters"sv);
    }
    Inst->waitableSetRemove(SetIdx);
    return {};
  }

  case ComponentCanonOpCode::Waitable__join: {
    const uint32_t WIdx = Args[0].get<uint32_t>();
    const uint32_t SetIdx = Args[1].get<uint32_t>();
    auto *W = Inst->waitableGet(WIdx);
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
      auto *WSet = Inst->waitableSetGet(SetIdx);
      if (WSet == nullptr) {
        EXPECTED_TRY(trapUnknownHandle(SetIdx));
      }
      W->join(WSet);
    }
    return {};
  }

  case ComponentCanonOpCode::Subtask__cancel: {
    // A synchronous subtask.cancel in a task that cannot block traps before
    // the handle check.
    if (!Info.Cancellable && (Task == nullptr || !Task->mayBlock())) {
      return trapCannotBlock();
    }
    const uint32_t Idx = Args[0].get<uint32_t>();
    auto *W = Inst->waitableGet(Idx);
    if (W == nullptr || W->getKind() != AC::WaitableObj::Kind::Subtask) {
      EXPECTED_TRY(trapUnknownHandle(Idx));
    }
    auto *Sub = static_cast<AC::SubtaskObj *>(W);
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
    if (Sub->inWaitableSet() && !Info.Cancellable) {
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
        if (!Info.Cancellable) {
          if (Task == nullptr || !Task->mayBlock()) {
            return trapCannotBlock();
          }
          W->HasSyncWaiter = true;
          auto SigOrErr = Exec->componentTaskWait(
              *Task, [W]() { return W->hasPendingEvent(); },
              /*Cancellable=*/false);
          EXPECTED_TRY(auto Sig, SigOrErr);
          if (Sig == ResumeSignal::Abort) {
            return Unexpect(ErrCode::Value::ComponentAsyncAborted);
          }
          W->HasSyncWaiter = false;
        } else {
          Rets[0] = AC::AsyncBlocked;
          return {};
        }
      }
    }
    if (W->hasPendingEvent()) {
      (void)W->takePendingEvent();
    }
    if (!Sub->Delivered) {
      for (const auto &[LInst, LIdx] : Sub->Lenders) {
        if (auto *Slot = LInst->handleGet(LIdx);
            Slot != nullptr && Slot->Lends > 0) {
          Slot->Lends -= 1;
        }
      }
      Sub->Lenders.clear();
      Sub->Delivered = true;
    }
    Rets[0] = static_cast<uint32_t>(Sub->State);
    return {};
  }

  case ComponentCanonOpCode::Subtask__drop: {
    const uint32_t Idx = Args[0].get<uint32_t>();
    auto *W = Inst->waitableGet(Idx);
    if (W == nullptr || W->getKind() != AC::WaitableObj::Kind::Subtask) {
      EXPECTED_TRY(trapUnknownHandle(Idx));
    }
    auto *Sub = static_cast<AC::SubtaskObj *>(W);
    if (!Sub->resolveDelivered()) {
      return trapMsg(ErrCode::Value::ComponentSubtaskNotResolved,
                     "cannot drop a subtask which has not yet resolved"sv);
    }
    // The subtask object may outlive its table slot (the caller-side
    // closures keep it alive): leave any waitable set now.
    W->join(nullptr);
    Inst->waitableRemove(Idx);
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
    Rt.newSpawnVehicle(
        Task, Ctx, [this, FnMut, C, Ctx, Inst](ResumeSignal Sig) {
          if (Sig == ResumeSignal::Abort) {
            return;
          }
          std::array<ValVariant, 1> A{ValVariant(C)};
          std::array<ValType, 1> Ty{ValType(TypeCode::I32)};
          auto Res = Exec->invoke(FnMut, A, Ty);
          if (!Res &&
              Res.error().getEnum() != ErrCode::Value::ComponentAsyncAborted) {
            Exec->getComponentAsyncRuntime().noteTrap(Res.error(), Inst);
          }
          if (Ctx->Registered) {
            Inst->threadRemove(Ctx->Index);
            Ctx->Registered = false;
          }
        });
    Ctx->Index = Inst->threadAdd(Ctx);
    Ctx->Registered = true;
    Rets[0] = Ctx->Index;
    return {};
  }
  case ComponentCanonOpCode::Thread__resume_later: {
    const uint32_t Idx = Args[0].get<uint32_t>();
    auto *Ctx = Inst->threadGet(Idx);
    if (Ctx == nullptr) {
      EXPECTED_TRY(trapUnknownHandle(Idx));
    }
    auto *V = Ctx->Vehicle;
    const bool Suspended =
        V != nullptr && !V->ReadyFn &&
        std::find(Rt.Waiting.begin(), Rt.Waiting.end(), V) != Rt.Waiting.end();
    if (!Suspended) {
      return trapMsg(ErrCode::Value::ComponentThreadNotSuspended,
                     "cannot resume thread which is not suspended"sv);
    }
    V->ReadyFn = []() { return true; };
    return {};
  }
  case ComponentCanonOpCode::Thread__suspend: {
    if (Task == nullptr || !Task->mayBlock()) {
      return trapCannotBlock();
    }
    EXPECTED_TRY(auto Sig,
                 Exec->componentTaskWait(*Task, nullptr, Info.Cancellable));
    if (Sig == ResumeSignal::Abort) {
      return Unexpect(ErrCode::Value::ComponentAsyncAborted);
    }
    Rets[0] = Sig == ResumeSignal::Cancelled ? UINT32_C(1) : UINT32_C(0);
    return {};
  }
  case ComponentCanonOpCode::Thread__yield_then_resume:
  case ComponentCanonOpCode::Thread__suspend_then_resume: {
    const uint32_t Idx = Args[0].get<uint32_t>();
    auto *Ctx = Inst->threadGet(Idx);
    if (Ctx == nullptr) {
      EXPECTED_TRY(trapUnknownHandle(Idx));
    }
    auto *V = Ctx->Vehicle;
    const bool Suspended =
        V != nullptr && !V->ReadyFn &&
        std::find(Rt.Waiting.begin(), Rt.Waiting.end(), V) != Rt.Waiting.end();
    if (!Suspended) {
      return trapMsg(ErrCode::Value::ComponentThreadNotSuspended,
                     "cannot resume thread which is not suspended"sv);
    }
    if (Task == nullptr || !Task->mayBlock()) {
      return trapCannotBlock();
    }
    V->ReadyFn = []() { return true; };
    const bool SuspendSelf =
        Info.Code == ComponentCanonOpCode::Thread__suspend_then_resume;
    EXPECTED_TRY(auto Sig,
                 Exec->componentTaskWait(
                     *Task,
                     SuspendSelf ? std::function<bool()>()
                                 : std::function<bool()>([]() { return true; }),
                     Info.Cancellable));
    if (Sig == ResumeSignal::Abort) {
      return Unexpect(ErrCode::Value::ComponentAsyncAborted);
    }
    Rets[0] = Sig == ResumeSignal::Cancelled ? UINT32_C(1) : UINT32_C(0);
    return {};
  }

  case ComponentCanonOpCode::Stream__new:
  case ComponentCanonOpCode::Future__new: {
    const bool IsStream = Info.Code == ComponentCanonOpCode::Stream__new;
    auto Shared = std::make_shared<AC::SharedCopyObj>();
    Shared->IsStream = IsStream;
    Shared->ElemType = Info.Elem;
    Shared->ElemTypeInst = Inst;
    auto ReadEnd = std::make_shared<AC::CopyEndObj>(
        IsStream ? AC::WaitableObj::Kind::StreamRead
                 : AC::WaitableObj::Kind::FutureRead,
        Shared);
    auto WriteEnd = std::make_shared<AC::CopyEndObj>(
        IsStream ? AC::WaitableObj::Kind::StreamWrite
                 : AC::WaitableObj::Kind::FutureWrite,
        Shared);
    auto *ReadP = ReadEnd.get();
    auto *WriteP = WriteEnd.get();
    const uint32_t RIdx = Inst->waitableAdd(std::move(ReadEnd));
    const uint32_t WIdx = Inst->waitableAdd(std::move(WriteEnd));
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

  case ComponentCanonOpCode::Stream__close_readable:
  case ComponentCanonOpCode::Stream__close_writable:
  case ComponentCanonOpCode::Future__close_readable:
  case ComponentCanonOpCode::Future__close_writable:
    return runDropEnd(Args);

  case ComponentCanonOpCode::Error_context__new: {
    const uint32_t Ptr = Args[0].get<uint32_t>();
    const uint32_t Len = Args[1].get<uint32_t>();
    std::string Msg;
    if (Info.Mem != nullptr && Len > 0) {
      if (!Info.Mem->checkAccessBound(Ptr, Len)) {
        return trapMsg(ErrCode::Value::ComponentTrap,
                       "error-context message out of bounds"sv);
      }
      const auto SpanBytes = Info.Mem->getSpan<const char>(Ptr, Len);
      Msg.assign(SpanBytes.begin(), SpanBytes.end());
    }
    Rets[0] = Inst->errorContextAdd(std::move(Msg));
    return {};
  }
  case ComponentCanonOpCode::Error_context__debug_message: {
    const uint32_t Idx = Args[0].get<uint32_t>();
    const uint32_t Ptr = Args[1].get<uint32_t>();
    auto *Ctx = Inst->errorContextGet(Idx);
    if (Ctx == nullptr) {
      EXPECTED_TRY(trapUnknownHandle(Idx));
    }
    CanonicalABI::CanonCtx Cx{Exec, Info.Mem, Info.Realloc, Inst,
                              {},   {},       nullptr,      Info.Enc};
    return CanonicalABI::store(Cx, ComponentValVariant(Ctx->Message),
                               ComponentValType(ComponentTypeCode::String),
                               Ptr);
  }
  case ComponentCanonOpCode::Error_context__drop: {
    const uint32_t Idx = Args[0].get<uint32_t>();
    if (!Inst->errorContextRemove(Idx)) {
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
  const auto *Inst = Info.Inst;
  ComponentTask *Task = Exec->getComponentAsyncRuntime().currentTask();
  const bool IsWrite = Info.Code == ComponentCanonOpCode::Stream__write ||
                       Info.Code == ComponentCanonOpCode::Future__write;
  const bool IsFuture = Info.Code == ComponentCanonOpCode::Future__read ||
                        Info.Code == ComponentCanonOpCode::Future__write;
  const auto WantKind = IsFuture
                            ? (IsWrite ? AC::WaitableObj::Kind::FutureWrite
                                       : AC::WaitableObj::Kind::FutureRead)
                            : (IsWrite ? AC::WaitableObj::Kind::StreamWrite
                                       : AC::WaitableObj::Kind::StreamRead);

  // A synchronous op in a task that cannot block traps before inspecting the
  // handle: the task could never safely wait for the transfer.
  if (!Info.Cancellable && (Task == nullptr || !Task->mayBlock())) {
    return trapCannotBlock();
  }

  const uint32_t Idx = Args[0].get<uint32_t>();
  const uint32_t Ptr = Args[1].get<uint32_t>();
  const uint32_t Len = IsFuture ? 1 : Args[2].get<uint32_t>();

  auto *W = Inst->waitableGet(Idx);
  if (W == nullptr || W->getKind() != WantKind) {
    EXPECTED_TRY(trapUnknownHandle(Idx));
  }
  auto *E = static_cast<AC::CopyEndObj *>(W);
  auto Shared = E->Shared;
  if (!elemTypeEq(Inst, Info.Elem, Shared->ElemTypeInst, Shared->ElemType)) {
    return trapMsg(ErrCode::Value::ComponentHandleWrongType,
                   "stream or future element type mismatch"sv);
  }
  if (E->St == AC::CopyState::Done) {
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
  if (E->inWaitableSet() && !Info.Cancellable) {
    return trapMsg(
        ErrCode::Value::ComponentWaitableInSetSyncUse,
        "waitable cannot be used synchronously while added to a waitable "
        "set"sv);
  }

  // Build and validate this side's buffer.
  E->Buffer = AC::GuestBufferDesc{
      Inst, Info.Mem, Info.Realloc, Info.Enc, Info.Elem, Inst, Ptr, Len, 0};
  EXPECTED_TRY(checkBuffer(Exec, E->Buffer));

  E->St = AC::CopyState::Copying;

  if (Shared->Dropped) {
    // The peer end is gone: the copy resolves immediately as dropped.
    queueCopyEvent(E, Idx, AC::AsyncCopyResult::Dropped, false);
  } else if (!Shared->HasPending || Shared->PendingDone) {
    // Park this side of the rendezvous. An exhausted-but-uncollected parked
    // side no longer joins; the current end replaces it (its queued event
    // stays in its own object until collected).
    Shared->HasPending = true;
    Shared->PendingDone = false;
    Shared->PendingEnd = E;
  } else {
    auto *Peer = Shared->PendingEnd;
    const uint32_t PeerTableIdx = Peer->TableIdx;
    if (Peer->Buffer.Inst == Inst && !noneOrNumberType(Inst, Info.Elem)) {
      return trapMsg(ErrCode::Value::ComponentIntraCopy,
                     "cannot read from and write to intra-component "
                     "future"sv);
    }
    // `Src` is the writer's buffer, `Dst` the reader's; the current end is
    // E and the parked one is Peer.
    auto &Src = IsWrite ? E->Buffer : Peer->Buffer;
    auto &Dst = IsWrite ? Peer->Buffer : E->Buffer;
    // Mark the parked side finished (its Completed event stays queued until
    // collected; a peer drop before then flips it to Dropped).
    auto CompletePeer = [&]() {
      Shared->PendingDone = true;
      queueCopyEvent(Peer, PeerTableIdx, AC::AsyncCopyResult::Completed, true);
    };
    // Transfer into/out of the parked buffer. The parked side's Completed
    // event is queued on any progress, with the payload read from the
    // accumulated buffer progress at collection time; the buffer stays
    // parked and keeps accepting rendezvous until it fills up.
    auto TransferToPeer = [&]() -> Expect<void> {
      if (E->Buffer.remain() > 0) {
        const uint32_t N = std::min(E->Buffer.remain(), Peer->Buffer.remain());
        EXPECTED_TRY(copyElements(Exec, Dst, Src, N));
        queueCopyEvent(Peer, PeerTableIdx, AC::AsyncCopyResult::Completed,
                       true);
        if (Peer->Buffer.remain() == 0) {
          Shared->PendingDone = true;
        }
      }
      queueCopyEvent(E, Idx, AC::AsyncCopyResult::Completed, false);
      return {};
    };
    if (IsFuture) {
      // Single-element rendezvous: move the value and clear the pending slot.
      EXPECTED_TRY(copyElements(Exec, Dst, Src, 1));
      Shared->HasPending = false;
      Shared->PendingEnd = nullptr;
      queueCopyEvent(Peer, PeerTableIdx, AC::AsyncCopyResult::Completed, false);
      queueCopyEvent(E, Idx, AC::AsyncCopyResult::Completed, false);
    } else if (IsWrite) {
      // Spec SharedStreamImpl.write (writer active, reader parked).
      if (Peer->Buffer.remain() > 0) {
        EXPECTED_TRY(TransferToPeer());
      } else if (E->Buffer.zeroLength() && Peer->Buffer.zeroLength()) {
        // Both zero-length: the writer completes, the reader stays pending.
        queueCopyEvent(E, Idx, AC::AsyncCopyResult::Completed, false);
      } else {
        // The reader's buffer is full/zero: complete it, then park the
        // writer as the new pending end (E gets no event → parks below).
        CompletePeer();
        Shared->PendingEnd = E;
        Shared->PendingDone = false;
      }
    } else {
      // Spec SharedStreamImpl.read (reader active, writer parked).
      if (Peer->Buffer.remain() > 0) {
        EXPECTED_TRY(TransferToPeer());
      } else {
        // The pending writer is zero-length: complete it, then park the
        // reader as the new pending end (E gets no event → parks below).
        CompletePeer();
        Shared->PendingEnd = E;
        Shared->PendingDone = false;
      }
    }
  }

  if (!E->hasPendingEvent()) {
    if (!Info.Cancellable) {
      if (Task == nullptr || !Task->mayBlock()) {
        return trapCannotBlock();
      }
      E->HasSyncWaiter = true;
      auto SigOrErr = Exec->componentTaskWait(
          *Task, [E]() { return E->hasPendingEvent(); }, /*Cancellable=*/false,
          /*AlwaysReleaseExcl=*/false, /*FastPath=*/true);
      EXPECTED_TRY(auto Sig, SigOrErr);
      if (Sig == ResumeSignal::Abort) {
        return Unexpect(ErrCode::Value::ComponentAsyncAborted);
      }
      E->HasSyncWaiter = false;
    } else {
      Rets[0] = AC::AsyncBlocked;
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
  const auto *Inst = Info.Inst;
  ComponentTask *Task = Exec->getComponentAsyncRuntime().currentTask();
  const bool IsWrite =
      Info.Code == ComponentCanonOpCode::Stream__cancel_write ||
      Info.Code == ComponentCanonOpCode::Future__cancel_write;
  const bool IsFuture =
      Info.Code == ComponentCanonOpCode::Future__cancel_read ||
      Info.Code == ComponentCanonOpCode::Future__cancel_write;
  const auto WantKind = IsFuture
                            ? (IsWrite ? AC::WaitableObj::Kind::FutureWrite
                                       : AC::WaitableObj::Kind::FutureRead)
                            : (IsWrite ? AC::WaitableObj::Kind::StreamWrite
                                       : AC::WaitableObj::Kind::StreamRead);
  // A synchronous cancel in a task that cannot block traps before the handle
  // check (it could never wait for the cancellation to resolve).
  if (!Info.Cancellable && (Task == nullptr || !Task->mayBlock())) {
    return trapCannotBlock();
  }
  const uint32_t Idx = Args[0].get<uint32_t>();
  auto *W = Inst->waitableGet(Idx);
  if (W == nullptr || W->getKind() != WantKind) {
    EXPECTED_TRY(trapUnknownHandle(Idx));
  }
  auto *E = static_cast<AC::CopyEndObj *>(W);
  if (E->St != AC::CopyState::Copying || E->HasSyncWaiter) {
    return IsWrite
               ? trapMsg(ErrCode::Value::ComponentCancelWriteNotPending,
                         "stream or future write cancelled when no write is "
                         "pending"sv)
               : trapMsg(ErrCode::Value::ComponentCancelReadNotPending,
                         "stream or future read cancelled when no read is "
                         "pending"sv);
  }
  if (E->inWaitableSet() && !Info.Cancellable) {
    return trapMsg(
        ErrCode::Value::ComponentWaitableInSetSyncUse,
        "waitable cannot be used synchronously while added to a waitable "
        "set"sv);
  }
  E->St = AC::CopyState::CancellingCopy;
  auto Shared = E->Shared;
  if (!E->hasPendingEvent()) {
    // Cancel this side's parked rendezvous.
    if (Shared->HasPending && Shared->PendingEnd == E) {
      Shared->HasPending = false;
      Shared->PendingEnd = nullptr;
      queueCopyEvent(E, Idx, AC::AsyncCopyResult::Cancelled, false);
    }
    if (!E->hasPendingEvent()) {
      if (!Info.Cancellable) {
        if (Task == nullptr || !Task->mayBlock()) {
          return trapCannotBlock();
        }
        E->HasSyncWaiter = true;
        auto SigOrErr = Exec->componentTaskWait(
            *Task, [E]() { return E->hasPendingEvent(); },
            /*Cancellable=*/false);
        EXPECTED_TRY(auto Sig, SigOrErr);
        if (Sig == ResumeSignal::Abort) {
          return Unexpect(ErrCode::Value::ComponentAsyncAborted);
        }
        E->HasSyncWaiter = false;
      } else {
        Rets[0] = AC::AsyncBlocked;
        return {};
      }
    }
  }
  const auto Ev = E->takePendingEvent();
  uint32_t Payload = Ev.P2;
  // A cancelled stream copy that had already made progress reports that
  // progress as cancelled-with-count; futures keep their completion.
  if (!IsFuture && (Payload & 0xFU) ==
                       static_cast<uint32_t>(AC::AsyncCopyResult::Completed)) {
    Payload = (Payload & ~0xFU) |
              static_cast<uint32_t>(AC::AsyncCopyResult::Cancelled);
  }
  Rets[0] = Payload;
  return {};
}

Expect<void>
CanonAsyncBuiltinHostFunc::runDropEnd(Span<const ValVariant> Args) {
  const auto *Inst = Info.Inst;
  const bool IsWrite =
      Info.Code == ComponentCanonOpCode::Stream__close_writable ||
      Info.Code == ComponentCanonOpCode::Future__close_writable;
  const bool IsFuture =
      Info.Code == ComponentCanonOpCode::Future__close_readable ||
      Info.Code == ComponentCanonOpCode::Future__close_writable;
  const auto WantKind = IsFuture
                            ? (IsWrite ? AC::WaitableObj::Kind::FutureWrite
                                       : AC::WaitableObj::Kind::FutureRead)
                            : (IsWrite ? AC::WaitableObj::Kind::StreamWrite
                                       : AC::WaitableObj::Kind::StreamRead);
  const uint32_t Idx = Args[0].get<uint32_t>();
  auto *W = Inst->waitableGet(Idx);
  if (W == nullptr || W->getKind() != WantKind) {
    EXPECTED_TRY(trapUnknownHandle(Idx));
  }
  auto *E = static_cast<AC::CopyEndObj *>(W);
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
  if (IsFuture && IsWrite && E->St != AC::CopyState::Done) {
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
      queueCopyEvent(Peer, Peer->TableIdx, AC::AsyncCopyResult::Dropped, false);
    }
  }
  Inst->waitableRemove(Idx);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
