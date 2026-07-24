// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- component_canon.cpp - Canonical built-in validation ---------------===//
//
// Validation of canon lift / lower / resource.* definitions, including the
// validator-side implementation of the canonical ABI `flatten_functype`.
//
//===----------------------------------------------------------------------===//

#include "common/errinfo.h"
#include "common/spdlog.h"
#include "validator/validator.h"

#include <vector>

namespace WasmEdge {
namespace Validator {

using namespace std::literals;

// NOLINTBEGIN(misc-no-recursion) -- type structures are finitely nested.

// Spec flatten_type: appends the flat core types of Q. Returns false for
// types that cannot be flattened (async-gated ones).
bool Validator::flattenValType(const CtxView::QualValType &Q,
                               std::vector<ValType> &Out,
                               const ValType &Ptr) noexcept {
  const auto N = normalizeValType(Q);
  if (!N.Valid) {
    return false;
  }
  if (N.DVT == nullptr) {
    switch (N.Prim) {
    case ComponentTypeCode::Bool:
    case ComponentTypeCode::S8:
    case ComponentTypeCode::U8:
    case ComponentTypeCode::S16:
    case ComponentTypeCode::U16:
    case ComponentTypeCode::S32:
    case ComponentTypeCode::U32:
    case ComponentTypeCode::Char:
      Out.push_back(ValType(TypeCode::I32));
      return true;
    case ComponentTypeCode::S64:
    case ComponentTypeCode::U64:
      Out.push_back(ValType(TypeCode::I64));
      return true;
    case ComponentTypeCode::F32:
      Out.push_back(ValType(TypeCode::F32));
      return true;
    case ComponentTypeCode::F64:
      Out.push_back(ValType(TypeCode::F64));
      return true;
    case ComponentTypeCode::String:
      Out.push_back(Ptr);
      Out.push_back(Ptr);
      return true;
    case ComponentTypeCode::ErrContext:
      Out.push_back(ValType(TypeCode::I32));
      return true;
    default:
      return false;
    }
  }
  const auto &D = *N.DVT;
  auto Sub = [&](const ComponentValType &VT) noexcept {
    return flattenValType({VT, N.Home, N.Remap}, Out, Ptr);
  };
  if (D.isRecordTy()) {
    for (const auto &LT : D.getRecord().LabelTypes) {
      if (!Sub(LT.getValType())) {
        return false;
      }
    }
    return true;
  }
  if (D.isTupleTy()) {
    for (const auto &Ty : D.getTuple().Types) {
      if (!Sub(Ty)) {
        return false;
      }
    }
    return true;
  }
  if (D.isListTy()) {
    const auto &L = D.getList();
    if (L.Len.has_value()) {
      // Fixed-length lists flatten to Len copies of the element.
      for (uint32_t I = 0; I < *L.Len; ++I) {
        if (!Sub(L.ValTy)) {
          return false;
        }
      }
      return true;
    }
    Out.push_back(Ptr);
    Out.push_back(Ptr);
    return true;
  }
  if (D.isFlagsTy() || D.isEnumTy() || D.isOwnTy() || D.isBorrowTy() ||
      D.isStreamTy() || D.isFutureTy()) {
    Out.push_back(ValType(TypeCode::I32));
    return true;
  }
  if (D.isVariantTy() || D.isOptionTy() || D.isResultTy()) {
    // flatten_variant: discriminant + element-wise join of the payloads.
    std::vector<std::vector<ComponentValType>> Payloads;
    if (D.isVariantTy()) {
      for (const auto &[Label, Ty] : D.getVariant().Cases) {
        if (Ty.has_value()) {
          Payloads.push_back({*Ty});
        } else {
          Payloads.push_back({});
        }
      }
    } else if (D.isOptionTy()) {
      Payloads.push_back({});
      Payloads.push_back({D.getOption().ValTy});
    } else {
      const auto &R = D.getResult();
      Payloads.push_back(R.ValTy.has_value()
                             ? std::vector<ComponentValType>{*R.ValTy}
                             : std::vector<ComponentValType>{});
      Payloads.push_back(R.ErrTy.has_value()
                             ? std::vector<ComponentValType>{*R.ErrTy}
                             : std::vector<ComponentValType>{});
    }
    auto Join = [](ValType A, ValType B) noexcept {
      if (A == B) {
        return A;
      }
      if ((A.getCode() == TypeCode::I32 && B.getCode() == TypeCode::F32) ||
          (A.getCode() == TypeCode::F32 && B.getCode() == TypeCode::I32)) {
        return ValType(TypeCode::I32);
      }
      return ValType(TypeCode::I64);
    };
    std::vector<ValType> Joined;
    for (const auto &Payload : Payloads) {
      std::vector<ValType> Flat;
      for (const auto &Ty : Payload) {
        if (!flattenValType({Ty, N.Home, N.Remap}, Flat, Ptr)) {
          return false;
        }
      }
      for (size_t I = 0; I < Flat.size(); ++I) {
        if (I < Joined.size()) {
          Joined[I] = Join(Joined[I], Flat[I]);
        } else {
          Joined.push_back(Flat[I]);
        }
      }
    }
    Out.push_back(ValType(TypeCode::I32));
    Out.insert(Out.end(), Joined.begin(), Joined.end());
    return true;
  }
  return false;
}

// True iff the type transitively contains a list or string (drives the
// memory / realloc option requirements).
bool Validator::needsMemory(const CtxView::QualValType &Q) noexcept {
  const auto N = normalizeValType(Q);
  if (!N.Valid) {
    return false;
  }
  if (N.DVT == nullptr) {
    return N.Prim == ComponentTypeCode::String;
  }
  const auto &D = *N.DVT;
  auto Sub = [&](const ComponentValType &VT) noexcept {
    return needsMemory({VT, N.Home, N.Remap});
  };
  if (D.isListTy()) {
    return true;
  }
  if (D.isRecordTy()) {
    for (const auto &LT : D.getRecord().LabelTypes) {
      if (Sub(LT.getValType())) {
        return true;
      }
    }
    return false;
  }
  if (D.isVariantTy()) {
    for (const auto &[Label, Ty] : D.getVariant().Cases) {
      if (Ty.has_value() && Sub(*Ty)) {
        return true;
      }
    }
    return false;
  }
  if (D.isTupleTy()) {
    for (const auto &Ty : D.getTuple().Types) {
      if (Sub(Ty)) {
        return true;
      }
    }
    return false;
  }
  if (D.isOptionTy()) {
    return Sub(D.getOption().ValTy);
  }
  if (D.isResultTy()) {
    const auto &R = D.getResult();
    return (R.ValTy.has_value() && Sub(*R.ValTy)) ||
           (R.ErrTy.has_value() && Sub(*R.ErrTy));
  }
  return false;
}

// NOLINTEND(misc-no-recursion)

// Shared canonopt structural rules: duplicates, index validity, and the
// per-site option whitelist.
Expect<void>
Validator::validateCanonOptions(const AST::Component::Canonical &Canon,
                                bool IsLift) noexcept {
  bool SeenEncoding = false, SeenMemory = false, SeenRealloc = false,
       SeenPostReturn = false;
  // The pointer width of realloc follows the selected memory.
  const bool MemoryIs64 = canonMemoryIs64(Canon);
  for (const auto &Opt : Canon.getOptions()) {
    switch (Opt.getCode()) {
    case ComponentCanonOptCode::Encode_UTF8:
    case ComponentCanonOptCode::Encode_UTF16:
    case ComponentCanonOptCode::Encode_Latin1:
      if (SeenEncoding) {
        spdlog::error(ErrCode::Value::CanonEncodingConflict);
        spdlog::error("    Duplicate string-encoding canonical option."sv);
        return Unexpect(ErrCode::Value::CanonEncodingConflict);
      }
      SeenEncoding = true;
      break;
    case ComponentCanonOptCode::Memory: {
      if (SeenMemory) {
        spdlog::error(ErrCode::Value::CanonMemoryDuplicated);
        spdlog::error("    Duplicate memory canonical option."sv);
        return Unexpect(ErrCode::Value::CanonMemoryDuplicated);
      }
      SeenMemory = true;
      const uint32_t Idx = Opt.getIndex();
      if (Idx >= CompCtx.top().CoreMemories.size()) {
        spdlog::error(ErrCode::Value::ComponentMemoryIndexOutOfBounds);
        spdlog::error("    Canonical option memory index {} out of bounds."sv,
                      Idx);
        return Unexpect(ErrCode::Value::ComponentMemoryIndexOutOfBounds);
      }
      break;
    }
    case ComponentCanonOptCode::Realloc: {
      if (SeenRealloc) {
        spdlog::error(ErrCode::Value::CanonReallocDuplicated);
        spdlog::error("    Duplicate realloc canonical option."sv);
        return Unexpect(ErrCode::Value::CanonReallocDuplicated);
      }
      SeenRealloc = true;
      const auto *Func = CompCtx.top().getCoreFunc(Opt.getIndex());
      if (Func == nullptr) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error(
            "    Canonical option realloc function index {} out of bounds."sv,
            Opt.getIndex());
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
      // realloc must have type [ptr ptr ptr ptr] -> [ptr], where ptr is the
      // index type of the selected memory.
      const TypeCode Ptr = MemoryIs64 ? TypeCode::I64 : TypeCode::I32;
      const std::vector<ValType> ReallocParams(4, ValType(Ptr));
      const std::vector<ValType> ReallocResults(1, ValType(Ptr));
      const auto &CT = Func->getCompositeType();
      if (!CT.isFunc() || CT.getFuncType().getParamTypes() != ReallocParams ||
          CT.getFuncType().getReturnTypes() != ReallocResults) {
        spdlog::error(ErrCode::Value::CanonReallocSignature);
        spdlog::error(
            "    realloc must have type [ptr ptr ptr ptr] -> [ptr]."sv);
        return Unexpect(ErrCode::Value::CanonReallocSignature);
      }
      break;
    }
    case ComponentCanonOptCode::PostReturn:
      if (!IsLift) {
        spdlog::error(ErrCode::Value::CanonPostReturnOnLower);
        spdlog::error("    post-return cannot be specified for lowerings."sv);
        return Unexpect(ErrCode::Value::CanonPostReturnOnLower);
      }
      if (SeenPostReturn) {
        spdlog::error(ErrCode::Value::CanonPostReturnDuplicated);
        spdlog::error("    post-return is specified more than once."sv);
        return Unexpect(ErrCode::Value::CanonPostReturnDuplicated);
      }
      SeenPostReturn = true;
      // The signature is checked in validateCanonLift once the flat type of
      // the lifted function is known.
      break;
    case ComponentCanonOptCode::Async:
    case ComponentCanonOptCode::AlwaysTaskReturn:
      break;
    case ComponentCanonOptCode::Callback: {
      const auto *Func = CompCtx.top().getCoreFunc(Opt.getIndex());
      if (Func == nullptr) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error(
            "    Canonical option callback function index {} out of bounds."sv,
            Opt.getIndex());
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
      // callback has type [i32 i32 i32] -> [i32].
      const std::vector<ValType> CallbackParams(3, ValType(TypeCode::I32));
      const std::vector<ValType> CallbackResults(1, ValType(TypeCode::I32));
      const auto &CT = Func->getCompositeType();
      if (!CT.isFunc() || CT.getFuncType().getParamTypes() != CallbackParams ||
          CT.getFuncType().getReturnTypes() != CallbackResults) {
        spdlog::error(ErrCode::Value::InvalidCanonOption);
        spdlog::error("    callback must have type [i32 i32 i32] -> [i32]."sv);
        return Unexpect(ErrCode::Value::InvalidCanonOption);
      }
      break;
    }
    default:
      spdlog::error(ErrCode::Value::UnknownCanonicalOption);
      return Unexpect(ErrCode::Value::UnknownCanonicalOption);
    }
  }
  if (SeenRealloc && !SeenMemory) {
    spdlog::error(ErrCode::Value::CanonMemoryRequired);
    spdlog::error("    realloc requires the memory canonical option."sv);
    return Unexpect(ErrCode::Value::CanonMemoryRequired);
  }
  return {};
}

namespace {
// Option presence probes shared by lift/lower requirement checks.
bool hasOpt(const AST::Component::Canonical &Canon,
            ComponentCanonOptCode Code) noexcept {
  for (const auto &Opt : Canon.getOptions()) {
    if (Opt.getCode() == Code) {
      return true;
    }
  }
  return false;
}
} // namespace

bool Validator::canonMemoryIs64(
    const AST::Component::Canonical &Canon) noexcept {
  for (const auto &Opt : Canon.getOptions()) {
    if (Opt.getCode() == ComponentCanonOptCode::Memory) {
      const uint32_t Idx = Opt.getIndex();
      if (Idx < CompCtx.top().CoreMemories.size()) {
        const auto *Mem = CompCtx.top().CoreMemories[Idx];
        return Mem != nullptr && Mem->getLimit().is64();
      }
    }
  }
  return false;
}

Expect<void>
Validator::validateCanonLift(const AST::Component::Canonical &Canon) noexcept {
  auto &S = CompCtx.top();
  // Target function type.
  const auto *Entry = S.getType(Canon.getTargetIndex());
  if (Entry == nullptr) {
    spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
    spdlog::error("    canon lift type index {} out of bounds."sv,
                  Canon.getTargetIndex());
    return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
  }
  if (Entry->DT == nullptr || !Entry->DT->isFuncType()) {
    spdlog::error(ErrCode::Value::ComponentNotFunctionType);
    spdlog::error("    canon lift type index {} is not a function type."sv,
                  Canon.getTargetIndex());
    return Unexpect(ErrCode::Value::ComponentNotFunctionType);
  }
  const CtxView::FuncInfo FI{&Entry->DT->getFuncType(), Entry->Home,
                             Entry->Remap};
  EXPECTED_TRY(validateCanonOptions(Canon, true));
  const bool AsyncOpt = hasOpt(Canon, ComponentCanonOptCode::Async);
  if (AsyncOpt && !FI.FT->isAsync()) {
    spdlog::error(ErrCode::Value::CanonAsyncRequiresAsyncType);
    spdlog::error("    canon lift with `async` needs `(func async ...)`."sv);
    return Unexpect(ErrCode::Value::CanonAsyncRequiresAsyncType);
  }
  if (hasOpt(Canon, ComponentCanonOptCode::Callback) && !AsyncOpt) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error("    the `callback` option requires the `async` option."sv);
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }
  if (AsyncOpt && hasOpt(Canon, ComponentCanonOptCode::PostReturn)) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error("    cannot specify post-return function in combination "
                  "with async."sv);
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }

