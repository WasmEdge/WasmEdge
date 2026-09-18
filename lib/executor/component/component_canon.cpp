// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

namespace Component {

// Run a canonical built-in. See "include/executor/component/canon.h".
Expect<void> CanonFunction::run(const Runtime::CallingFrame &,
                                Span<const ValVariant> Args,
                                Span<ValVariant> Rets) {
  return Exec.runCanon(*this, Args, Rets);
}

} // namespace Component

// Resolve the canonical options. See "include/executor/component/executor.h".
Expect<Runtime::Component::CanonOptions> ComponentExecutor::getCanonOptions(
    const Runtime::Instance::ComponentInstance &CompInst,
    const AST::Component::Canonical &Canon) {
  Runtime::Component::CanonOptions Opts;
  Opts.Inst = &CompInst;
  for (const auto &Opt : Canon.getOptions()) {
    switch (Opt.getCode()) {
    case ComponentCanonOptCode::Encode_UTF8:
      Opts.Encoding = Runtime::Component::StringEncoding::UTF8;
      break;
    case ComponentCanonOptCode::Encode_UTF16:
      Opts.Encoding = Runtime::Component::StringEncoding::UTF16;
      break;
    case ComponentCanonOptCode::Encode_Latin1:
      Opts.Encoding = Runtime::Component::StringEncoding::Latin1UTF16;
      break;
    case ComponentCanonOptCode::Memory: {
      EXPECTED_TRY(Opts.Mem, CompInst.getCoreMemory(Opt.getIndex()));
      break;
    }
    case ComponentCanonOptCode::Realloc: {
      EXPECTED_TRY(Opts.Realloc, CompInst.getCoreFunction(Opt.getIndex()));
      break;
    }
    case ComponentCanonOptCode::PostReturn: {
      EXPECTED_TRY(Opts.PostReturn, CompInst.getCoreFunction(Opt.getIndex()));
      break;
    }
    case ComponentCanonOptCode::Async:
      Opts.Async = true;
      break;
    case ComponentCanonOptCode::Callback: {
      EXPECTED_TRY(Opts.Callback, CompInst.getCoreFunction(Opt.getIndex()));
      break;
    }
    default:
      spdlog::error(ErrCode::Value::InvalidCanonOption);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CanonOpt));
      return Unexpect(ErrCode::Value::InvalidCanonOption);
    }
  }
  return Opts;
}

// Flatten a function type. See "include/executor/component/executor.h".
Expect<AST::FunctionType> ComponentExecutor::flattenFuncType(
    const Runtime::Instance::ComponentInstance &TypeInst,
    const AST::Component::FuncType &FuncType,
    const Runtime::Component::CanonOptions &Opts) {
  const bool IsMem64 = Opts.isMemory64();
  const ValType Ptr(IsMem64 ? TypeCode::I64 : TypeCode::I32);
  std::vector<ValType> Params, Results;
  for (const auto &Param : FuncType.getParamList()) {
    EXPECTED_TRY(flattenType(TypeInst, Param.getValType(), IsMem64, Params));
  }
  for (const auto &Result : FuncType.getResultList()) {
    EXPECTED_TRY(flattenType(TypeInst, Result.getValType(), IsMem64, Results));
  }
  // Past the limit the parameters go through memory and a result pointer is
  // the last parameter; an async lower returns the packed subtask state.
  const bool ParamsIndirect =
      Params.size() > (Opts.Async ? MaxFlatAsyncParams : MaxFlatParams);
  const bool ResultsIndirect =
      Opts.Async ? !Results.empty() : Results.size() > MaxFlatResults;
  if (ParamsIndirect) {
    Params.assign(1, Ptr);
  }
  if (ResultsIndirect) {
    Params.push_back(Ptr);
    Results.clear();
  }
  if (Opts.Async) {
    Results.assign(1, ValType(TypeCode::I32));
  }
  return AST::FunctionType(Params, Results);
}

