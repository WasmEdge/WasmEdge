// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/canonical_abi.h"
#include "executor/component/executor.h"
#include "executor/executor.h"

#include "common/errcode.h"
#include "common/spdlog.h"

using namespace std::literals;

namespace WasmEdge {
namespace Executor {
namespace Component {

CanonResourceNewHostFunc::CanonResourceNewHostFunc(
    const Runtime::Instance::ComponentInstance *CompInst,
    const Runtime::Instance::Component::ResourceTypeInstance *ResType) noexcept
    : HostFunctionBase(/*FuncCost=*/0), Inst(CompInst), RT(ResType) {
  // The representation takes the width the resource type declares.
  DefType.getCompositeType().getFuncType() =
      AST::FunctionType({ResType->getRepType()}, {ValType(TypeCode::I32)});
}

Expect<void> CanonResourceNewHostFunc::run(const Runtime::CallingFrame &,
                                           Span<const ValVariant> Args,
                                           Span<ValVariant> Rets) {
  if (!Inst->isLeaveAllowed()) {
    spdlog::error(ErrCode::Value::ComponentCannotLeave);
    return Unexpect(ErrCode::Value::ComponentCannotLeave);
  }
  const uint64_t Rep = RT->getRep(Args[0]);
  Rets[0].emplace<uint32_t>(Inst->addHandle(RT, Rep, true));
  return {};
}

CanonResourceRepHostFunc::CanonResourceRepHostFunc(
    const Runtime::Instance::ComponentInstance *CompInst,
    const Runtime::Instance::Component::ResourceTypeInstance *ResType) noexcept
    : HostFunctionBase(/*FuncCost=*/0), Inst(CompInst), RT(ResType) {
  DefType.getCompositeType().getFuncType() =
      AST::FunctionType({ValType(TypeCode::I32)}, {ResType->getRepType()});
}

Expect<void> CanonResourceRepHostFunc::run(const Runtime::CallingFrame &,
                                           Span<const ValVariant> Args,
                                           Span<ValVariant> Rets) {
  const uint32_t Idx = Args[0].get<uint32_t>();
  auto *Slot = Inst->getHandle(Idx);
  if (Slot == nullptr) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    spdlog::error("    resource.rep: handle index {}"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  if (Slot->RT != RT) {
    spdlog::error(ErrCode::Value::ComponentHandleWrongType);
    spdlog::error("    resource.rep: handle index {}"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleWrongType);
  }
  Rets[0] = RT->getRepSlot(Slot->Rep);
  return {};
}

CanonResourceDropHostFunc::CanonResourceDropHostFunc(
    ComponentExecutor *CompExec,
    const Runtime::Instance::ComponentInstance *CompInst,
    const Runtime::Instance::Component::ResourceTypeInstance *ResType) noexcept
    : HostFunctionBase(/*FuncCost=*/0), Exec(CompExec), Inst(CompInst),
      RT(ResType) {
  DefType.getCompositeType().getFuncType() =
      AST::FunctionType({ValType(TypeCode::I32)}, {});
}

Expect<void> CanonResourceDropHostFunc::run(const Runtime::CallingFrame &,
                                            Span<const ValVariant> Args,
                                            Span<ValVariant>) {
  if (!Inst->isLeaveAllowed()) {
    spdlog::error(ErrCode::Value::ComponentCannotLeave);
    return Unexpect(ErrCode::Value::ComponentCannotLeave);
  }
  const uint32_t Idx = Args[0].get<uint32_t>();
  auto *Slot = Inst->getHandle(Idx);
  if (Slot == nullptr) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    spdlog::error("    resource.drop: handle index {}"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  if (Slot->RT != RT) {
    spdlog::error(ErrCode::Value::ComponentHandleWrongType);
    spdlog::error("    resource.drop: handle index {}"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleWrongType);
  }
  if (Slot->Own && Slot->Lends != 0) {
    spdlog::error(ErrCode::Value::ComponentResourceBorrowed);
    spdlog::error("    resource.drop: handle index {}"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentResourceBorrowed);
  }
  const auto Removed = Inst->removeHandle(Idx);
  if (Removed->Own) {
    if (RT->getHostDtor()) {
      RT->getHostDtor()(Removed->Rep);
    } else if (RT->getDtor() != nullptr) {
      // The destructor runs as an implicit synchronous task.
      EXPECTED_TRY(
          Exec->resourceDtorCall(RT->getImpl(), RT->getDtor(), Removed->Rep)
              .map_error([](auto E) {
                spdlog::error("    resource.drop: destructor failed"sv);
                return E;
              }));
    }
  } else if (Removed->BorrowScope != nullptr) {
    // Dropping a borrow releases it from the receiving task's count.
    if (Removed->BorrowScope->NumBorrows > 0) {
      Removed->BorrowScope->NumBorrows -= 1;
    }
  }
  return {};
}

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