  // Flatten parameters and results in the 'lift' context.
  const ValType Ptr(canonMemoryIs64(Canon) ? TypeCode::I64 : TypeCode::I32);
  std::vector<ValType> FlatParams, FlatResults;
  bool ParamsNeedMemory = false, ResultsNeedMemory = false;
  for (const auto &P : FI.FT->getParamList()) {
    if (!flattenValType({P.getValType(), FI.Home, FI.Remap}, FlatParams, Ptr)) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    ParamsNeedMemory =
        ParamsNeedMemory || needsMemory({P.getValType(), FI.Home, FI.Remap});
  }
  for (const auto &R : FI.FT->getResultList()) {
    if (!flattenValType({R.getValType(), FI.Home, FI.Remap}, FlatResults,
                        Ptr)) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    ResultsNeedMemory =
        ResultsNeedMemory || needsMemory({R.getValType(), FI.Home, FI.Remap});
  }
  const bool ParamsIndirect = FlatParams.size() > MaxFlatParams;
  const bool ResultsIndirect = FlatResults.size() > MaxFlatResults;
  if (ParamsIndirect) {
    FlatParams.assign(1, Ptr);
  }
  if (AsyncOpt) {
    // Async lift: the core function returns the packed callback code
    // (callback) or nothing (stackful); results flow through task.return.
    FlatResults.clear();
    if (hasOpt(Canon, ComponentCanonOptCode::Callback)) {
      FlatResults.push_back(ValType(TypeCode::I32));
    }
  } else if (ResultsIndirect) {
    FlatResults.assign(1, Ptr);
  }
  // Required options: lifting params lowers them into the callee's memory.
  if ((ParamsNeedMemory || ParamsIndirect ||
       (!AsyncOpt && (ResultsNeedMemory || ResultsIndirect))) &&
      !hasOpt(Canon, ComponentCanonOptCode::Memory)) {
    spdlog::error(ErrCode::Value::CanonMemoryRequired);
    spdlog::error("    canon lift requires the memory option."sv);
    return Unexpect(ErrCode::Value::CanonMemoryRequired);
  }
  if ((ParamsNeedMemory || ParamsIndirect) &&
      !hasOpt(Canon, ComponentCanonOptCode::Realloc)) {
    spdlog::error(ErrCode::Value::CanonReallocRequired);
    spdlog::error("    canon lift requires the realloc option."sv);
    return Unexpect(ErrCode::Value::CanonReallocRequired);
  }

