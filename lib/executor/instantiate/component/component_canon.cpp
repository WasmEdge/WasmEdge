// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
  // Wrapper over the spec's lower_flat_values (CanonicalABI.md L3212-3232).
  CanonicalABI::CanonCtx Cx{this, MemInst, RFuncInst, CompInst, {}};
  return CanonicalABI::lowerFlatValues(Cx, Vals, ValTypes,
                                       CanonicalABI::MaxFlatParams);
}

Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
Executor::convValsToComponent(
    Span<const std::pair<ValVariant, ValType>> CoreVals,
    Span<const ComponentValType> ValTypes,
    Runtime::Instance::MemoryInstance *MemInst,
    const Runtime::Instance::ComponentInstance *CompInst) {
  // Wrapper over the spec's lift_flat_values (CanonicalABI.md L3193-3202).
  CanonicalABI::CanonCtx Cx{this, MemInst, nullptr, CompInst, {}};
  CanonicalABI::FlatIter VI(CoreVals);
  EXPECTED_TRY(auto Lifted,
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
      Runtime::Instance::FunctionInstance *PostReturnFunc = nullptr;
      for (auto &Opt : Opts) {
        switch (Opt.getCode()) {
        case ComponentCanonOptCode::Encode_UTF8:
          // UTF-8 is the only encoding supported by the current canon ABI.
          break;
        case ComponentCanonOptCode::Encode_UTF16:
        case ComponentCanonOptCode::Encode_Latin1:
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    canon lift: non-UTF-8 encoding not implemented"sv);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
        case ComponentCanonOptCode::Memory:
          MemInst = CompInst.getCoreMemory(Opt.getIndex());
          break;
        case ComponentCanonOptCode::Realloc:
          ReallocFunc = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case ComponentCanonOptCode::PostReturn:
          PostReturnFunc = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case ComponentCanonOptCode::Async:
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    canon lift: 'async' not implemented"sv);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
        default:
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    canon lift: unsupported canonical option"sv);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
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
      // Pre-flight the ABI signature: surfaces gated / out-of-scope shapes
      // (async, indirect-params, lower-side indirect, etc.) at instantiation
      // time rather than at call time. Captures FlatSig so the post-return
      // signature check can compare against flatten_functype({}, $ft,
      // 'lift').results (spec L3292).
      CanonicalABI::CanonCtx PrefCx{nullptr, nullptr, nullptr, &CompInst, {}};
      EXPECTED_TRY(auto FlatSig,
                   CanonicalABI::flattenFuncType(PrefCx, DType->getFuncType(),
                                                 /*IsLift=*/true));

      // Validate the post-return signature against the lift's flat result
      // shape (spec L3292): post-return takes the original flat_results as
      // parameters and returns nothing.
      //
      // This duplicates component_validator.cpp:1546-1579's check. The
      // validator silently skips when it cannot resolve the core func's
      // SubType (alias / canon-synthesized core paths), so this layer is
      // the fallback that catches signature mismatches on those paths. Do
      // not demote it to `assuming` until the validator can resolve core
      // func types from every source.
      if (PostReturnFunc != nullptr) {
        const auto &PRType = PostReturnFunc->getFuncType();
        if (!PRType.getReturnTypes().empty() ||
            PRType.getParamTypes().size() != FlatSig.Results.size()) {
          spdlog::error(ErrCode::Value::InvalidCanonOption);
          spdlog::error("    canon lift: post-return must have signature "
                        "(func (param ...flatten_lift_results))"sv);
          return Unexpect(ErrCode::Value::InvalidCanonOption);
        }
        for (size_t I = 0; I < FlatSig.Results.size(); ++I) {
          if (PRType.getParamTypes()[I].getCode() !=
              FlatSig.Results[I].getCode()) {
            spdlog::error(ErrCode::Value::InvalidCanonOption);
            spdlog::error(
                "    canon lift: post-return param[{}] type mismatch"sv, I);
            return Unexpect(ErrCode::Value::InvalidCanonOption);
          }
        }
      }

      auto *FuncInst = CompInst.getCoreFunction(Canon.getIndex());
      CompInst.addFunction(
          std::make_unique<Runtime::Instance::Component::FunctionInstance>(
              DType->getFuncType(), FuncInst, MemInst, ReallocFunc, &CompInst,
              PostReturnFunc));
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
          spdlog::error(
              "    canon lower: non-UTF-8 encoding not implemented"sv);
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
      CanonicalABI::CanonCtx PrefCx{this, nullptr, nullptr, &CompInst, {}};
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
      spdlog::error("    incomplete canonical"sv);
      return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
