// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/component/lower_thunk.h"
#include "executor/executor.h"

#include "common/spdlog.h"

#include <cstdint>
#include <string_view>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;

CanonLowerHostFunc::CanonLowerHostFunc(
    Executor *ExecIn, const CanonicalABI::FlatFuncType &FlatSig,
    Runtime::Instance::Component::FunctionInstance *CalleeIn,
    Runtime::Instance::MemoryInstance *MemoryIn,
    Runtime::Instance::FunctionInstance *ReallocIn,
    const Runtime::Instance::ComponentInstance *CompInstIn,
    StringEncoding EncIn) noexcept
    : HostFunctionBase(/*FuncCost=*/0), Exec(ExecIn), Callee(CalleeIn),
      Memory(MemoryIn), Realloc(ReallocIn), CompInst(CompInstIn),
      // Lower side adds a trailing out-ptr when flat_results > MaxFlatResults;
      // in that case FlatSig.Results is empty (spec L2829-2831) while the
      // callee still has result types.
      HasOutPtr(FlatSig.Results.empty() &&
                !CalleeIn->getFuncType().getResultList().empty()),
      Enc(EncIn) {
  // Populate DefType from the pre-flighted flat ABI signature.
  auto &FT = DefType.getCompositeType().getFuncType();
  auto &Params = FT.getParamTypes();
  auto &Returns = FT.getReturnTypes();
  Params.reserve(FlatSig.Params.size());
  Returns.reserve(FlatSig.Results.size());
  for (const auto &P : FlatSig.Params) {
    Params.push_back(P);
  }
  for (const auto &R : FlatSig.Results) {
    Returns.push_back(R);
  }
}

Expect<void> CanonLowerHostFunc::run(const Runtime::CallingFrame &,
                                     Span<const ValVariant> Args,
                                     Span<ValVariant> Rets) {
  // Lower-direction CanonCtx: Memory/Realloc come from the canon lower
  // options. Exec is needed by callRealloc inside lower_flat_values when
  // nested strings/lists in results need their own buffer.
  std::vector<std::pair<const Runtime::Instance::ComponentInstance *, uint32_t>>
      LiftedBorrows;
  CanonicalABI::CanonCtx Cx{Exec, Memory, Realloc,        CompInst,
                            {},   {},     &LiftedBorrows, Enc};
  // The callee's function type carries type indices of the callee's
  // instance; handle tables stay with the caller (CompInst).
  if (const auto *CalleeComp = Callee->getComponentInstance();
      CalleeComp != nullptr && CalleeComp != CompInst) {
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

  // Spec L2829-2831: if flat_results > MAX_FLAT_RESULTS, params += [ptr];
  // results = []. The thunk's wasm caller passes the out-ptr as the last arg.
  std::optional<uint32_t> OutPtr;
  Span<const ValVariant> ParamArgs = Args;
  if (HasOutPtr) {
    if (Args.empty()) {
      spdlog::error(ErrCode::Value::FuncSigMismatch);
      spdlog::error("    canon lower thunk: missing trailing out-ptr"sv);
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

  // Invoke the wrapped component function, then release the lends taken by
  // borrow arguments (sync calls scope borrows to the call).
  auto CompResOrErr = Exec->invoke(Callee, Params, ParamTypes);
  for (const auto &[Inst, Idx] : LiftedBorrows) {
    if (auto *Slot = Inst->handleGet(Idx); Slot != nullptr && Slot->Lends > 0) {
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

  // Copy flat returns into the Rets span. When OutPtr is set, FlatRet is
  // empty and Rets is empty too.
  if (FlatRet.size() != Rets.size()) {
    spdlog::error(ErrCode::Value::FuncSigMismatch);
    spdlog::error(
        "    canon lower thunk: flat result arity mismatch (got {}, expected {})"sv,
        FlatRet.size(), Rets.size());
    return Unexpect(ErrCode::Value::FuncSigMismatch);
  }
  for (size_t I = 0; I < FlatRet.size(); ++I) {
    Rets[I] = std::move(FlatRet[I]);
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