  // The callee must have exactly the flattened core type.
  const auto *Callee = S.getCoreFunc(Canon.getIndex());
  if (Callee == nullptr) {
    spdlog::error(ErrCode::Value::ComponentFunctionIndexOutOfBounds);
    spdlog::error("    canon lift core function index {} out of bounds."sv,
                  Canon.getIndex());
    return Unexpect(ErrCode::Value::ComponentFunctionIndexOutOfBounds);
  }
  const auto &CT = Callee->getCompositeType();
  if (!CT.isFunc() || CT.getFuncType().getParamTypes() != FlatParams) {
    spdlog::error(ErrCode::Value::CanonLoweredParamsMismatch);
    spdlog::error("    canon lift core function does not match the "
                  "flattened parameters."sv);
    return Unexpect(ErrCode::Value::CanonLoweredParamsMismatch);
  }
  if (CT.getFuncType().getReturnTypes() != FlatResults) {
    spdlog::error(ErrCode::Value::CanonLoweredResultsMismatch);
    spdlog::error("    canon lift core function does not match the "
                  "flattened results."sv);
    return Unexpect(ErrCode::Value::CanonLoweredResultsMismatch);
  }

  // post-return has type (func (param flat_results)).
  for (const auto &Opt : Canon.getOptions()) {
    if (Opt.getCode() == ComponentCanonOptCode::PostReturn) {
      const auto *Post = S.getCoreFunc(Opt.getIndex());
      if (Post == nullptr) {
        spdlog::error(ErrCode::Value::ComponentFunctionIndexOutOfBounds);
        spdlog::error("    post-return core function index {} out of bounds."sv,
                      Opt.getIndex());
        return Unexpect(ErrCode::Value::ComponentFunctionIndexOutOfBounds);
      }
      const auto &PT = Post->getCompositeType();
      if (!PT.isFunc() || PT.getFuncType().getParamTypes() != FlatResults ||
          !PT.getFuncType().getReturnTypes().empty()) {
        spdlog::error(ErrCode::Value::CanonPostReturnSignature);
        spdlog::error(
            "    post-return must take the lifted core results and return "
            "nothing."sv);
        return Unexpect(ErrCode::Value::CanonPostReturnSignature);
      }
    }
  }