// Instantiate canon section. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
                               const AST::Component::CanonSection &CanonSec) {
  const ValType I32Type(TypeCode::I32);
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return E;
  };
  auto Fail = [](ErrCode::Value Code) -> Expect<void> {
    spdlog::error(Code);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(Code);
  };
  for (const auto &Canon : CanonSec.getContent()) {
    const auto Code = Canon.getOpCode();
    EXPECTED_TRY(auto Opts,
                 getCanonOptions(CompInst, Canon).map_error(ReportError));
    const bool IsMem64 = Opts.isMemory64();
    const ValType Ptr(IsMem64 ? TypeCode::I64 : TypeCode::I32);

    // A canonical function of the core type the built-in has, with the
    // options and the immediates resolved.
    auto Builtin = [&](AST::FunctionType CoreType) {
      auto Func = std::make_unique<Component::CanonFunction>(
          *this, Code, Opts, std::move(CoreType));
      Func->setFlag(Canon.getFlagImmediate());
      Func->setConstVal(Canon.getConstVal());
      Func->setMemory64(IsMem64);
      return Func;
    };
    auto SharedType = [&](const ValType &AddrType) {
      auto [Params, Results] =
          AST::Component::Canonical::getBuiltinCoreFuncType(Code, AddrType);
      return AST::FunctionType(Params, Results);
    };

    switch (Code) {
    case ComponentCanonOpCode::Lift: {
      EXPECTED_TRY(
          auto *CoreFunc,
          CompInst.getCoreFunction(Canon.getIndex()).map_error(ReportError));
      EXPECTED_TRY(const auto *TypeDef,
                   CompInst.getTypeDefinition(Canon.getTargetIndex())
                       .map_error(ReportError));
      if (TypeDef->Def == nullptr || !TypeDef->Def->isFuncType()) {
        return Fail(ErrCode::Value::InvalidTypeReference);
      }
      CompInst.addFunction(
          std::make_unique<Runtime::Instance::ComponentFunctionInstance>(
              TypeDef->Def->getFuncType(), TypeDef->Owner, CoreFunc, Opts));
      break;
    }
    case ComponentCanonOpCode::Lower: {
      EXPECTED_TRY(
          auto *Callee,
          CompInst.getFunction(Canon.getIndex()).map_error(ReportError));
      EXPECTED_TRY(auto CoreType, flattenFuncType(*Callee->getTypeInstance(),
                                                  Callee->getFuncType(), Opts)
                                      .map_error(ReportError));
      auto Func = Builtin(std::move(CoreType));
      Func->setCallee(Callee);
      CompInst.addCoreHostFunction(std::move(Func));
      break;
    }
    case ComponentCanonOpCode::Resource__new:
    case ComponentCanonOpCode::Resource__rep:
    case ComponentCanonOpCode::Resource__drop: {
      EXPECTED_TRY(
          const auto *ResType,
          CompInst.getTypeResource(Canon.getIndex()).map_error(ReportError));
      if (ResType == nullptr) {
        return Fail(ErrCode::Value::InvalidTypeReference);
      }
      const ValType Rep = ResType->getRepType();
      AST::FunctionType CoreType;
      if (Code == ComponentCanonOpCode::Resource__new) {
        CoreType = AST::FunctionType({Rep}, {I32Type});
      } else if (Code == ComponentCanonOpCode::Resource__rep) {
        CoreType = AST::FunctionType({I32Type}, {Rep});
      } else {
        CoreType = AST::FunctionType({I32Type}, {});
      }
      auto Func = Builtin(std::move(CoreType));
      Func->setResource(ResType);
      CompInst.addCoreHostFunction(std::move(Func));
      break;
    }
    case ComponentCanonOpCode::Task__return: {
      // The results lower as the core parameters, through memory past the
      // limit.
      std::vector<ValType> Params;
      std::vector<ComponentValType> ResultTypes;
      for (const auto &Result : Canon.getResultList()) {
        EXPECTED_TRY(flattenType(CompInst, Result.getValType(), IsMem64, Params)
                         .map_error(ReportError));
        ResultTypes.push_back(Result.getValType());
      }
      if (Params.size() > MaxFlatParams) {
        Params.assign(1, Ptr);
      }
      auto Func = Builtin(AST::FunctionType(Params, {}));
      Func->setResultTypes(std::move(ResultTypes));
      CompInst.addCoreHostFunction(std::move(Func));
      break;
    }
    case ComponentCanonOpCode::Context__get:
      CompInst.addCoreHostFunction(
          Builtin(AST::FunctionType({}, {Canon.getContextType()})));
      break;
    case ComponentCanonOpCode::Context__set:
      CompInst.addCoreHostFunction(
          Builtin(AST::FunctionType({Canon.getContextType()}, {})));
      break;
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
      // The element type of the stream or future type immediate.
      EXPECTED_TRY(
          const auto *TypeDef,
          CompInst.getTypeDefinition(Canon.getIndex()).map_error(ReportError));
      const auto *Def = TypeDef->Def;
      if (Def == nullptr || !Def->isDefValType() ||
          !(Def->getDefValType().isStreamTy() ||
            Def->getDefValType().isFutureTy())) {
        return Fail(ErrCode::Value::InvalidTypeReference);
      }
      auto Func = Builtin(SharedType(Ptr));
      Func->setValType(Def->getDefValType().isStreamTy()
                           ? Def->getDefValType().getStream().ValTy
                           : Def->getDefValType().getFuture().ValTy);
      Func->setTypeInstance(TypeDef->Owner);
      CompInst.addCoreHostFunction(std::move(Func));
      break;
    }
    case ComponentCanonOpCode::Thread__new_indirect: {
      // The thread index carries the address type of the table.
      EXPECTED_TRY(
          auto *Tab,
          CompInst.getCoreTable(Canon.getTargetIndex()).map_error(ReportError));
      const ValType TabAddr(Tab->getTableType().getLimit().is64()
                                ? TypeCode::I64
                                : TypeCode::I32);
      auto Func = Builtin(SharedType(TabAddr));
      Func->setTable(Tab);
      Func->setConstVal(Canon.getIndex());
      CompInst.addCoreHostFunction(std::move(Func));
      break;
    }
    case ComponentCanonOpCode::Waitable_set__wait:
    case ComponentCanonOpCode::Waitable_set__poll: {
      // The event payload goes to the memory of the index immediate, whose
      // address type the pointer argument carries.
      EXPECTED_TRY(
          Opts.Mem,
          CompInst.getCoreMemory(Canon.getIndex()).map_error(ReportError));
      const bool WaitMem64 = Opts.isMemory64();
      const ValType WaitPtr(WaitMem64 ? TypeCode::I64 : TypeCode::I32);
      auto Func = Builtin(SharedType(WaitPtr));
      Func->setMemory64(WaitMem64);
      CompInst.addCoreHostFunction(std::move(Func));
      break;
    }
    case ComponentCanonOpCode::Backpressure__inc:
    case ComponentCanonOpCode::Backpressure__dec:
    case ComponentCanonOpCode::Task__cancel:
    case ComponentCanonOpCode::Yield:
    case ComponentCanonOpCode::Subtask__cancel:
    case ComponentCanonOpCode::Subtask__drop:
    case ComponentCanonOpCode::Error_context__new:
    case ComponentCanonOpCode::Error_context__debug_message:
    case ComponentCanonOpCode::Error_context__drop:
    case ComponentCanonOpCode::Waitable_set__new:
    case ComponentCanonOpCode::Waitable_set__drop:
    case ComponentCanonOpCode::Waitable__join:
    case ComponentCanonOpCode::Thread__index:
    case ComponentCanonOpCode::Thread__resume_later:
    case ComponentCanonOpCode::Thread__suspend:
    case ComponentCanonOpCode::Thread__suspend_then_resume:
    case ComponentCanonOpCode::Thread__yield_then_resume:
    case ComponentCanonOpCode::Thread__suspend_then_promote:
    case ComponentCanonOpCode::Thread__yield_then_promote:
      CompInst.addCoreHostFunction(Builtin(SharedType(Ptr)));
      break;
    default:
      spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
      spdlog::error("    canonical built-in {} is not supported."sv,
                    ComponentCanonOpCodeStr[Code]);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
      return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
    }
  }
  return {};
}

