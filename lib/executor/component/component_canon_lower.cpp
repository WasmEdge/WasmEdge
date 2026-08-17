// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/component/canonical_abi.h"
#include "executor/component/executor.h"
#include "executor/executor.h"

#include "common/spdlog.h"

#include <cstdint>
#include <string_view>

namespace WasmEdge {
namespace Executor {
namespace Component {

using namespace std::literals;

CanonLowerHostFunc::CanonLowerHostFunc(
    ComponentExecutor *ExecIn, AST::FunctionType FlatSig,
    Runtime::Instance::Component::FunctionInstance *CalleeIn,
    const Runtime::Component::CanonOptions &OptsIn) noexcept
    : HostFunctionBase(/*FuncCost=*/0), Exec(ExecIn), Callee(CalleeIn),
      Opts(OptsIn),
      // A trailing out-ptr exists when the flat results spill.
      HasOutPtr(FlatSig.getReturnTypes().empty() &&
                !CalleeIn->getFuncType().getResultList().empty()) {
  DefType.getCompositeType().getFuncType() = std::move(FlatSig);
}

Expect<void> CanonLowerHostFunc::run(const Runtime::CallingFrame &,
                                     Span<const ValVariant> Args,
                                     Span<ValVariant> Rets) {
  // Lower-direction Context: memory and realloc come from the canon options.
  std::vector<std::pair<const Runtime::Instance::ComponentInstance *, uint32_t>>
      LiftedBorrows;
  CanonicalABI::Context Cx{Opts, Exec};
  Cx.LiftedBorrows = &LiftedBorrows;
  // Callee type indices are its own; the handle tables stay with the caller.
  if (const auto *CalleeComp = Callee->getComponentInstance();
      CalleeComp != nullptr && CalleeComp != Opts.Inst) {
    Cx.TypeResolver = [CalleeComp](uint32_t I) {
      return CalleeComp->getType(I);
    };
    Cx.ResourceResolver = [CalleeComp](uint32_t I) {
      return CalleeComp->getTypeResource(I);
    };
  }

  // Collect component-level param + result types from the callee.
  const auto &CFT = Callee->getFuncType();
  std::vector<ComponentValType> ParamTypes;
  ParamTypes.reserve(CFT.getParamList().size());
  for (const auto &P : CFT.getParamList()) {
    ParamTypes.push_back(P.getValType());
  }
  std::vector<ComponentValType> ResultTypes;
  ResultTypes.reserve(CFT.getResultList().size());
  for (const auto &R : CFT.getResultList()) {
    ResultTypes.push_back(R.getValType());
  }

  // A trailing out-ptr param appears when flat_results exceed the cap.
  std::optional<uint32_t> OutPtr;
  Span<const ValVariant> ParamArgs = Args;
  if (HasOutPtr) {
    if (Args.empty()) {
      spdlog::error(ErrCode::Value::FuncSigMismatch);
      spdlog::error("    canon lower: missing trailing out-ptr"sv);
      return Unexpect(ErrCode::Value::FuncSigMismatch);
    }
    OutPtr = Args.back().get<uint32_t>();
    ParamArgs = Args.subspan(0, Args.size() - 1);
  }

  // Lift params (spec L3193-3202).
  CanonicalABI::FlatIter VI(ParamArgs);
  EXPECTED_TRY(auto Params,
               CanonicalABI::liftFlatValues(Cx, VI, ParamTypes,
                                            CanonicalABI::MaxFlatParams));

  // Invoke the callee, then release the lends taken by borrow arguments.
  auto CompResOrErr = Exec->invoke(Callee, Params, ParamTypes);
  for (const auto &[Inst, Idx] : LiftedBorrows) {
    if (auto *Slot = Inst->handles().handleGet(Idx);
        Slot != nullptr && Slot->Lends > 0) {
      Slot->Lends -= 1;
    }
  }
  EXPECTED_TRY(auto CompRes, std::move(CompResOrErr));

  std::vector<ComponentValVariant> ResultValues;
  ResultValues.reserve(CompRes.size());
  for (auto &P : CompRes) {
    ResultValues.push_back(std::move(P.first));
  }

  // Lower results back to flat core values (spec L3212-3232).
  EXPECTED_TRY(auto FlatRet, CanonicalABI::lowerFlatValues(
                                 Cx, ResultValues, ResultTypes,
                                 CanonicalABI::MaxFlatResults, OutPtr));

  // Copy flat returns into Rets; with OutPtr both are empty.
  if (FlatRet.size() != Rets.size()) {
    spdlog::error(ErrCode::Value::FuncSigMismatch);
    spdlog::error(
        "    canon lower: flat result arity mismatch (got {}, expected {})"sv,
        FlatRet.size(), Rets.size());
    return Unexpect(ErrCode::Value::FuncSigMismatch);
  }
  for (size_t I = 0; I < FlatRet.size(); ++I) {
    Rets[I] = std::move(FlatRet[I]);
  }
  return {};
}

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
