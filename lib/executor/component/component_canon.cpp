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
          break;
        case ComponentCanonOptCode::Realloc:
          CanonOpts.Realloc = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case ComponentCanonOptCode::PostReturn:
          CanonOpts.PostReturn = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case ComponentCanonOptCode::Async:
          CanonOpts.Async = true;
          break;
        case ComponentCanonOptCode::Callback:
          CanonOpts.Callback = CompInst.getCoreFunction(Opt.getIndex());
          break;
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
      EXPECTED_TRY(auto FlatSig,
                   Component::CanonicalABI::flattenFuncType(
                       PrefCx, DType->getFuncType(), /*IsLift=*/true,
                       CanonOpts.Async, CanonOpts.Callback != nullptr));

      // post-return takes the flat results and returns nothing.
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
          std::make_unique<Runtime::Instance::ComponentFunctionInstance>(
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
          break;
        case ComponentCanonOptCode::Realloc:
          CanonOpts.Realloc = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case ComponentCanonOptCode::PostReturn:
          // post-return is only valid on canon lift.
          spdlog::error(ErrCode::Value::InvalidCanonOption);
          spdlog::error(
              "    canon lower: 'post-return' is only allowed on canon lift"sv);
          return Unexpect(ErrCode::Value::InvalidCanonOption);
        case ComponentCanonOptCode::Async:
          CanonOpts.Async = true;
          break;
        default:
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    canon lower: unsupported canonical option"sv);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
        }
      }

      auto *Callee = CompInst.getFunction(Canon.getIndex());
      if (Callee == nullptr) {
        spdlog::error(ErrCode::Value::ComponentFunctionIndexOutOfBounds);
        spdlog::error("    canon lower: function {} not found"sv,
                      Canon.getIndex());
        return Unexpect(ErrCode::Value::ComponentFunctionIndexOutOfBounds);
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
      EXPECTED_TRY(auto FlatSig, Component::CanonicalABI::flattenFuncType(
                                     PrefCx, CFT,
                                     /*IsLift=*/false, CanonOpts.Async));

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
        spdlog::error(ErrCode::Value::ComponentExpectedResource);
        spdlog::error("    canon resource built-in: type {} has no runtime "
                      "resource"sv,
                      Canon.getIndex());
        return Unexpect(ErrCode::Value::ComponentExpectedResource);
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
    case ComponentCanonOpCode::Stream__drop_readable:
    case ComponentCanonOpCode::Stream__drop_writable:
    case ComponentCanonOpCode::Future__new:
    case ComponentCanonOpCode::Future__read:
    case ComponentCanonOpCode::Future__write:
    case ComponentCanonOpCode::Future__cancel_read:
    case ComponentCanonOpCode::Future__cancel_write:
    case ComponentCanonOpCode::Future__drop_readable:
    case ComponentCanonOpCode::Future__drop_writable:
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
    case ComponentCanonOpCode::Thread__suspend_then_resume:
    case ComponentCanonOpCode::Thread__yield_then_promote:
    case ComponentCanonOpCode::Thread__suspend_then_promote: {
      Component::AsyncBuiltinInfo Info;
      Info.Code = Canon.getOpCode();
      Info.Opts.Inst = &CompInst;
      Info.Flag = Canon.getFlagImmediate();
      Info.ContextIdx = Canon.getConstVal();
      Info.ContextType = Canon.getContextType();
      // wait/poll carry a direct memory index; the others use the option.
      if (Canon.getOpCode() == ComponentCanonOpCode::Waitable_set__wait ||
          Canon.getOpCode() == ComponentCanonOpCode::Waitable_set__poll) {
        Info.Opts.Mem = CompInst.getCoreMemory(Canon.getIndex());
      }
      for (const auto &Opt : Canon.getOptions()) {
        switch (Opt.getCode()) {
        case ComponentCanonOptCode::Memory:
          Info.Opts.Mem = CompInst.getCoreMemory(Opt.getIndex());
          break;
        case ComponentCanonOptCode::Realloc:
          Info.Opts.Realloc = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case ComponentCanonOptCode::Encode_UTF8:
        case ComponentCanonOptCode::Encode_UTF16:
        case ComponentCanonOptCode::Encode_Latin1:
          Info.Opts.Enc = toStringEncoding(Opt.getCode());
          break;
        case ComponentCanonOptCode::Async:
          Info.Flag = true;
          break;
        default:
          break;
        }
      }
      // stream/future: resolve the element type from the type immediate.
      switch (Canon.getOpCode()) {
      case ComponentCanonOpCode::Stream__new:
      case ComponentCanonOpCode::Stream__read:
      case ComponentCanonOpCode::Stream__write:
      case ComponentCanonOpCode::Stream__cancel_read:
      case ComponentCanonOpCode::Stream__cancel_write:
      case ComponentCanonOpCode::Stream__drop_readable:
      case ComponentCanonOpCode::Stream__drop_writable: {
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
      case ComponentCanonOpCode::Future__drop_readable:
      case ComponentCanonOpCode::Future__drop_writable: {
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
      auto HostFunc = std::make_unique<Component::CanonAsyncBuiltinHostFunc>(
          this, std::move(Info));
      CompInst.addCoreHostFunction(std::move(HostFunc), "$async-builtin"sv);
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
