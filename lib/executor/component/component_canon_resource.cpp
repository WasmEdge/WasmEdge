// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- component_canon_resource.cpp - canon resource.* core functions ----===//
//
// Runtime behavior of the resource built-ins over the canonical handle table.
//
//===----------------------------------------------------------------------===//

#include "executor/component/canonical_abi.h"

#include "common/errcode.h"
#include "common/spdlog.h"
#include "executor/component/executor.h"
#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {
namespace Component {

using namespace std::literals;

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
// A resource representation is i32, or i64 under the memory64 proposal.
ValType
repType(const Runtime::Instance::Component::ResourceTypeInstance *RT) noexcept {
  return RT != nullptr && RT->RepI64 ? ValType(TypeCode::I64)
                                     : ValType(TypeCode::I32);
}

uint64_t repOf(const Runtime::Instance::Component::ResourceTypeInstance *RT,
               const ValVariant &V) noexcept {
  return RT != nullptr && RT->RepI64 ? V.get<uint64_t>()
                                     : static_cast<uint64_t>(V.get<uint32_t>());
}

} // namespace

CanonResourceNewHostFunc::CanonResourceNewHostFunc(
    const Runtime::Instance::ComponentInstance *InstIn,
    const Runtime::Instance::Component::ResourceTypeInstance *RTIn) noexcept
    : HostFunctionBase(/*FuncCost=*/0), Inst(InstIn), RT(RTIn) {
  // The representation takes the width the resource type declares.
  setFuncType(DefType.getCompositeType().getFuncType(), {repType(RTIn)},
              {ValType(TypeCode::I32)});
}

Expect<void> CanonResourceNewHostFunc::run(const Runtime::CallingFrame &,
                                           Span<const ValVariant> Args,
                                           Span<ValVariant> Rets) {
  if (!Inst->concurrency().mayLeave()) {
    spdlog::error(ErrCode::Value::ComponentCannotLeave);
    spdlog::error("    cannot leave component instance"sv);
    return Unexpect(ErrCode::Value::ComponentCannotLeave);
  }
  const uint64_t Rep = repOf(RT, Args[0]);
  Rets[0].emplace<uint32_t>(Inst->handles().handleAdd(RT, Rep, true));
  return {};
}

CanonResourceRepHostFunc::CanonResourceRepHostFunc(
    const Runtime::Instance::ComponentInstance *InstIn,
    const Runtime::Instance::Component::ResourceTypeInstance *RTIn) noexcept
    : HostFunctionBase(/*FuncCost=*/0), Inst(InstIn), RT(RTIn) {
  setFuncType(DefType.getCompositeType().getFuncType(),
              {ValType(TypeCode::I32)}, {repType(RTIn)});
}

Expect<void> CanonResourceRepHostFunc::run(const Runtime::CallingFrame &,
                                           Span<const ValVariant> Args,
                                           Span<ValVariant> Rets) {
  const uint32_t Idx = Args[0].get<uint32_t>();
  auto *Slot = Inst->handles().handleGet(Idx);
  if (Slot == nullptr) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    spdlog::error("    resource.rep: unknown handle index {}"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  if (Slot->RT != RT) {
    spdlog::error(ErrCode::Value::ComponentHandleWrongType);
    spdlog::error("    resource.rep: handle index {} used with the wrong "
                  "type"sv,
                  Idx);
    return Unexpect(ErrCode::Value::ComponentHandleWrongType);
  }
  if (RT != nullptr && RT->RepI64) {
    Rets[0].emplace<uint64_t>(Slot->Rep);
  } else {
    Rets[0].emplace<uint32_t>(static_cast<uint32_t>(Slot->Rep));
  }
  return {};
}

CanonResourceDropHostFunc::CanonResourceDropHostFunc(
    ComponentExecutor *ExecIn,
    const Runtime::Instance::ComponentInstance *InstIn,
    const Runtime::Instance::Component::ResourceTypeInstance *RTIn) noexcept
    : HostFunctionBase(/*FuncCost=*/0), Exec(ExecIn), Inst(InstIn), RT(RTIn) {
  setFuncType(DefType.getCompositeType().getFuncType(),
              {ValType(TypeCode::I32)}, {});
}

Expect<void> CanonResourceDropHostFunc::run(const Runtime::CallingFrame &,
                                            Span<const ValVariant> Args,
                                            Span<ValVariant> Rets) {
  if (!Inst->concurrency().mayLeave()) {
    spdlog::error(ErrCode::Value::ComponentCannotLeave);
    spdlog::error("    cannot leave component instance"sv);
    return Unexpect(ErrCode::Value::ComponentCannotLeave);
  }
  (void)Rets;
  const uint32_t Idx = Args[0].get<uint32_t>();
  auto *Slot = Inst->handles().handleGet(Idx);
  if (Slot == nullptr) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    spdlog::error("    resource.drop: unknown handle index {}"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  if (Slot->RT != RT) {
    spdlog::error(ErrCode::Value::ComponentHandleWrongType);
    spdlog::error("    resource.drop: handle index {} used with the wrong "
                  "type"sv,
                  Idx);
    return Unexpect(ErrCode::Value::ComponentHandleWrongType);
  }
  if (Slot->Own && Slot->Lends != 0) {
    spdlog::error(ErrCode::Value::ComponentResourceBorrowed);
    spdlog::error("    resource.drop: handle {} still lent"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentResourceBorrowed);
  }
  const auto Removed = Inst->handles().handleRemove(Idx);
  if (Removed->Own) {
    if (RT != nullptr && RT->HostDtor) {
      RT->HostDtor(Removed->Rep);
    } else if (RT != nullptr && RT->Dtor != nullptr) {
      // The destructor runs as an implicit synchronous task.
      EXPECTED_TRY(Exec->resourceDtorCall(RT->Impl, RT->Dtor, Removed->Rep)
                       .map_error([](auto E) {
                         spdlog::error("    resource.drop: destructor "
                                       "failed"sv);
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
