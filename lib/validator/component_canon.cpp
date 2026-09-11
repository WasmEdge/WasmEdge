// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- component_canon.cpp - Canonical built-in validation ---------------===//
//
// Validation of canon lift / lower / resource.* definitions.
//
//===----------------------------------------------------------------------===//

#include "common/errinfo.h"
#include "common/spdlog.h"
#include "validator/validator.h"

#include <vector>

namespace WasmEdge {
namespace Validator {

using namespace std::literals;

// canon ::= lift | lower | resource.* | async built-in, one switch per opcode.
Expect<void>
Validator::validate(const AST::Component::Canonical &Canon) noexcept {
  auto &S = CompCtx.top();
  const ValType I32V{TypeCode::I32};
  const ValType I64V{TypeCode::I64};

  // Defines the core function a canonical definition lowers to.
  auto PushCoreFunc = [&](std::vector<ValType> Params,
                          std::vector<ValType> Results) -> Expect<void> {
    S.addCoreFunc(CompTypes.addCoreFuncType(Params, Results));
    return {};
  };
  // A built-in takes its core type from the shared table, keyed on Addr.
  auto PushBuiltin = [&](const ValType &Addr) -> Expect<void> {
    auto [Params, Results] = AST::Component::Canonical::getBuiltinCoreFuncType(
        Canon.getOpCode(), Addr);
    return PushCoreFunc(std::move(Params), std::move(Results));
  };

  // The resource built-ins take no options and name a resource type.
  auto GetResourceEntry =
      [&](std::string_view What) -> Expect<const Component::TypeEntry *> {
    if (!Canon.getOptions().empty()) {
      spdlog::error(ErrCode::Value::CanonOptionOnBuiltin);
      spdlog::error("    resource built-ins take no canonical options."sv);
      return Unexpect(ErrCode::Value::CanonOptionOnBuiltin);
    }
    const auto *Entry = S.getType(Canon.getIndex());
    if (Entry == nullptr) {
      spdlog::error(ErrCode::Value::ComponentTypeIndexOutOfBounds);
      spdlog::error("    {} type index {} out of bounds."sv, What,
                    Canon.getIndex());
      return Unexpect(ErrCode::Value::ComponentTypeIndexOutOfBounds);
    }
    if (!Entry->isResource()) {
      spdlog::error(ErrCode::Value::ComponentNotResourceType);
      spdlog::error("    {} type index {} is not a resource."sv, What,
                    Canon.getIndex());
      return Unexpect(ErrCode::Value::ComponentNotResourceType);
    }
    return Entry;
  };

  // resource.new / resource.rep need a locally-defined resource and its rep.
  auto LocalResourceRep = [&](std::string_view What) -> Expect<ValType> {
    EXPECTED_TRY(const auto *Entry, GetResourceEntry(What));
    const auto &Res = CompTypes.getResource(*Entry->ResourceId);
    if (Res.RT == nullptr || Res.Origin != &S) {
      spdlog::error(ErrCode::Value::ComponentNotLocalResource);
      spdlog::error("    {} requires a locally-defined resource type."sv, What);
      return Unexpect(ErrCode::Value::ComponentNotLocalResource);
    }
    return Res.RT->isAddrI64() ? I64V : I32V;
  };

  // stream/future built-ins name their element type in the type immediate.
  auto CheckTypeImmediate =
      [&](bool WantStream) -> Expect<const Component::TypeEntry *> {
    const auto *Entry = S.getType(Canon.getIndex());
    const auto *Def = Entry != nullptr ? Entry->getDefValType() : nullptr;
    const bool Ok =
        Def != nullptr && (WantStream ? Def->isStreamTy() : Def->isFutureTy());
    if (!Ok) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    Built-in type index {} does not refer to a {} "
                    "type."sv,
                    Canon.getIndex(), WantStream ? "stream" : "future");
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    return Entry;
  };