  S.Funcs.push_back(FI);
  return {};
}

Expect<void>
Validator::validateCanonLower(const AST::Component::Canonical &Canon) noexcept {
  auto &S = CompCtx.top();
  const auto *FI = S.getFunc(Canon.getIndex());
  if (FI == nullptr) {
    spdlog::error(ErrCode::Value::ComponentFunctionIndexOutOfBounds);
    spdlog::error("    canon lower function index {} out of bounds."sv,
                  Canon.getIndex());
    return Unexpect(ErrCode::Value::ComponentFunctionIndexOutOfBounds);
  }
  EXPECTED_TRY(validateCanonOptions(Canon, false));
  const bool AsyncOpt = hasOpt(Canon, ComponentCanonOptCode::Async);
  if (AsyncOpt && !FI->FT->isAsync()) {
    spdlog::error(ErrCode::Value::CanonAsyncRequiresAsyncType);
    spdlog::error("    canon lower with `async` needs `(func async ...)`."sv);
    return Unexpect(ErrCode::Value::CanonAsyncRequiresAsyncType);
  }

  const ValType Ptr(canonMemoryIs64(Canon) ? TypeCode::I64 : TypeCode::I32);
  std::vector<ValType> FlatParams, FlatResults;
  bool ParamsNeedMemory = false, ResultsNeedMemory = false;
  for (const auto &P : FI->FT->getParamList()) {
    if (!flattenValType({P.getValType(), FI->Home, FI->Remap}, FlatParams,
                        Ptr)) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    ParamsNeedMemory =
        ParamsNeedMemory || needsMemory({P.getValType(), FI->Home, FI->Remap});
  }
  for (const auto &R : FI->FT->getResultList()) {
    if (!flattenValType({R.getValType(), FI->Home, FI->Remap}, FlatResults,
                        Ptr)) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    ResultsNeedMemory =
        ResultsNeedMemory || needsMemory({R.getValType(), FI->Home, FI->Remap});
  }
  const bool ParamsIndirect =
      FlatParams.size() > (AsyncOpt ? MaxFlatAsyncParams : MaxFlatParams);
  const bool ResultsIndirect =
      AsyncOpt ? !FlatResults.empty() : FlatResults.size() > MaxFlatResults;
  if (ParamsIndirect) {
    FlatParams.assign(1, Ptr);
  }
  if (ResultsIndirect) {
    // The caller passes a pointer for the results as the last parameter.
    FlatParams.push_back(Ptr);
    FlatResults.clear();
  }
  if (AsyncOpt) {
    // Async lower: the core function returns the packed subtask state.
    FlatResults.assign(1, ValType(TypeCode::I32));
  }
  if ((ParamsNeedMemory || ResultsNeedMemory || ParamsIndirect ||
       ResultsIndirect) &&
      !hasOpt(Canon, ComponentCanonOptCode::Memory)) {
    spdlog::error(ErrCode::Value::CanonMemoryRequired);
    spdlog::error("    canon lower requires the memory option."sv);
    return Unexpect(ErrCode::Value::CanonMemoryRequired);
  }
  if (ResultsNeedMemory && !hasOpt(Canon, ComponentCanonOptCode::Realloc)) {
    spdlog::error(ErrCode::Value::CanonReallocRequired);
    spdlog::error("    canon lower requires the realloc option."sv);
    return Unexpect(ErrCode::Value::CanonReallocRequired);
  }

  S.CoreFuncs.push_back(CompCtx.makeCoreFuncType(FlatParams, FlatResults));
  return {};
}

