// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/canonical_abi.h"
#include "executor/component/lower_thunk.h"
#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <cstdint>
#include <string_view>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;

Expect<std::vector<ValVariant>> Executor::convValsToCoreWASM(
    Span<const ComponentValVariant> Vals, Span<const ComponentValType> ValTypes,
    Runtime::Instance::FunctionInstance *RFuncInst,
    Runtime::Instance::MemoryInstance *MemInst,
    const Runtime::Instance::ComponentInstance *CompInst) {
  // Thin wrapper over the spec's lower_flat_values (CanonicalABI.md
  // L3212-3232). lowerFlatValues already covers direct-primitive lowering,
  // top-level String via callRealloc, aggregate types via lowerFlat→storeDef
  // recursion, and the indirect-params (>MAX_FLAT_PARAMS) realloc-and-store
  // path. All ComponentValVariant primitives flow on the typed-arm
  // convention end-to-end.
  CanonicalABI::CanonCtx Cx{this, MemInst, RFuncInst, CompInst};
  return CanonicalABI::lowerFlatValues(Cx, Vals, ValTypes,
                                       CanonicalABI::MaxFlatParams);
}

Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
Executor::convValsToComponent(
    Span<const std::pair<ValVariant, ValType>> CoreVals,
    Span<const ComponentValType> ValTypes,
    Runtime::Instance::MemoryInstance *MemInst,
    const Runtime::Instance::ComponentInstance *CompInst) {
  // Thin wrapper over the spec's lift_flat_values (CanonicalABI.md
  // L3193-3202). liftFlatValues already covers direct-primitive lifting,
  // top-level String via loadPrim, aggregate types via liftFlat→loadDef
  // recursion, and the indirect-result (>MAX_FLAT_RESULTS) load-tuple-from-ptr
  // path. All produced ComponentValVariant primitives use the typed-arm
  // convention.
  CanonicalABI::CanonCtx Cx{this, MemInst, nullptr, CompInst};
  CanonicalABI::FlatIter VI(CoreVals);
  EXPECTED_TRY(
      auto Lifted,
      CanonicalABI::liftFlatValues(Cx, VI, ValTypes,
                                   CanonicalABI::MaxFlatResults));
  std::vector<std::pair<ComponentValVariant, ComponentValType>> Out;
  Out.reserve(Lifted.size());
  for (size_t I = 0; I < Lifted.size(); ++I) {
    Out.emplace_back(std::move(Lifted[I]), ValTypes[I]);
  }
  return Out;
}