  // Option-bearing built-ins are lowerings: no post-return or callback.
  auto BuiltinOptions = [&](bool NeedMemory, bool NeedRealloc) -> Expect<void> {
    EXPECTED_TRY(CompCtx.checkOptions(Canon, false));
    return CompCtx.requireOptions(Canon, NeedMemory, NeedRealloc,
                                  "this canonical built-in"sv);
  };

  // Built-ins the spec restricts to synchronous use.
  auto RequireSync = [&]() -> Expect<void> {
    if (Canon.hasOption(ComponentCanonOptCode::Async)) {
      spdlog::error(ErrCode::Value::CanonAsyncOnBuiltin);
      spdlog::error("    this canonical built-in is always synchronous."sv);
      return Unexpect(ErrCode::Value::CanonAsyncOnBuiltin);
    }
    return {};
  };

  bool ParamsNeedMemory = false, ResultsNeedMemory = false;

  switch (Canon.getOpCode()) {
  case ComponentCanonOpCode::Lift: {
    const auto *Entry = S.getType(Canon.getTargetIndex());
    if (Entry == nullptr) {
      spdlog::error(ErrCode::Value::ComponentTypeIndexOutOfBounds);
      spdlog::error("    canon lift type index {} out of bounds."sv,
                    Canon.getTargetIndex());
      return Unexpect(ErrCode::Value::ComponentTypeIndexOutOfBounds);
    }
    if (Entry->getFuncType() == nullptr) {
      spdlog::error(ErrCode::Value::ComponentNotFunctionType);
      spdlog::error("    canon lift type index {} is not a function type."sv,
                    Canon.getTargetIndex());
      return Unexpect(ErrCode::Value::ComponentNotFunctionType);
    }
    const Component::FuncInfo FI{Entry->getFuncType(), Entry->Home,
                                 Entry->Remap};
    EXPECTED_TRY(CompCtx.checkOptions(Canon, true));
    const bool AsyncOpt = Canon.hasOption(ComponentCanonOptCode::Async);
    if (AsyncOpt && !FI.FT->isAsync()) {
      spdlog::error(ErrCode::Value::CanonAsyncRequiresAsyncType);
      spdlog::error("    canon lift with `async` needs `(func async ...)`."sv);
      return Unexpect(ErrCode::Value::CanonAsyncRequiresAsyncType);
    }
    if (Canon.hasOption(ComponentCanonOptCode::Callback) && !AsyncOpt) {
      spdlog::error(ErrCode::Value::CanonCallbackRequiresAsync);
      spdlog::error("    the `callback` option requires the `async` option."sv);
      return Unexpect(ErrCode::Value::CanonCallbackRequiresAsync);
    }
    if (AsyncOpt && Canon.hasOption(ComponentCanonOptCode::PostReturn)) {
      spdlog::error(ErrCode::Value::CanonPostReturnWithAsync);
      spdlog::error("    cannot specify post-return function in combination "
                    "with async."sv);
      return Unexpect(ErrCode::Value::CanonPostReturnWithAsync);
    }

    const ValType Ptr = CompCtx.getCanonPtrType(Canon);
    EXPECTED_TRY(auto Sig, CompTypes.flattenFuncType(FI, Ptr, ParamsNeedMemory,
                                                     ResultsNeedMemory));
    const bool ParamsIndirect =
        Sig.getParamTypes().size() > Component::TypeSystem::MaxFlatParams;
    const bool ResultsIndirect =
        Sig.getReturnTypes().size() > Component::TypeSystem::MaxFlatResults;
    if (ParamsIndirect) {
      Sig.getParamTypes().assign(1, Ptr);
    }
    if (AsyncOpt) {
      // Async lift returns the packed callback code, or nothing when stackful.
      Sig.getReturnTypes().clear();
      if (Canon.hasOption(ComponentCanonOptCode::Callback)) {
        Sig.getReturnTypes().push_back(I32V);
      }
    } else if (ResultsIndirect) {
      Sig.getReturnTypes().assign(1, Ptr);
    }
    // Required options: lifting params lowers them into the callee's memory.
    EXPECTED_TRY(CompCtx.requireOptions(
        Canon,
        ParamsNeedMemory || ParamsIndirect ||
            (!AsyncOpt && (ResultsNeedMemory || ResultsIndirect)),
        ParamsNeedMemory || ParamsIndirect, "canon lift"sv));

    // The callee must have exactly the flattened core type.
    const auto *Callee = S.getCoreFunc(Canon.getIndex());
    if (Callee == nullptr) {
      spdlog::error(ErrCode::Value::ComponentFunctionIndexOutOfBounds);
      spdlog::error("    canon lift core function index {} out of bounds."sv,
                    Canon.getIndex());
      return Unexpect(ErrCode::Value::ComponentFunctionIndexOutOfBounds);
    }
    const auto &CalleeType = Callee->getCompositeType();
    if (!CalleeType.isFunc() ||
        CalleeType.getFuncType().getParamTypes() != Sig.getParamTypes()) {
      spdlog::error(ErrCode::Value::CanonLoweredParamsMismatch);
      spdlog::error("    canon lift core function does not match the "
                    "flattened parameters."sv);
      return Unexpect(ErrCode::Value::CanonLoweredParamsMismatch);
    }
    if (CalleeType.getFuncType().getReturnTypes() != Sig.getReturnTypes()) {
      spdlog::error(ErrCode::Value::CanonLoweredResultsMismatch);
      spdlog::error("    canon lift core function does not match the "
                    "flattened results."sv);
      return Unexpect(ErrCode::Value::CanonLoweredResultsMismatch);
    }

    // post-return has type (func (param flat_results)).
    for (const auto &Opt : Canon.getOptions()) {
      if (Opt.getCode() != ComponentCanonOptCode::PostReturn) {
        continue;
      }
      const auto *Post = S.getCoreFunc(Opt.getIndex());
      if (Post == nullptr) {
        spdlog::error(ErrCode::Value::ComponentFunctionIndexOutOfBounds);
        spdlog::error("    post-return core function index {} out of bounds."sv,
                      Opt.getIndex());
        return Unexpect(ErrCode::Value::ComponentFunctionIndexOutOfBounds);
      }
      const auto &PT = Post->getCompositeType();
      if (!PT.isFunc() ||
          PT.getFuncType().getParamTypes() != Sig.getReturnTypes() ||
          !PT.getFuncType().getReturnTypes().empty()) {
        spdlog::error(ErrCode::Value::CanonPostReturnSignature);
        spdlog::error(
            "    post-return must take the lifted core results and return "
            "nothing."sv);
        return Unexpect(ErrCode::Value::CanonPostReturnSignature);
      }
    }

    S.addFunc(FI);
    return {};
  }

  case ComponentCanonOpCode::Lower: {
    const auto *FI = S.getFunc(Canon.getIndex());
    if (FI == nullptr) {
      spdlog::error(ErrCode::Value::ComponentFunctionIndexOutOfBounds);
      spdlog::error("    canon lower function index {} out of bounds."sv,
                    Canon.getIndex());
      return Unexpect(ErrCode::Value::ComponentFunctionIndexOutOfBounds);
    }
    EXPECTED_TRY(CompCtx.checkOptions(Canon, false));
    const bool AsyncOpt = Canon.hasOption(ComponentCanonOptCode::Async);
    if (FI->FT == nullptr) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    if (AsyncOpt && !FI->FT->isAsync()) {
      spdlog::error(ErrCode::Value::CanonAsyncRequiresAsyncType);
      spdlog::error("    canon lower with `async` needs `(func async ...)`."sv);
      return Unexpect(ErrCode::Value::CanonAsyncRequiresAsyncType);
    }

    const ValType Ptr = CompCtx.getCanonPtrType(Canon);
    EXPECTED_TRY(auto Sig, CompTypes.flattenFuncType(*FI, Ptr, ParamsNeedMemory,
                                                     ResultsNeedMemory));
    const bool ParamsIndirect =
        Sig.getParamTypes().size() >
        (AsyncOpt ? Component::TypeSystem::MaxFlatAsyncParams
                  : Component::TypeSystem::MaxFlatParams);
    const bool ResultsIndirect =
        AsyncOpt ? !Sig.getReturnTypes().empty()
                 : Sig.getReturnTypes().size() >
                       Component::TypeSystem::MaxFlatResults;
    if (ParamsIndirect) {
      Sig.getParamTypes().assign(1, Ptr);
    }
    if (ResultsIndirect) {
      // The caller passes a pointer for the results as the last parameter.
      Sig.getParamTypes().push_back(Ptr);
      Sig.getReturnTypes().clear();
    }
    if (AsyncOpt) {
      // Async lower: the core function returns the packed subtask state.
      Sig.getReturnTypes().assign(1, I32V);
    }
    EXPECTED_TRY(CompCtx.requireOptions(Canon,
                                        ParamsNeedMemory || ResultsNeedMemory ||
                                            ParamsIndirect || ResultsIndirect,
                                        ResultsNeedMemory, "canon lower"sv));
    return PushCoreFunc(Sig.getParamTypes(), Sig.getReturnTypes());
  }

  case ComponentCanonOpCode::Resource__new: {
    EXPECTED_TRY(const auto Rep, LocalResourceRep("resource.new"sv));
    return PushCoreFunc({Rep}, {I32V});
  }
  case ComponentCanonOpCode::Resource__rep: {
    EXPECTED_TRY(const auto Rep, LocalResourceRep("resource.rep"sv));
    return PushCoreFunc({I32V}, {Rep});
  }
  case ComponentCanonOpCode::Resource__drop:
    EXPECTED_TRY(GetResourceEntry("resource.drop"sv));
    return PushCoreFunc({I32V}, {});

  // Async built-ins take their core signatures from the shared table.
  case ComponentCanonOpCode::Backpressure__inc:
  case ComponentCanonOpCode::Backpressure__dec:
    return PushBuiltin(I32V);
  case ComponentCanonOpCode::Thread__index:
    return PushBuiltin(I32V);
  case ComponentCanonOpCode::Task__return: {
    // The results lower as the core parameters; task.return never reallocates.
    EXPECTED_TRY(CompCtx.checkOptions(Canon, false));
    if (Canon.hasOption(ComponentCanonOptCode::Realloc)) {
      spdlog::error(ErrCode::Value::CanonReallocOnBuiltin);
      spdlog::error("    realloc cannot be specified for task.return."sv);
      return Unexpect(ErrCode::Value::CanonReallocOnBuiltin);
    }
    EXPECTED_TRY(RequireSync());
    const ValType Ptr = CompCtx.getCanonPtrType(Canon);
    std::vector<ValType> Params;
    for (const auto &R : Canon.getResultList()) {
      const Component::QualValType Q{R.getValType(), &S, nullptr};
      if (!CompTypes.flattenValType(Q, Params, Ptr)) {
        spdlog::error(ErrCode::Value::InvalidTypeReference);
        return Unexpect(ErrCode::Value::InvalidTypeReference);
      }
      ParamsNeedMemory = ParamsNeedMemory || CompTypes.needsMemory(Q);
    }
    if (Params.size() > Component::TypeSystem::MaxFlatParams) {
      Params.assign(1, Ptr);
      ParamsNeedMemory = true;
    }
    EXPECTED_TRY(CompCtx.requireOptions(Canon, ParamsNeedMemory, false,
                                        "task.return"sv));
    return PushCoreFunc(std::move(Params), {});
  }
  case ComponentCanonOpCode::Task__cancel:
    return PushBuiltin(I32V);
  case ComponentCanonOpCode::Context__get:
  case ComponentCanonOpCode::Context__set: {
    const bool IsGet = Canon.getOpCode() == ComponentCanonOpCode::Context__get;
    const std::string_view Name = IsGet ? "context.get"sv : "context.set"sv;
    // The slot immediate is stored as the constant value, not as an index.
    if (Canon.getConstVal() >= Component::TypeSystem::MaxContextSlots) {
      spdlog::error(ErrCode::Value::ComponentContextSlotOutOfBounds);
      spdlog::error(
          "    Context slot {} out of bounds, must be less than {}."sv,
          Canon.getConstVal(), Component::TypeSystem::MaxContextSlots);
      return Unexpect(ErrCode::Value::ComponentContextSlotOutOfBounds);
    }
    const ValType Ty = Canon.getContextType();
    if (Ty != I32V && Ty != I64V) {
      spdlog::error(ErrCode::Value::ComponentContextTypeInvalid);
      spdlog::error("    `{}` only supports `i32` or `i64`."sv, Name);
      return Unexpect(ErrCode::Value::ComponentContextTypeInvalid);
    }
    // A 64-bit slot is gated the same way a 64-bit resource rep is.
    if (Ty == I64V && !Conf.hasProposal(Proposal::Memory64)) {
      spdlog::error(ErrCode::Value::ComponentContextTypeInvalid);
      spdlog::error(ErrInfo::InfoProposal(Proposal::Memory64));
      spdlog::error("    a 64-bit `{}` needs the memory64 proposal."sv, Name);
      return Unexpect(ErrCode::Value::ComponentContextTypeInvalid);
    }
    // The whole component shares one thread-local slot type.
    auto &Shared = CompCtx.top().ContextType;
    if (Shared.has_value() && *Shared != Ty) {
      spdlog::error(ErrCode::Value::ComponentContextTypeMismatch);
      spdlog::error("    `{}` type must match previous context type."sv, Name);
      return Unexpect(ErrCode::Value::ComponentContextTypeMismatch);
    }
    Shared = Ty;
    if (IsGet) {
      return PushCoreFunc({}, {Ty});
    }
    return PushCoreFunc({Ty}, {});
  }
  case ComponentCanonOpCode::Yield:
    return PushBuiltin(I32V);
  case ComponentCanonOpCode::Subtask__cancel:
    return PushBuiltin(I32V);
  case ComponentCanonOpCode::Subtask__drop:
    return PushBuiltin(I32V);
  case ComponentCanonOpCode::Stream__new:
    EXPECTED_TRY(CheckTypeImmediate(true));
    return PushBuiltin(I32V);
  case ComponentCanonOpCode::Stream__read:
  case ComponentCanonOpCode::Stream__write: {
    EXPECTED_TRY(const auto *Entry, CheckTypeImmediate(true));
    const auto &Elem = Entry->getDefValType()->getStream().ValTy;
    // Reading a payload that allocates needs realloc; writing never does.
    const bool NeedRealloc =
        Canon.getOpCode() == ComponentCanonOpCode::Stream__read &&
        Elem.has_value() &&
        CompTypes.needsMemory({*Elem, Entry->Home, Entry->Remap});
    EXPECTED_TRY(BuiltinOptions(Elem.has_value(), NeedRealloc));
    return PushBuiltin(CompCtx.getCanonPtrType(Canon));
  }
  case ComponentCanonOpCode::Stream__cancel_read:
  case ComponentCanonOpCode::Stream__cancel_write:
    EXPECTED_TRY(CheckTypeImmediate(true));
    return PushBuiltin(I32V);
  case ComponentCanonOpCode::Stream__drop_readable:
  case ComponentCanonOpCode::Stream__drop_writable:
    EXPECTED_TRY(CheckTypeImmediate(true));
    return PushBuiltin(I32V);
  case ComponentCanonOpCode::Future__new:
    EXPECTED_TRY(CheckTypeImmediate(false));
    return PushBuiltin(I32V);
  case ComponentCanonOpCode::Future__read:
  case ComponentCanonOpCode::Future__write: {
    EXPECTED_TRY(const auto *Entry, CheckTypeImmediate(false));
    const auto &Elem = Entry->getDefValType()->getFuture().ValTy;
    // Reading a payload that allocates needs realloc; writing never does.
    const bool NeedRealloc =
        Canon.getOpCode() == ComponentCanonOpCode::Future__read &&
        Elem.has_value() &&
        CompTypes.needsMemory({*Elem, Entry->Home, Entry->Remap});
    EXPECTED_TRY(BuiltinOptions(Elem.has_value(), NeedRealloc));
    return PushBuiltin(CompCtx.getCanonPtrType(Canon));
  }
  case ComponentCanonOpCode::Future__cancel_read:
  case ComponentCanonOpCode::Future__cancel_write:
    EXPECTED_TRY(CheckTypeImmediate(false));
    return PushBuiltin(I32V);
  case ComponentCanonOpCode::Future__drop_readable:
  case ComponentCanonOpCode::Future__drop_writable:
    EXPECTED_TRY(CheckTypeImmediate(false));
    return PushBuiltin(I32V);
  case ComponentCanonOpCode::Error_context__new:
    EXPECTED_TRY(BuiltinOptions(true, false));
    EXPECTED_TRY(RequireSync());
    return PushBuiltin(CompCtx.getCanonPtrType(Canon));
  case ComponentCanonOpCode::Error_context__debug_message:
    EXPECTED_TRY(BuiltinOptions(true, true));
    EXPECTED_TRY(RequireSync());
    return PushBuiltin(CompCtx.getCanonPtrType(Canon));
  case ComponentCanonOpCode::Error_context__drop:
    return PushBuiltin(I32V);
  case ComponentCanonOpCode::Waitable_set__new:
    return PushBuiltin(I32V);
  case ComponentCanonOpCode::Waitable_set__wait:
  case ComponentCanonOpCode::Waitable_set__poll: {
    if (Canon.getIndex() >= S.CoreMemories.size()) {
      spdlog::error(ErrCode::Value::ComponentMemoryIndexOutOfBounds);
      spdlog::error("    Canonical built-in memory index {} out of bounds."sv,
                    Canon.getIndex());
      return Unexpect(ErrCode::Value::ComponentMemoryIndexOutOfBounds);
    }
    // The payload address carries the address type of the given memory.
    const auto *Mem = S.CoreMemories[Canon.getIndex()];
    const ValType Addr =
        ValType(Mem != nullptr && Mem->getLimit().is64() ? TypeCode::I64
                                                         : TypeCode::I32);
    return PushBuiltin(Addr);
  }
  case ComponentCanonOpCode::Waitable_set__drop:
    return PushBuiltin(I32V);
  case ComponentCanonOpCode::Waitable__join:
    return PushBuiltin(I32V);
  case ComponentCanonOpCode::Thread__new_indirect: {
    // The start type must be (func (param i32)) and the target a core table.
    const auto *CT = S.getCoreType(Canon.getIndex());
    const bool TypeOk =
        CT != nullptr && CT->Func != nullptr &&
        CT->Func->getCompositeType().isFunc() &&
        CT->Func->getCompositeType().getFuncType().getParamTypes() ==
            std::vector<ValType>{I32V} &&
        CT->Func->getCompositeType().getFuncType().getReturnTypes().empty();
    if (!TypeOk) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error(
          "    thread.new-indirect start type must be (func (param i32))."sv);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    if (Canon.getTargetIndex() >= S.CoreTables.size()) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    thread.new-indirect table index {} out of bounds."sv,
                    Canon.getTargetIndex());
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    // The thread index carries the address type of the table.
    const auto *Tab = S.CoreTables[Canon.getTargetIndex()];
    return PushBuiltin(ValType(Tab != nullptr && Tab->getLimit().is64()
                                   ? TypeCode::I64
                                   : TypeCode::I32));
  }
  case ComponentCanonOpCode::Thread__resume_later:
    return PushBuiltin(I32V);
  case ComponentCanonOpCode::Thread__suspend:
    return PushBuiltin(I32V);
  case ComponentCanonOpCode::Thread__suspend_then_resume:
  case ComponentCanonOpCode::Thread__yield_then_resume:
  case ComponentCanonOpCode::Thread__suspend_then_promote:
  case ComponentCanonOpCode::Thread__yield_then_promote:
    return PushBuiltin(I32V);
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplValidator);
    spdlog::error("    canonical built-in {} is not supported yet."sv,
                  static_cast<uint32_t>(Canon.getOpCode()));
    return Unexpect(ErrCode::Value::ComponentNotImplValidator);
  }
}

} // namespace Validator
} // namespace WasmEdge