Expect<void> Validator::validateCanonResourceNew(
    const AST::Component::Canonical &Canon) noexcept {
  auto &S = CompCtx.top();
  if (!Canon.getOptions().empty()) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error("    resource built-ins take no canonical options."sv);
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }
  const auto *Entry = S.getType(Canon.getIndex());
  if (Entry == nullptr) {
    spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
    spdlog::error("    resource.new type index {} out of bounds."sv,
                  Canon.getIndex());
    return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
  }
  if (!Entry->ResourceId.has_value()) {
    spdlog::error(ErrCode::Value::ComponentNotResourceType);
    spdlog::error("    resource.new type index {} is not a resource."sv,
                  Canon.getIndex());
    return Unexpect(ErrCode::Value::ComponentNotResourceType);
  }
  const auto &Res = CompCtx.getResource(*Entry->ResourceId);
  if (Res.RT == nullptr || Res.Origin != &S) {
    spdlog::error(ErrCode::Value::ComponentNotLocalResource);
    spdlog::error(
        "    resource.new requires a locally-defined resource type."sv);
    return Unexpect(ErrCode::Value::ComponentNotLocalResource);
  }
  const ValType Rep =
      Res.RT->isAddrI64() ? ValType(TypeCode::I64) : ValType(TypeCode::I32);
  const std::vector<ValType> Params{Rep}, Results{ValType(TypeCode::I32)};
  S.CoreFuncs.push_back(CompCtx.makeCoreFuncType(Params, Results));
  return {};
}

