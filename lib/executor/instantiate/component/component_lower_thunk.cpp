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
  CanonicalABI::CanonCtx Cx{Exec, Memory, Realloc, CompInst, {}};

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

  // Lift params (spec L3193-3202). Produces typed-arm ComponentValVariants;
  // Executor::invoke (lift direction) now consumes the same convention end-to-
  // end, so no boundary translation is needed.
  CanonicalABI::FlatIter VI(ParamArgs);
  EXPECTED_TRY(auto Params,
               CanonicalABI::liftFlatValues(Cx, VI, ParamTypes,
                                            CanonicalABI::MaxFlatParams));

  // Invoke the wrapped component function. Note this re-enters
  // Executor::invoke from inside a host-function frame — host re-entry is
  // already supported by the WASI plumbing, so no special-casing here.
  EXPECTED_TRY(auto CompRes,
               Exec->invoke(Callee, Params, ParamTypes));

  // Split CompRes (vector<pair<ComponentValVariant, ComponentValType>>) into
  // a value-only vector for lowerFlatValues. Values already use typed arms.
  std::vector<ComponentValVariant> ResultValues;
  ResultValues.reserve(CompRes.size());
  for (auto &P : CompRes) {
    ResultValues.push_back(std::move(P.first));
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
