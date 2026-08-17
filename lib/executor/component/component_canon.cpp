// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/canonical_abi.h"
#include "executor/component/executor.h"
#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <cstdint>
#include <string_view>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;

namespace {
// Map a canon `string-encoding` option code to the runtime StringEncoding.
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

Expect<std::vector<ValVariant>> ComponentExecutor::convValsToCoreWASM(
    Span<const ComponentValVariant> Vals, Span<const ComponentValType> ValTypes,
    const Runtime::Component::CanonOptions &Opts) {
  // Wrapper over the spec's lower_flat_values (Component::CanonicalABI.md
  // L3212-3232).
  Component::CanonicalABI::Context Cx{Opts, this};
  return Component::CanonicalABI::lowerFlatValues(
      Cx, Vals, ValTypes, Component::CanonicalABI::MaxFlatParams);
}

Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
ComponentExecutor::convValsToComponent(
    Span<const std::pair<ValVariant, ValType>> CoreVals,
    Span<const ComponentValType> ValTypes,
    const Runtime::Component::CanonOptions &Opts) {
  // Wrapper over the spec's lift_flat_values (Component::CanonicalABI.md
  // L3193-3202).
  Component::CanonicalABI::Context Cx{Opts, this};
  Cx.Realloc = nullptr;
  Component::CanonicalABI::FlatIter VI(CoreVals);
  EXPECTED_TRY(auto Lifted,
               Component::CanonicalABI::liftFlatValues(
                   Cx, VI, ValTypes, Component::CanonicalABI::MaxFlatResults));
  std::vector<std::pair<ComponentValVariant, ComponentValType>> Out;
  Out.reserve(Lifted.size());
  for (size_t I = 0; I < Lifted.size(); ++I) {
    Out.emplace_back(std::move(Lifted[I]), ValTypes[I]);
  }
  return Out;
}

Expect<void>
ComponentExecutor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
                               const AST::Component::CanonSection &CanonSec) {
  for (const auto &Canon : CanonSec.getContent()) {
    switch (Canon.getOpCode()) {
    case ComponentCanonOpCode::Lift: {
      // Lift wraps a core function as a component function under the canon ABI.
      Runtime::Component::CanonOptions CanonOpts{&CompInst};
      for (auto &Opt : Canon.getOptions()) {
        switch (Opt.getCode()) {
        case ComponentCanonOptCode::Encode_UTF8:
        case ComponentCanonOptCode::Encode_UTF16:
        case ComponentCanonOptCode::Encode_Latin1:
          CanonOpts.Enc = toStringEncoding(Opt.getCode());
          break;
        case ComponentCanonOptCode::Memory:
          CanonOpts.Mem = CompInst.getCoreMemory(Opt.getIndex());
          if (CanonOpts.Mem != nullptr &&
              CanonOpts.Mem->getMemoryType().getLimit().is64()) {
            spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
            spdlog::error(
                "    canonical ABI over a 64-bit memory is not implemented"sv);
            return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
          }
          break;
        case ComponentCanonOptCode::Realloc:
          CanonOpts.Realloc = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case ComponentCanonOptCode::PostReturn:
          CanonOpts.PostReturn = CompInst.getCoreFunction(Opt.getIndex());
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
        // Lifting a non-function is rejected earlier, so this is unlikely.
        spdlog::error(ErrCode::Value::InvalidCanonOption);
        spdlog::error("    Cannot lift a non-function"sv);
        return Unexpect(ErrCode::Value::InvalidCanonOption);
      }
      // Pre-flight the ABI signature so a gated shape fails at instantiation.
      Component::CanonicalABI::Context PrefCx{{&CompInst}};
      EXPECTED_TRY(auto FlatSig, Component::CanonicalABI::flattenFuncType(
                                     PrefCx, DType->getFuncType(),
                                     /*IsLift=*/true));

      // post-return takes the flat results; the validator may skip this.
      if (CanonOpts.PostReturn != nullptr) {
        const auto &PRType = CanonOpts.PostReturn->getFuncType();
        if (!PRType.getReturnTypes().empty() ||
            PRType.getParamTypes().size() != FlatSig.getReturnTypes().size()) {
          spdlog::error(ErrCode::Value::InvalidCanonOption);
          spdlog::error("    canon lift: post-return must have signature "
                        "(func (param ...flatten_lift_results))"sv);
          return Unexpect(ErrCode::Value::InvalidCanonOption);
        }
        for (size_t I = 0; I < FlatSig.getReturnTypes().size(); ++I) {
          if (PRType.getParamTypes()[I].getCode() !=
              FlatSig.getReturnTypes()[I].getCode()) {
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
              DType->getFuncType(), FuncInst, CanonOpts));
      break;
    }
    case ComponentCanonOpCode::Lower: {
      // canon lower: synthesize the core function wrapping the callee.
      Runtime::Component::CanonOptions CanonOpts{&CompInst};
      for (auto &Opt : Canon.getOptions()) {
        switch (Opt.getCode()) {
        case ComponentCanonOptCode::Encode_UTF8:
        case ComponentCanonOptCode::Encode_UTF16:
        case ComponentCanonOptCode::Encode_Latin1:
          CanonOpts.Enc = toStringEncoding(Opt.getCode());
          break;
        case ComponentCanonOptCode::Memory:
          CanonOpts.Mem = CompInst.getCoreMemory(Opt.getIndex());
          if (CanonOpts.Mem != nullptr &&
              CanonOpts.Mem->getMemoryType().getLimit().is64()) {
            spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
            spdlog::error(
                "    canonical ABI over a 64-bit memory is not implemented"sv);
            return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
          }
          break;
        case ComponentCanonOptCode::Realloc:
          CanonOpts.Realloc = CompInst.getCoreFunction(Opt.getIndex());
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
      if (Callee == nullptr) {
        spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
        spdlog::error("    canon lower: function {} not found"sv,
                      Canon.getIndex());
        return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
      }
      const auto &CFT = Callee->getFuncType();

      // Pre-flight the lower-direction flat ABI in the callee's index space.
      Component::CanonicalABI::Context PrefCx{{&CompInst}, this};
      if (const auto *CalleeComp = Callee->getComponentInstance();
          CalleeComp != nullptr && CalleeComp != &CompInst) {
        PrefCx.TypeResolver = [CalleeComp](uint32_t I) {
          return CalleeComp->getType(I);
        };
        PrefCx.ResourceResolver = [CalleeComp](uint32_t I) {
          return CalleeComp->getTypeResource(I);
        };
      }
      EXPECTED_TRY(auto FlatSig,
                   Component::CanonicalABI::flattenFuncType(PrefCx, CFT,
                                                            /*IsLift=*/false));

      auto HostFunc = std::make_unique<Component::CanonLowerHostFunc>(
          this, std::move(FlatSig), Callee, CanonOpts);
      // Register through the helper so matchType can walk the synthesized type.
      CompInst.addCoreHostFunction(std::move(HostFunc));
      break;
    }
    case ComponentCanonOpCode::Resource__new:
    case ComponentCanonOpCode::Resource__drop:
    case ComponentCanonOpCode::Resource__rep: {
      const auto *RT = CompInst.getTypeResource(Canon.getIndex());
      if (RT == nullptr) {
        spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
        spdlog::error("    canon resource built-in: type {} has no runtime "
                      "resource"sv,
                      Canon.getIndex());
        return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
      }
      std::unique_ptr<Runtime::HostFunctionBase> HostFunc;
      std::string_view Name;
      switch (Canon.getOpCode()) {
      case ComponentCanonOpCode::Resource__new:
        HostFunc = std::make_unique<Component::CanonResourceNewHostFunc>(
            &CompInst, RT);
        Name = "$resource-new"sv;
        break;
      case ComponentCanonOpCode::Resource__rep:
        HostFunc = std::make_unique<Component::CanonResourceRepHostFunc>(
            &CompInst, RT);
        Name = "$resource-rep"sv;
        break;
      default:
        HostFunc = std::make_unique<Component::CanonResourceDropHostFunc>(
            this, &CompInst, RT);
        Name = "$resource-drop"sv;
        break;
      }
      CompInst.addCoreHostFunction(std::move(HostFunc), Name);
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
