// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/async_thunk.h"
#include "executor/component/canonical_abi.h"
#include "executor/component/lower_thunk.h"
#include "executor/component/resource_thunk.h"
#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <cstdint>
#include <string_view>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;

namespace {
// Map a canon `string-encoding` option code to the runtime StringEncoding. The
// three encoding option codes correspond 1:1; shared by the canon lift and
// lower option loops so the mapping lives in one place.
StringEncoding toStringEncoding(ComponentCanonOptCode Code) noexcept {
  switch (Code) {
  case ComponentCanonOptCode::Encode_UTF16:
    return StringEncoding::UTF16;
  case ComponentCanonOptCode::Encode_Latin1:
    return StringEncoding::Latin1UTF16;
  default:
    return StringEncoding::UTF8;
  }
}
} // namespace

Expect<std::vector<ValVariant>> Executor::convValsToCoreWASM(
    Span<const ComponentValVariant> Vals, Span<const ComponentValType> ValTypes,
    Runtime::Instance::FunctionInstance *RFuncInst,
    Runtime::Instance::MemoryInstance *MemInst,
    const Runtime::Instance::ComponentInstance *CompInst, StringEncoding Enc) {
  // Wrapper over the spec's lower_flat_values (CanonicalABI.md L3212-3232).
  CanonicalABI::CanonCtx Cx{this, MemInst, RFuncInst, CompInst,
                            {},   {},      nullptr,   Enc};
  return CanonicalABI::lowerFlatValues(Cx, Vals, ValTypes,
                                       CanonicalABI::MaxFlatParams);
}

Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
Executor::convValsToComponent(
    Span<const std::pair<ValVariant, ValType>> CoreVals,
    Span<const ComponentValType> ValTypes,
    Runtime::Instance::MemoryInstance *MemInst,
    const Runtime::Instance::ComponentInstance *CompInst, StringEncoding Enc) {
  // Wrapper over the spec's lift_flat_values (CanonicalABI.md L3193-3202).
  CanonicalABI::CanonCtx Cx{this, MemInst, nullptr, CompInst,
                            {},   {},      nullptr, Enc};
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
      Runtime::Instance::FunctionInstance *CallbackFunc = nullptr;
      StringEncoding Enc = StringEncoding::UTF8;
      bool AsyncLift = false;
      bool AlwaysTaskReturn = false;
      for (auto &Opt : Opts) {
        switch (Opt.getCode()) {
        case ComponentCanonOptCode::Encode_UTF8:
        case ComponentCanonOptCode::Encode_UTF16:
        case ComponentCanonOptCode::Encode_Latin1:
          Enc = toStringEncoding(Opt.getCode());
          break;
        case ComponentCanonOptCode::Memory:
          MemInst = CompInst.getCoreMemory(Opt.getIndex());
          if (MemInst != nullptr &&
              MemInst->getMemoryType().getLimit().is64()) {
            spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
            spdlog::error(
                "    canonical ABI over a 64-bit memory is not implemented"sv);
            return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
          }
          break;
        case ComponentCanonOptCode::Realloc:
          ReallocFunc = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case ComponentCanonOptCode::PostReturn:
          PostReturnFunc = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case ComponentCanonOptCode::Async:
          AsyncLift = true;
          break;
        case ComponentCanonOptCode::Callback:
          CallbackFunc = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case ComponentCanonOptCode::AlwaysTaskReturn:
          AlwaysTaskReturn = true;
          break;
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
      // (indirect-params, lower-side indirect, etc.) at instantiation
      // time rather than at call time. Captures FlatSig so the post-return
      // signature check can compare against flatten_functype({}, $ft,
      // 'lift').results (spec L3292).
      CanonicalABI::CanonCtx PrefCx{nullptr,   nullptr, nullptr,
                                    &CompInst, {},      {}};
      EXPECTED_TRY(auto FlatSig,
                   CanonicalABI::flattenFuncType(
                       PrefCx, DType->getFuncType(), /*IsLift=*/true,
                       {AsyncLift, CallbackFunc != nullptr}));

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
      auto Lifted =
          std::make_unique<Runtime::Instance::Component::FunctionInstance>(
              DType->getFuncType(), FuncInst, MemInst, ReallocFunc, &CompInst,
              PostReturnFunc, Enc);
      Lifted->setAsyncOptions(AsyncLift, CallbackFunc, AlwaysTaskReturn);
      CompInst.addFunction(std::move(Lifted));
      break;
    }
    case ComponentCanonOpCode::Lower: {
      // canon lower: synthesize a core wasm function whose body lifts core
      // args to component values, calls the wrapped component function, and
      // lowers the result back.
      const auto &Opts = Canon.getOptions();
      Runtime::Instance::MemoryInstance *MemInst = nullptr;
      Runtime::Instance::FunctionInstance *ReallocFunc = nullptr;
      StringEncoding Enc = StringEncoding::UTF8;
      bool AsyncLower = false;
      for (auto &Opt : Opts) {
        switch (Opt.getCode()) {
        case ComponentCanonOptCode::Encode_UTF8:
        case ComponentCanonOptCode::Encode_UTF16:
        case ComponentCanonOptCode::Encode_Latin1:
          Enc = toStringEncoding(Opt.getCode());
          break;
        case ComponentCanonOptCode::Memory:
          MemInst = CompInst.getCoreMemory(Opt.getIndex());
          if (MemInst != nullptr &&
              MemInst->getMemoryType().getLimit().is64()) {
            spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
            spdlog::error(
                "    canonical ABI over a 64-bit memory is not implemented"sv);
            return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
          }
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
          AsyncLower = true;
          break;
        default:
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    canon lower: unsupported canonical option"sv);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
        }
      }

      auto *Callee = CompInst.getFunction(Canon.getIndex());
      if (Callee == nullptr) {
        spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
        spdlog::error("    canon lower: function {} not found"sv,
                      Canon.getIndex());
        return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
      }
      const auto &CFT = Callee->getFuncType();

      // Pre-flight the lower-direction flat ABI so unsupported shapes (async,
      // gated types) fail at instantiation time. flattenFuncType doesn't need
      // Mem / Realloc to compute the signature. The callee's function type
      // carries type indices of the callee's own instance.
      CanonicalABI::CanonCtx PrefCx{this, nullptr, nullptr, &CompInst,
                                    {},   {},      nullptr};
      if (const auto *CalleeComp = Callee->getComponentInstance();
          CalleeComp != nullptr && CalleeComp != &CompInst) {
        PrefCx.TypeResolver = [CalleeComp](uint32_t I) {
          return CalleeComp->getType(I);
        };
        PrefCx.ResourceResolver = [CalleeComp](uint32_t I) {
          return CalleeComp->getTypeResource(I);
        };
      }
      EXPECTED_TRY(auto FlatSig, CanonicalABI::flattenFuncType(
                                     PrefCx, CFT,
                                     /*IsLift=*/false, {AsyncLower, false}));

      auto Thunk = std::make_unique<CanonLowerHostFunc>(
          this, FlatSig, Callee, MemInst, ReallocFunc, &CompInst, Enc,
          AsyncLower);
      // Register via the host-function helper so the synthesized core
      // function's defined type lands in a ModuleInstance::Types list and
      // matchType walks find it when wasm callers import this function.
      CompInst.addCoreHostFunction(std::move(Thunk));
      break;
    }
    case ComponentCanonOpCode::Resource__new:
    case ComponentCanonOpCode::Resource__drop:
    case ComponentCanonOpCode::Resource__drop_async:
    case ComponentCanonOpCode::Resource__rep: {
      const auto *RT = CompInst.getTypeResource(Canon.getIndex());
      if (RT == nullptr) {
        spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
        spdlog::error("    canon resource built-in: type {} has no runtime "
                      "resource"sv,
                      Canon.getIndex());
        return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
      }
      std::unique_ptr<Runtime::HostFunctionBase> Thunk;
      std::string_view Name;
      switch (Canon.getOpCode()) {
      case ComponentCanonOpCode::Resource__new:
        Thunk = std::make_unique<CanonResourceNewHostFunc>(&CompInst, RT);
        Name = "$resource-new"sv;
        break;
      case ComponentCanonOpCode::Resource__rep:
        Thunk = std::make_unique<CanonResourceRepHostFunc>(&CompInst, RT);
        Name = "$resource-rep"sv;
        break;
      default:
        Thunk =
            std::make_unique<CanonResourceDropHostFunc>(this, &CompInst, RT);
        Name = "$resource-drop"sv;
        break;
      }
      CompInst.addCoreHostFunction(std::move(Thunk), Name);
      break;
    }
    case ComponentCanonOpCode::Backpressure__set:
    case ComponentCanonOpCode::Backpressure__inc:
    case ComponentCanonOpCode::Backpressure__dec:
    case ComponentCanonOpCode::Thread__index:
    case ComponentCanonOpCode::Task__return:
    case ComponentCanonOpCode::Task__cancel:
    case ComponentCanonOpCode::Context__get:
    case ComponentCanonOpCode::Context__set:
    case ComponentCanonOpCode::Yield:
    case ComponentCanonOpCode::Subtask__cancel:
    case ComponentCanonOpCode::Subtask__drop:
    case ComponentCanonOpCode::Stream__new:
    case ComponentCanonOpCode::Stream__read:
    case ComponentCanonOpCode::Stream__write:
    case ComponentCanonOpCode::Stream__cancel_read:
    case ComponentCanonOpCode::Stream__cancel_write:
    case ComponentCanonOpCode::Stream__close_readable:
    case ComponentCanonOpCode::Stream__close_writable:
    case ComponentCanonOpCode::Future__new:
    case ComponentCanonOpCode::Future__read:
    case ComponentCanonOpCode::Future__write:
    case ComponentCanonOpCode::Future__cancel_read:
    case ComponentCanonOpCode::Future__cancel_write:
    case ComponentCanonOpCode::Future__close_readable:
    case ComponentCanonOpCode::Future__close_writable:
    case ComponentCanonOpCode::Error_context__new:
    case ComponentCanonOpCode::Error_context__debug_message:
    case ComponentCanonOpCode::Error_context__drop:
    case ComponentCanonOpCode::Waitable_set__new:
    case ComponentCanonOpCode::Waitable_set__wait:
    case ComponentCanonOpCode::Waitable_set__poll:
    case ComponentCanonOpCode::Waitable_set__drop:
    case ComponentCanonOpCode::Waitable__join:
    case ComponentCanonOpCode::Thread__new_indirect:
    case ComponentCanonOpCode::Thread__resume_later:
    case ComponentCanonOpCode::Thread__suspend:
    case ComponentCanonOpCode::Thread__yield_then_resume:
    case ComponentCanonOpCode::Thread__suspend_then_resume: {
      AsyncBuiltinInfo Info;
      Info.Code = Canon.getOpCode();
      Info.Inst = &CompInst;
      Info.Cancellable = Canon.isAsync();
      Info.CtxIdx = Canon.getConstVal();
      // Resolve the memory for built-ins that touch linear memory: the
      // wait/poll built-ins carry a direct memory index, the others use the
      // memory canonical option.
      if (Canon.getOpCode() == ComponentCanonOpCode::Waitable_set__wait ||
          Canon.getOpCode() == ComponentCanonOpCode::Waitable_set__poll) {
        Info.Mem = CompInst.getCoreMemory(Canon.getIndex());
      }
      for (const auto &Opt : Canon.getOptions()) {
        switch (Opt.getCode()) {
        case ComponentCanonOptCode::Memory:
          Info.Mem = CompInst.getCoreMemory(Opt.getIndex());
          break;
        case ComponentCanonOptCode::Realloc:
          Info.Realloc = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case ComponentCanonOptCode::Encode_UTF8:
        case ComponentCanonOptCode::Encode_UTF16:
        case ComponentCanonOptCode::Encode_Latin1:
          Info.Enc = toStringEncoding(Opt.getCode());
          break;
        case ComponentCanonOptCode::Async:
          Info.Cancellable = true;
          break;
        default:
          break;
        }
      }
      // stream/future built-ins: resolve the element type declared by the
      // type immediate.
      switch (Canon.getOpCode()) {
      case ComponentCanonOpCode::Stream__new:
      case ComponentCanonOpCode::Stream__read:
      case ComponentCanonOpCode::Stream__write:
      case ComponentCanonOpCode::Stream__cancel_read:
      case ComponentCanonOpCode::Stream__cancel_write:
      case ComponentCanonOpCode::Stream__close_readable:
      case ComponentCanonOpCode::Stream__close_writable: {
        const auto *DT = CompInst.getType(Canon.getIndex());
        if (DT != nullptr && DT->isDefValType() &&
            DT->getDefValType().isStreamTy()) {
          Info.Elem = DT->getDefValType().getStream().ValTy;
        }
        Info.IsStream = true;
        break;
      }
      case ComponentCanonOpCode::Future__new:
      case ComponentCanonOpCode::Future__read:
      case ComponentCanonOpCode::Future__write:
      case ComponentCanonOpCode::Future__cancel_read:
      case ComponentCanonOpCode::Future__cancel_write:
      case ComponentCanonOpCode::Future__close_readable:
      case ComponentCanonOpCode::Future__close_writable: {
        const auto *DT = CompInst.getType(Canon.getIndex());
        if (DT != nullptr && DT->isDefValType() &&
            DT->getDefValType().isFutureTy()) {
          Info.Elem = DT->getDefValType().getFuture().ValTy;
        }
        Info.IsStream = false;
        break;
      }
      case ComponentCanonOpCode::Task__return:
        for (const auto &R : Canon.getResultList()) {
          Info.RetTypes.push_back(R.getValType());
        }
        break;
      case ComponentCanonOpCode::Thread__new_indirect:
        Info.Table = CompInst.getCoreTable(Canon.getTargetIndex());
        break;
      default:
        break;
      }
      auto Thunk =
          std::make_unique<CanonAsyncBuiltinHostFunc>(this, std::move(Info));
      CompInst.addCoreHostFunction(std::move(Thunk), "$async-builtin"sv);
      break;
    }
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
