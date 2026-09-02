// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/canonical_abi.h"
#include "executor/component/executor.h"
#include "executor/executor.h"
#include "runtime/component/canonopt.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <cstdint>
#include <string_view>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

namespace {
// Map a canon `string-encoding` option code to the runtime string encoding.
Runtime::Component::StringEncoding
toStringEncoding(ComponentCanonOptCode Code) noexcept {
  switch (Code) {
  case ComponentCanonOptCode::Encode_UTF16:
    return Runtime::Component::StringEncoding::UTF16;
  case ComponentCanonOptCode::Encode_Latin1:
    return Runtime::Component::StringEncoding::Latin1UTF16;
  default:
    return Runtime::Component::StringEncoding::UTF8;
  }
}

// A canonical definition with an option or a shape validation admits nowhere.
Expect<void> logCanonError(ErrCode::Value Code) noexcept {
  spdlog::error(Code);
  spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
  return Unexpect(Code);
}
} // namespace

// Instantiate canonical section. See executor.h.
Expect<void>
ComponentExecutor::instantiate(Component::Instantiator &Ctx,
                               const AST::Component::CanonSection &CanonSec) {
  auto &CompInst = Ctx.getInstance();
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
        case ComponentCanonOptCode::Memory: {
          EXPECTED_TRY(CanonOpts.Mem, Ctx.getCoreMemory(Opt.getIndex()));
          break;
        }
        case ComponentCanonOptCode::Realloc: {
          EXPECTED_TRY(CanonOpts.Realloc, Ctx.getCoreFunction(Opt.getIndex()));
          break;
        }
        case ComponentCanonOptCode::PostReturn: {
          EXPECTED_TRY(CanonOpts.PostReturn,
                       Ctx.getCoreFunction(Opt.getIndex()));
          break;
        }
        case ComponentCanonOptCode::Async:
          CanonOpts.Async = true;
          break;
        case ComponentCanonOptCode::Callback: {
          EXPECTED_TRY(CanonOpts.Callback, Ctx.getCoreFunction(Opt.getIndex()));
          break;
        }
        default:
          return logCanonError(ErrCode::Value::ComponentNotImplInstantiate);
        }
      }

      EXPECTED_TRY(const auto *Def, CompInst.getType(Canon.getTargetIndex()));
      if (unlikely(Def == nullptr || !Def->isFuncType())) {
        // Lifting a non-function is rejected by validation.
        return logCanonError(ErrCode::Value::InvalidCanonOption);
      }
      // Pre-flight the ABI signature so a gated shape fails at instantiation.
      Component::LiftLowerContext PrefCtx{{&CompInst}};
      EXPECTED_TRY(auto FlatSig,
                   PrefCtx.flattenFuncType(Def->getFuncType(), /*IsLift=*/true,
                                           CanonOpts.Async,
                                           CanonOpts.Callback != nullptr));

      // post-return takes the flat results and returns nothing.
      if (CanonOpts.PostReturn != nullptr) {
        const auto &PostReturnType = CanonOpts.PostReturn->getFuncType();
        if (!PostReturnType.getReturnTypes().empty() ||
            PostReturnType.getParamTypes() != FlatSig.getReturnTypes()) {
          spdlog::error(ErrCode::Value::InvalidCanonOption);
          spdlog::error(ErrInfo::InfoMismatch(FlatSig.getReturnTypes(), {},
                                              PostReturnType.getParamTypes(),
                                              PostReturnType.getReturnTypes()));
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CanonOpt));
          return Unexpect(ErrCode::Value::InvalidCanonOption);
        }
      }

      EXPECTED_TRY(auto *CoreFunc, Ctx.getCoreFunction(Canon.getIndex()));
      Ctx.addFunction(
          std::make_unique<Runtime::Instance::ComponentFunctionInstance>(
              Def->getFuncType(), CoreFunc, CanonOpts));
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
        case ComponentCanonOptCode::Memory: {
          EXPECTED_TRY(CanonOpts.Mem, Ctx.getCoreMemory(Opt.getIndex()));
          break;
        }
        case ComponentCanonOptCode::Realloc: {
          EXPECTED_TRY(CanonOpts.Realloc, Ctx.getCoreFunction(Opt.getIndex()));
          break;
        }
        case ComponentCanonOptCode::PostReturn:
          // post-return is only valid on canon lift.
          return logCanonError(ErrCode::Value::InvalidCanonOption);
        case ComponentCanonOptCode::Async:
          CanonOpts.Async = true;
          break;
        default:
          return logCanonError(ErrCode::Value::ComponentNotImplInstantiate);
        }
      }

      EXPECTED_TRY(auto *Callee, Ctx.getFunction(Canon.getIndex()));

      // Pre-flight the lower-direction flat ABI in the callee's index space.
      Component::LiftLowerContext PrefCtx{{&CompInst}, this};
      if (const auto *CalleeComp = Callee->getComponentInstance();
          CalleeComp != nullptr && CalleeComp != &CompInst) {
        PrefCtx.setTypeInstance(CalleeComp);
      }
      EXPECTED_TRY(auto FlatSig,
                   PrefCtx.flattenFuncType(Callee->getFuncType(),
                                           /*IsLift=*/false, CanonOpts.Async));

      auto HostFunc = std::make_unique<Component::CanonLowerHostFunc>(
          this, std::move(FlatSig), Callee, CanonOpts);
      Ctx.addCoreHostFunction(std::move(HostFunc));
      break;
    }
    case ComponentCanonOpCode::Resource__new:
    case ComponentCanonOpCode::Resource__drop:
    case ComponentCanonOpCode::Resource__rep: {
      // Validation guarantees the type index names a resource type.
      EXPECTED_TRY(const auto *ResType,
                   CompInst.getTypeResource(Canon.getIndex()));
      assuming(ResType != nullptr);
      std::unique_ptr<Runtime::HostFunctionBase> HostFunc;
      std::string_view Name;
      switch (Canon.getOpCode()) {
      case ComponentCanonOpCode::Resource__new:
        HostFunc = std::make_unique<Component::CanonResourceNewHostFunc>(
            &CompInst, ResType);
        Name = "$resource-new"sv;
        break;
      case ComponentCanonOpCode::Resource__rep:
        HostFunc = std::make_unique<Component::CanonResourceRepHostFunc>(
            &CompInst, ResType);
        Name = "$resource-rep"sv;
        break;
      default:
        HostFunc = std::make_unique<Component::CanonResourceDropHostFunc>(
            this, &CompInst, ResType);
        Name = "$resource-drop"sv;
        break;
      }
      Ctx.addCoreHostFunction(std::move(HostFunc));
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
        EXPECTED_TRY(Info.Opts.Mem, Ctx.getCoreMemory(Canon.getIndex()));
      }
      for (const auto &Opt : Canon.getOptions()) {
        switch (Opt.getCode()) {
        case ComponentCanonOptCode::Memory: {
          EXPECTED_TRY(Info.Opts.Mem, Ctx.getCoreMemory(Opt.getIndex()));
          break;
        }
        case ComponentCanonOptCode::Realloc: {
          EXPECTED_TRY(Info.Opts.Realloc, Ctx.getCoreFunction(Opt.getIndex()));
          break;
        }
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
      case ComponentCanonOpCode::Stream__drop_writable:
      case ComponentCanonOpCode::Future__new:
      case ComponentCanonOpCode::Future__read:
      case ComponentCanonOpCode::Future__write:
      case ComponentCanonOpCode::Future__cancel_read:
      case ComponentCanonOpCode::Future__cancel_write:
      case ComponentCanonOpCode::Future__drop_readable:
      case ComponentCanonOpCode::Future__drop_writable: {
        Info.IsStream = Canon.getOpCode() < ComponentCanonOpCode::Future__new;
        EXPECTED_TRY(const auto *TypeDef,
                     CompInst.getTypeDefinition(Canon.getIndex()));
        const auto *Def = TypeDef->Def;
        if (Def != nullptr && Def->isDefValType()) {
          const auto &ValTy = Def->getDefValType();
          if (Info.IsStream && ValTy.isStreamTy()) {
            Info.Elem = ValTy.getStream().ValTy;
            Info.ElemInst = TypeDef->Owner;
          } else if (!Info.IsStream && ValTy.isFutureTy()) {
            Info.Elem = ValTy.getFuture().ValTy;
            Info.ElemInst = TypeDef->Owner;
          }
        }
        break;
      }
      case ComponentCanonOpCode::Task__return:
        for (const auto &Result : Canon.getResultList()) {
          Info.RetTypes.push_back(Result.getValType());
        }
        break;
      case ComponentCanonOpCode::Thread__new_indirect: {
        EXPECTED_TRY(Info.Table, Ctx.getCoreTable(Canon.getTargetIndex()));
        break;
      }
      default:
        break;
      }
      auto HostFunc = std::make_unique<Component::CanonAsyncBuiltinHostFunc>(
          this, std::move(Info));
      Ctx.addCoreHostFunction(std::move(HostFunc));
      break;
    }
    default:
      return logCanonError(ErrCode::Value::ComponentNotImplInstantiate);
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