Expect<void>
Executor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::CanonSection &CanonSec) {
  for (const auto &Canon : CanonSec.getContent()) {
    switch (Canon.getOpCode()) {
    case ComponentCanonOpCode::Lift: {
      // Lift wraps a core Wasm function to a component function with proper
      // canonical ABI modification.
      const auto &Opts = Canon.getOptions();
      Runtime::Instance::MemoryInstance *MemInst = nullptr;
      Runtime::Instance::FunctionInstance *ReallocFunc = nullptr;
      for (auto &Opt : Opts) {
        switch (Opt.getCode()) {
        case ComponentCanonOptCode::Encode_UTF8:
        case ComponentCanonOptCode::Encode_UTF16:
        case ComponentCanonOptCode::Encode_Latin1:
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    incomplete canonincal options"sv);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
        case ComponentCanonOptCode::Memory:
          MemInst = CompInst.getCoreMemory(Opt.getIndex());
          break;
        case ComponentCanonOptCode::Realloc:
          ReallocFunc = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case ComponentCanonOptCode::PostReturn:
        case ComponentCanonOptCode::Async:
          // TODO: incomplete validation of these cases.
        default:
          assumingUnreachable();
        }
      }

      const auto *DType = CompInst.getType(Canon.getTargetIndex());
      if (unlikely(!DType->isFuncType())) {
        // It does not make sense to lift an instance that is not a function, so
        // this is unlikely to happen.
        spdlog::error(ErrCode::Value::InvalidCanonOption);
        spdlog::error("    Cannot lift a non-function"sv);
        return Unexpect(ErrCode::Value::InvalidCanonOption);
      }
      // Pre-flight the ABI signature: this surfaces gated / out-of-scope
      // shapes (async, indirect-params, lower-side indirect, etc.) at
      // instantiation time rather than at call time. Sync-lift indirect
      // results are explicitly supported by B and reduce to results=[i32].
      {
        // Pre-flight only needs CompInst for TypeIndex resolution; flatten
        // never invokes realloc, so leaving Exec/Mem/Realloc unset is fine.
        CanonicalABI::CanonCtx PrefCx{nullptr, nullptr, nullptr, &CompInst};
        EXPECTED_TRY(CanonicalABI::flattenFuncType(PrefCx, DType->getFuncType(),
                                                   /*IsLift=*/true));
      }
      auto *FuncInst = CompInst.getCoreFunction(Canon.getIndex());
      CompInst.addFunction(
          std::make_unique<Runtime::Instance::Component::FunctionInstance>(
              DType->getFuncType(), FuncInst, MemInst, ReallocFunc,
              &CompInst));
      break;
    }
    case ComponentCanonOpCode::Lower: {
      // canon lower: synthesize a core wasm function whose body lifts core
      // args to component values, calls the wrapped component function, and
      // lowers the result back. Spec L3534-3640 (sync branch).
      const auto &Opts = Canon.getOptions();
      Runtime::Instance::MemoryInstance *MemInst = nullptr;
      Runtime::Instance::FunctionInstance *ReallocFunc = nullptr;
      for (auto &Opt : Opts) {
        switch (Opt.getCode()) {
        case ComponentCanonOptCode::Encode_UTF8:
          // UTF-8 is the only encoding supported here.
          break;
        case ComponentCanonOptCode::Encode_UTF16:
        case ComponentCanonOptCode::Encode_Latin1:
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    canon lower: non-UTF-8 encoding not implemented"sv);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
        case ComponentCanonOptCode::Memory:
          MemInst = CompInst.getCoreMemory(Opt.getIndex());
          break;
        case ComponentCanonOptCode::Realloc:
          ReallocFunc = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case ComponentCanonOptCode::PostReturn:
          // Spec L3261: post-return is only valid on canon lift.
          spdlog::error(ErrCode::Value::InvalidCanonOption);
          spdlog::error(
              "    canon lower: 'post-return' is only allowed on canon lift"sv);
          return Unexpect(ErrCode::Value::InvalidCanonOption);
        case ComponentCanonOptCode::Async:
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    canon lower: 'async' not implemented"sv);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
        default:
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    canon lower: unsupported canonical option"sv);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
        }
      }

      auto *Callee = CompInst.getFunction(Canon.getIndex());
      const auto &CFT = Callee->getFuncType();

      // Pre-flight the lower-direction flat ABI so unsupported shapes (async,
      // gated types) fail at instantiation time. flattenFuncType doesn't need
      // Mem / Realloc to compute the signature.
      CanonicalABI::CanonCtx PrefCx{this, nullptr, nullptr, &CompInst};
      EXPECTED_TRY(auto FlatSig,
                   CanonicalABI::flattenFuncType(PrefCx, CFT,
                                                 /*IsLift=*/false));

      auto Thunk = std::make_unique<CanonLowerHostFunc>(
          this, FlatSig, Callee, MemInst, ReallocFunc, &CompInst);
      // Register via the host-function helper so the synthesized core
      // function's defined type lands in a ModuleInstance::Types list and
      // matchType walks find it when wasm callers import this function.
      CompInst.addCoreHostFunction(std::move(Thunk));
      break;
    }
    case ComponentCanonOpCode::Resource__new:
    case ComponentCanonOpCode::Resource__drop:
    case ComponentCanonOpCode::Resource__rep:
    default:
      spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
      spdlog::error("    incomplete canonincal"sv);
      return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