// Dispatch a canonical built-in. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::runCanon(const Component::CanonFunction &Canon,
                                         Span<const ValVariant> Args,
                                         Span<ValVariant> Rets) {
  // A built-in that reaches outside the instance, all but the backpressure
  // and context ones, cannot run while the instance may not be left.
  switch (Canon.getOpCode()) {
  case ComponentCanonOpCode::Resource__rep:
  case ComponentCanonOpCode::Backpressure__inc:
  case ComponentCanonOpCode::Backpressure__dec:
  case ComponentCanonOpCode::Context__get:
  case ComponentCanonOpCode::Context__set:
    break;
  default:
    if (Canon.getInstance() != nullptr && !Canon.getInstance()->mayLeave()) {
      spdlog::error(ErrCode::Value::ComponentCannotLeave);
      return Unexpect(ErrCode::Value::ComponentCannotLeave);
    }
    break;
  }
  switch (Canon.getOpCode()) {
  case ComponentCanonOpCode::Lower:
    return runCanonLower(Canon, Args, Rets);
  case ComponentCanonOpCode::Resource__new:
    return runCanonResourceNew(Canon, Args, Rets);
  case ComponentCanonOpCode::Resource__rep:
    return runCanonResourceRep(Canon, Args, Rets);
  case ComponentCanonOpCode::Resource__drop:
    return runCanonResourceDrop(Canon, Args);
  case ComponentCanonOpCode::Backpressure__inc:
    return runCanonBackpressure(Canon, true);
  case ComponentCanonOpCode::Backpressure__dec:
    return runCanonBackpressure(Canon, false);
  case ComponentCanonOpCode::Task__return:
    return runCanonTaskReturn(Canon, Args);
  case ComponentCanonOpCode::Task__cancel:
    return runCanonTaskCancel(Canon);
  case ComponentCanonOpCode::Context__get:
    return runCanonContextGet(Canon, Rets);
  case ComponentCanonOpCode::Context__set:
    return runCanonContextSet(Canon, Args);
  case ComponentCanonOpCode::Yield:
    return runCanonYield(Canon, Rets);
  case ComponentCanonOpCode::Subtask__cancel:
    return runCanonSubtaskCancel(Canon, Args, Rets);
  case ComponentCanonOpCode::Subtask__drop:
    return runCanonSubtaskDrop(Canon, Args);
  case ComponentCanonOpCode::Stream__new:
    return runCanonStreamNew(Canon, Rets, true);
  case ComponentCanonOpCode::Future__new:
    return runCanonStreamNew(Canon, Rets, false);
  case ComponentCanonOpCode::Stream__read:
    return runCanonStreamCopy(Canon, Args, Rets, true, true);
  case ComponentCanonOpCode::Stream__write:
    return runCanonStreamCopy(Canon, Args, Rets, true, false);
  case ComponentCanonOpCode::Future__read:
    return runCanonStreamCopy(Canon, Args, Rets, false, true);
  case ComponentCanonOpCode::Future__write:
    return runCanonStreamCopy(Canon, Args, Rets, false, false);
  case ComponentCanonOpCode::Stream__cancel_read:
    return runCanonStreamCancel(Canon, Args, Rets, true, true);
  case ComponentCanonOpCode::Stream__cancel_write:
    return runCanonStreamCancel(Canon, Args, Rets, true, false);
  case ComponentCanonOpCode::Future__cancel_read:
    return runCanonStreamCancel(Canon, Args, Rets, false, true);
  case ComponentCanonOpCode::Future__cancel_write:
    return runCanonStreamCancel(Canon, Args, Rets, false, false);
  case ComponentCanonOpCode::Stream__drop_readable:
    return runCanonStreamDrop(Canon, Args, true, true);
  case ComponentCanonOpCode::Stream__drop_writable:
    return runCanonStreamDrop(Canon, Args, true, false);
  case ComponentCanonOpCode::Future__drop_readable:
    return runCanonStreamDrop(Canon, Args, false, true);
  case ComponentCanonOpCode::Future__drop_writable:
    return runCanonStreamDrop(Canon, Args, false, false);
  case ComponentCanonOpCode::Error_context__new:
    return runCanonErrorContextNew(Canon, Args, Rets);
  case ComponentCanonOpCode::Error_context__debug_message:
    return runCanonErrorContextDebugMessage(Canon, Args);
  case ComponentCanonOpCode::Error_context__drop:
    return runCanonErrorContextDrop(Canon, Args);
  case ComponentCanonOpCode::Waitable_set__new:
    return runCanonWaitableSetNew(Canon, Rets);
  case ComponentCanonOpCode::Waitable_set__wait:
    return runCanonWaitableSetWait(Canon, Args, Rets, false);
  case ComponentCanonOpCode::Waitable_set__poll:
    return runCanonWaitableSetWait(Canon, Args, Rets, true);
  case ComponentCanonOpCode::Waitable_set__drop:
    return runCanonWaitableSetDrop(Canon, Args);
  case ComponentCanonOpCode::Waitable__join:
    return runCanonWaitableJoin(Canon, Args);
  case ComponentCanonOpCode::Thread__index:
    return runCanonThreadIndex(Canon, Rets);
  case ComponentCanonOpCode::Thread__new_indirect:
    return runCanonThreadNewIndirect(Canon, Args, Rets);
  case ComponentCanonOpCode::Thread__resume_later:
    return runCanonThreadResumeLater(Canon, Args);
  case ComponentCanonOpCode::Thread__suspend:
    return runCanonThreadSuspend(Canon, Rets);
  case ComponentCanonOpCode::Thread__suspend_then_resume:
    return runCanonThreadSwitch(Canon, Args, Rets, false, false);
  case ComponentCanonOpCode::Thread__yield_then_resume:
    return runCanonThreadSwitch(Canon, Args, Rets, true, false);
  case ComponentCanonOpCode::Thread__suspend_then_promote:
    return runCanonThreadSwitch(Canon, Args, Rets, false, true);
  case ComponentCanonOpCode::Thread__yield_then_promote:
    return runCanonThreadSwitch(Canon, Args, Rets, true, true);
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical built-in {} is not supported."sv,
                  ComponentCanonOpCodeStr[Canon.getOpCode()]);
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
}

} // namespace Executor
} // namespace WasmEdge