Expect<void> Validator::validateCanonResourceRep(
    const AST::Component::Canonical &Canon) noexcept {
  auto &S = CompCtx.top();
  if (!Canon.getOptions().empty()) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error("    resource built-ins take no canonical options."sv);
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }
  const auto *Entry = S.getType(Canon.getIndex());
  if (Entry == nullptr) {
    spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
    spdlog::error("    resource.rep type index {} out of bounds."sv,
                  Canon.getIndex());
    return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
  }
  if (!Entry->ResourceId.has_value()) {
    spdlog::error(ErrCode::Value::ComponentNotResourceType);
    spdlog::error("    resource.rep type index {} is not a resource."sv,
                  Canon.getIndex());
    return Unexpect(ErrCode::Value::ComponentNotResourceType);
  }
  const auto &Res = CompCtx.getResource(*Entry->ResourceId);
  if (Res.RT == nullptr || Res.Origin != &S) {
    spdlog::error(ErrCode::Value::ComponentNotLocalResource);
    spdlog::error(
        "    resource.rep requires a locally-defined resource type."sv);
    return Unexpect(ErrCode::Value::ComponentNotLocalResource);
  }
  const ValType Rep =
      Res.RT->isAddrI64() ? ValType(TypeCode::I64) : ValType(TypeCode::I32);
  const std::vector<ValType> Params{ValType(TypeCode::I32)}, Results{Rep};
  S.CoreFuncs.push_back(CompCtx.makeCoreFuncType(Params, Results));
  return {};
}

