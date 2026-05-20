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

namespace {

// convValsToCoreWASM's direct path reads `std::get<ValVariant>(V)` for
// non-string primitives — so the inverse of coerceValVariantToTyped is needed
// when the thunk forwards lifted typed primitives back into the lift-direction
// invoke. Re-wraps typed primitive arms as ComponentValVariant{ValVariant}.
ComponentValVariant
coerceTypedToValVariant(const ComponentValVariant &V,
                        ComponentTypeCode Code) noexcept {
  using TC = ComponentTypeCode;
  switch (Code) {
  case TC::Bool:
    return ComponentValVariant{
        ValVariant(static_cast<uint32_t>(std::get<bool>(V) ? 1u : 0u))};
  case TC::U8:
    return ComponentValVariant{
        ValVariant(static_cast<uint32_t>(std::get<uint8_t>(V)))};
  case TC::U16:
    return ComponentValVariant{
        ValVariant(static_cast<uint32_t>(std::get<uint16_t>(V)))};
  case TC::U32:
    return ComponentValVariant{ValVariant(std::get<uint32_t>(V))};
  case TC::U64:
    return ComponentValVariant{ValVariant(std::get<uint64_t>(V))};
  case TC::S8:
    return ComponentValVariant{
        ValVariant(static_cast<uint32_t>(std::get<int8_t>(V)))};
  case TC::S16:
    return ComponentValVariant{
        ValVariant(static_cast<uint32_t>(std::get<int16_t>(V)))};
  case TC::S32:
    return ComponentValVariant{
        ValVariant(static_cast<uint32_t>(std::get<int32_t>(V)))};
  case TC::S64:
    return ComponentValVariant{
        ValVariant(static_cast<uint64_t>(std::get<int64_t>(V)))};
  case TC::F32:
    return ComponentValVariant{ValVariant(std::get<float>(V))};
  case TC::F64:
    return ComponentValVariant{ValVariant(std::get<double>(V))};
  case TC::Char:
    return ComponentValVariant{ValVariant(std::get<uint32_t>(V))};
  default:
    // String / TypeIndex go through their own arms in convValsToCoreWASM.
    return V;
  }
}

// convValsToComponent's direct path wraps each primitive result as
// ComponentValVariant{ValVariant} (preserving the pre-Phase-1 calling
// convention used by existing host-side consumers). lowerFlat / lowerFlatValues
// expect ComponentValVariants whose primitive arm holds the typed primitive
// (uint32_t / int8_t / ...) directly. This helper rewrites the legacy
// ValVariant arm into the typed arm so the thunk can route directly through
// lowerFlatValues without forking the contract.
ComponentValVariant
coerceValVariantToTyped(const ComponentValVariant &V,
                        ComponentTypeCode Code) noexcept {
  if (!std::holds_alternative<ValVariant>(V)) {
    return V;
  }
  const auto &VV = std::get<ValVariant>(V);
  using TC = ComponentTypeCode;
  switch (Code) {
  case TC::Bool:
    return ComponentValVariant{VV.get<uint32_t>() != 0u};
  case TC::U8:
    return ComponentValVariant{static_cast<uint8_t>(VV.get<uint32_t>())};
  case TC::U16:
    return ComponentValVariant{static_cast<uint16_t>(VV.get<uint32_t>())};
  case TC::U32:
    return ComponentValVariant{VV.get<uint32_t>()};
  case TC::U64:
    return ComponentValVariant{VV.get<uint64_t>()};
  case TC::S8:
    return ComponentValVariant{static_cast<int8_t>(VV.get<uint32_t>())};
  case TC::S16:
    return ComponentValVariant{static_cast<int16_t>(VV.get<uint32_t>())};
  case TC::S32:
    return ComponentValVariant{static_cast<int32_t>(VV.get<uint32_t>())};
  case TC::S64:
    return ComponentValVariant{static_cast<int64_t>(VV.get<uint64_t>())};
  case TC::F32:
    return ComponentValVariant{VV.get<float>()};
  case TC::F64:
    return ComponentValVariant{VV.get<double>()};
  case TC::Char:
    return ComponentValVariant{VV.get<uint32_t>()};
  default:
    // Non-primitive shapes don't go through the ValVariant-wrapped path in
    // the first place — pass through.
    return V;
  }
}

} // namespace

CanonLowerHostFunc::CanonLowerHostFunc(
    Executor *ExecIn, const CanonicalABI::FlatFuncType &FlatSig,
    Runtime::Instance::Component::FunctionInstance *CalleeIn,
    Runtime::Instance::MemoryInstance *MemoryIn,
    Runtime::Instance::FunctionInstance *ReallocIn,
    const Runtime::Instance::ComponentInstance *CompInstIn) noexcept
    : HostFunctionBase(/*FuncCost=*/0), Exec(ExecIn), Callee(CalleeIn),
      Memory(MemoryIn), Realloc(ReallocIn), CompInst(CompInstIn) {
  // Populate DefType from the pre-flighted flat ABI signature. Mirrors
  // HostFunction<T>::initializeFuncType() (include/runtime/hostfunc.h:100-111)
  // but built at runtime since the signature is data-driven.
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
  CanonicalABI::CanonCtx Cx{Exec, Memory, Realloc, CompInst};

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

  // Determine whether the lower direction added a trailing out-pointer.
  // Spec L2829-2831: if flat_results > MAX_FLAT_RESULTS, params += [ptr];
  // results = []. The thunk's wasm caller passes the out-ptr as the last arg.
  EXPECTED_TRY(auto FlatResultCount,
               [&]() -> Expect<uint32_t> {
                 uint32_t N = 0;
                 for (const auto &T : ResultTypes) {
                   EXPECTED_TRY(auto Sub, CanonicalABI::flattenType(Cx, T));
                   N += static_cast<uint32_t>(Sub.size());
                 }
                 return N;
               }());
  const bool HasOutPtr = FlatResultCount > CanonicalABI::MaxFlatResults;
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

  // The lift side (Executor::invoke + convValsToCoreWASM) currently expects
  // primitive params wrapped as ComponentValVariant{ValVariant} — coerce.
  for (size_t I = 0; I < Params.size(); ++I) {
    Params[I] = coerceTypedToValVariant(Params[I], ParamTypes[I].getCode());
  }

  // Invoke the wrapped component function. Note this re-enters
  // Executor::invoke from inside a host-function frame — host re-entry is
  // already supported by the WASI plumbing, so no special-casing here.
  EXPECTED_TRY(auto CompRes,
               Exec->invoke(Callee, Params, ParamTypes));

  // Split CompRes (vector<pair<ComponentValVariant, ComponentValType>>) into
  // values + types and coerce legacy ValVariant-wrapped primitives back to
  // their typed form before lowering.
  std::vector<ComponentValVariant> ResultValues;
  ResultValues.reserve(CompRes.size());
  for (size_t I = 0; I < CompRes.size(); ++I) {
    ResultValues.push_back(
        coerceValVariantToTyped(CompRes[I].first, ResultTypes[I].getCode()));
  }

  // Lower results back to flat core values (spec L3212-3232).
  EXPECTED_TRY(auto FlatRet,
               CanonicalABI::lowerFlatValues(Cx, ResultValues, ResultTypes,
                                             CanonicalABI::MaxFlatResults,
                                             OutPtr));

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