Expect<void> Validator::validateCanonResourceDrop(
    const AST::Component::Canonical &Canon) noexcept {
  auto &S = CompCtx.top();
  if (!Canon.getOptions().empty()) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error("    resource built-ins take no canonical options."sv);
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }
  const auto *Entry = S.getType(Canon.getIndex());
  if (Entry == nullptr) {
    spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
    spdlog::error("    resource.drop type index {} out of bounds."sv,
                  Canon.getIndex());
    return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
  }
  if (!Entry->ResourceId.has_value()) {
    spdlog::error(ErrCode::Value::ComponentNotResourceType);
    spdlog::error("    resource.drop type index {} is not a resource."sv,
                  Canon.getIndex());
    return Unexpect(ErrCode::Value::ComponentNotResourceType);
  }
  const std::vector<ValType> Params{ValType(TypeCode::I32)};
  const std::vector<ValType> Results;
  S.CoreFuncs.push_back(CompCtx.makeCoreFuncType(Params, Results));
  return {};
}

Expect<void>
Validator::validate(const AST::Component::Canonical &Canon) noexcept {
  switch (Canon.getOpCode()) {
  case ComponentCanonOpCode::Lift:
    return validateCanonLift(Canon);
  case ComponentCanonOpCode::Lower:
    return validateCanonLower(Canon);
  case ComponentCanonOpCode::Resource__new:
    return validateCanonResourceNew(Canon);
  case ComponentCanonOpCode::Resource__rep:
    return validateCanonResourceRep(Canon);
  case ComponentCanonOpCode::Resource__drop:
  case ComponentCanonOpCode::Resource__drop_async:
    return validateCanonResourceDrop(Canon);
  default:
    return validateCanonAsyncBuiltin(Canon);
  }
}

// Validation of the async-model built-ins: type immediates and the derived
// core function signatures (Explainer canonical built-in tables). The deep
// per-option checks tighten once the async runtime grows past the initial
// single-task model.
Expect<void> Validator::validateCanonAsyncBuiltin(
    const AST::Component::Canonical &Canon) noexcept {
  auto &S = CompCtx.top();
  const ValType I32V{TypeCode::I32};
  const ValType I64V{TypeCode::I64};
  auto Push = [&](std::vector<ValType> &&P,
                  std::vector<ValType> &&R) -> Expect<void> {
    S.CoreFuncs.push_back(CompCtx.makeCoreFuncType(P, R));
    return {};
  };
  // Type immediates of stream/future built-ins must name the matching
  // defined value type.
  auto CheckTypeImmediate = [&](bool WantStream) -> Expect<void> {
    const auto *Entry = S.getType(Canon.getIndex());
    const bool Ok = Entry != nullptr && Entry->DT != nullptr &&
                    Entry->DT->isDefValType() &&
                    (WantStream ? Entry->DT->getDefValType().isStreamTy()
                                : Entry->DT->getDefValType().isFutureTy());
    if (!Ok) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    Built-in type index {} does not refer to a {} "
                    "type."sv,
                    Canon.getIndex(), WantStream ? "stream" : "future");
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    return {};
  };
  switch (Canon.getOpCode()) {
  case ComponentCanonOpCode::Backpressure__set:
    return Push({I32V}, {});
  case ComponentCanonOpCode::Backpressure__inc:
  case ComponentCanonOpCode::Backpressure__dec:
    return Push({}, {});
  case ComponentCanonOpCode::Thread__index:
    return Push({}, {I32V});
  case ComponentCanonOpCode::Task__return: {
    // The declared results lower as the core parameters.
    std::vector<ValType> Params;
    for (const auto &R : Canon.getResultList()) {
      if (!flattenValType({R.getValType(), &S, nullptr}, Params, I32V)) {
        spdlog::error(ErrCode::Value::InvalidTypeReference);
        return Unexpect(ErrCode::Value::InvalidTypeReference);
      }
    }
    if (Params.size() > MaxFlatParams) {
      Params.assign(1, I32V);
    }
    return Push(std::move(Params), {});
  }
  case ComponentCanonOpCode::Task__cancel:
    return Push({}, {});
  case ComponentCanonOpCode::Context__get:
    return Push({}, {I32V});
  case ComponentCanonOpCode::Context__set:
    return Push({I32V}, {});
  case ComponentCanonOpCode::Yield:
    return Push({}, {I32V});
  case ComponentCanonOpCode::Subtask__cancel:
    return Push({I32V}, {I32V});
  case ComponentCanonOpCode::Subtask__drop:
    return Push({I32V}, {});
  case ComponentCanonOpCode::Stream__new:
    EXPECTED_TRY(CheckTypeImmediate(true));
    return Push({}, {I64V});
  case ComponentCanonOpCode::Stream__read:
  case ComponentCanonOpCode::Stream__write:
    EXPECTED_TRY(CheckTypeImmediate(true));
    return Push({I32V, I32V, I32V}, {I32V});
  case ComponentCanonOpCode::Stream__cancel_read:
  case ComponentCanonOpCode::Stream__cancel_write:
    EXPECTED_TRY(CheckTypeImmediate(true));
    return Push({I32V}, {I32V});
  case ComponentCanonOpCode::Stream__close_readable:
  case ComponentCanonOpCode::Stream__close_writable:
    EXPECTED_TRY(CheckTypeImmediate(true));
    return Push({I32V}, {});
  case ComponentCanonOpCode::Future__new:
    EXPECTED_TRY(CheckTypeImmediate(false));
    return Push({}, {I64V});
  case ComponentCanonOpCode::Future__read:
  case ComponentCanonOpCode::Future__write:
    EXPECTED_TRY(CheckTypeImmediate(false));
    return Push({I32V, I32V}, {I32V});
  case ComponentCanonOpCode::Future__cancel_read:
  case ComponentCanonOpCode::Future__cancel_write:
    EXPECTED_TRY(CheckTypeImmediate(false));
    return Push({I32V}, {I32V});
  case ComponentCanonOpCode::Future__close_readable:
  case ComponentCanonOpCode::Future__close_writable:
    EXPECTED_TRY(CheckTypeImmediate(false));
    return Push({I32V}, {});
  case ComponentCanonOpCode::Error_context__new:
    return Push({I32V, I32V}, {I32V});
  case ComponentCanonOpCode::Error_context__debug_message:
    return Push({I32V, I32V}, {});
  case ComponentCanonOpCode::Error_context__drop:
    return Push({I32V}, {});
  case ComponentCanonOpCode::Waitable_set__new:
    return Push({}, {I32V});
  case ComponentCanonOpCode::Waitable_set__wait:
  case ComponentCanonOpCode::Waitable_set__poll:
    return Push({I32V, I32V}, {I32V});
  case ComponentCanonOpCode::Waitable_set__drop:
    return Push({I32V}, {});
  case ComponentCanonOpCode::Waitable__join:
    return Push({I32V, I32V}, {});
  case ComponentCanonOpCode::Thread__new_indirect: {
    // The type immediate must be a core (func (param i32)) shape and the
    // target a core table.
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
    return Push({I32V, I32V}, {I32V});
  }
  case ComponentCanonOpCode::Thread__resume_later:
    return Push({I32V}, {});
  case ComponentCanonOpCode::Thread__suspend:
    return Push({}, {I32V});
  case ComponentCanonOpCode::Thread__suspend_then_resume:
  case ComponentCanonOpCode::Thread__yield_then_resume:
  case ComponentCanonOpCode::Thread__suspend_then_promote:
  case ComponentCanonOpCode::Thread__yield_then_promote:
    return Push({I32V}, {I32V});
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplValidator);
    spdlog::error("    canonical built-in {} is not supported yet."sv,
                  static_cast<uint32_t>(Canon.getOpCode()));
    return Unexpect(ErrCode::Value::ComponentNotImplValidator);
  }
}

} // namespace Validator
} // namespace WasmEdge
