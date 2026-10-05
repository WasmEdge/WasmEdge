// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- component_context.cpp - Component checker -------------------------===//
//
// The bodies of Context.
//
//===----------------------------------------------------------------------===//
#include "validator/component_context.h"

#include "common/component_valtype.h"
#include "common/errinfo.h"
#include "common/spdlog.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Validator {
namespace Component {

using namespace std::literals;
// ---------------------------------------------------------------------------
// Index-space registration and resolution.
// ---------------------------------------------------------------------------

void Context::addExtern(const ExternInfo &Info) noexcept {
  auto &S = getTop();
  switch (Info.Kind) {
  case ExternKind::CoreType:
    S.addCoreModule(Info.CoreMod);
    break;
  case ExternKind::FuncType:
    S.addFunc(Info.Func);
    break;
  case ExternKind::ValueBound:
    S.addValue(Info.Value);
    break;
  case ExternKind::TypeBound:
    S.addType(Info.Type);
    break;
  case ExternKind::InstanceType:
    S.addInstance(Info.Shape);
    break;
  case ExternKind::ComponentType:
    S.addComponent(Info.Shape);
    break;
  }
}

Expect<ExternInfo>
Context::getExtern(const AST::Component::SortIndex &SI) noexcept {
  ExternInfo Info;
  const auto &S = getTop();
  const auto &Sort = SI.getSort();
  const uint32_t Idx = SI.getIdx();
  if (Sort.isCore()) {
    if (Sort.getCoreSortType() == AST::Component::Sort::CoreSortType::Module) {
      const auto *Mod = S.getCoreModule(Idx);
      if (Mod == nullptr) {
        spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
        spdlog::error("    Core module index {} out of bounds (size {})."sv,
                      Idx, S.CoreModules.size());
        return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
      }
      Info.Kind = ExternKind::CoreType;
      Info.CoreMod = Mod;
      return Info;
    }
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error(
        "    Core sorts other than module cannot be used at component level."sv);
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  switch (Sort.getSortType()) {
  case AST::Component::Sort::SortType::Func: {
    const auto *F = S.getFunc(Idx);
    if (F == nullptr) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    Function index {} out of bounds (size {})."sv, Idx,
                    S.Funcs.size());
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    Info.Kind = ExternKind::FuncType;
    Info.Func = *F;
    return Info;
  }
  case AST::Component::Sort::SortType::Value: {
    if (Idx >= S.Values.size()) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    Value index {} out of bounds (size {})."sv, Idx,
                    S.Values.size());
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    Info.Kind = ExternKind::ValueBound;
    Info.Value = S.Values[Idx].Type;
    return Info;
  }
  case AST::Component::Sort::SortType::Type: {
    const auto *E = S.getType(Idx);
    if (E == nullptr) {
      spdlog::error(ErrCode::Value::ComponentTypeIndexOutOfBounds);
      spdlog::error("    Type index {} out of bounds (size {})."sv, Idx,
                    S.Types.size());
      return Unexpect(ErrCode::Value::ComponentTypeIndexOutOfBounds);
    }
    Info.Kind = ExternKind::TypeBound;
    Info.Type = *E;
    return Info;
  }
  case AST::Component::Sort::SortType::Component: {
    const auto *C = S.getComponent(Idx);
    if (C == nullptr) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    Component index {} out of bounds (size {})."sv, Idx,
                    S.Components.size());
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    Info.Kind = ExternKind::ComponentType;
    Info.Shape = C;
    return Info;
  }
  case AST::Component::Sort::SortType::Instance: {
    const auto *I = S.getInstance(Idx);
    if (I == nullptr) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    Instance index {} out of bounds (size {})."sv, Idx,
                    S.Instances.size());
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    Info.Kind = ExternKind::InstanceType;
    Info.Shape = I;
    return Info;
  }
  default:
    spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
    return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
  }
}

// valtype ::= i:<typeidx> | pvt:<primvaltype>; the index must be a defvaltype.
Expect<void> Context::validate(const ComponentValType &VT) const noexcept {
  if (VT.isPrimValType()) {
    return {};
  }
  const auto *Entry = getTop().getType(VT.getTypeIndex());
  if (Entry == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTypeIndexOutOfBounds);
    spdlog::error("    Value type index {} out of bounds (size {})."sv,
                  VT.getTypeIndex(), getTop().Types.size());
    return Unexpect(ErrCode::Value::ComponentTypeIndexOutOfBounds);
  }
  if (Entry->getDefValType() == nullptr) {
    spdlog::error(ErrCode::Value::NotADefinedType);
    spdlog::error("    Value type index {} does not refer to a value type."sv,
                  VT.getTypeIndex());
    return Unexpect(ErrCode::Value::NotADefinedType);
  }
  return {};
}

// ---------------------------------------------------------------------------
// Naming checks and the named-types rule; the only writer of NameSide.
// ---------------------------------------------------------------------------

Expect<ExternName> Context::parseExternName(std::string_view Name,
                                            bool IsImport) const noexcept {
  const auto Position = IsImport ? "Import"sv : "Export"sv;
  ExternName CN;
  EXPECTED_TRY(CN.parse(Name));
  if (CN.getKind() == ExternName::Kind::Invalid) {
    spdlog::error(ErrCode::Value::InvalidExternName);
    spdlog::error("    {} name '{}' is not a valid extern name."sv, Position,
                  Name);
    return Unexpect(ErrCode::Value::InvalidExternName);
  }
  return CN;
}

Expect<void> Context::addUniqueName(std::vector<NameRecord> &Names,
                                    const NameRecord &N,
                                    bool IsImport) const noexcept {
  // Declarator clashes carry their own code; exact duplicates never do.
  const auto SideCode = IsImport ? ErrCode::Value::ComponentImportNameConflict
                                 : ErrCode::Value::ComponentExportNameConflict;
  const auto ConflictCode =
      getTop().Kind == ScopeKind::Component
          ? SideCode
          : (IsImport ? ErrCode::Value::ComponentDeclImportNameConflict
                      : ErrCode::Value::ComponentDeclExportNameConflict);
  auto ReportClash = [&](ErrCode::Value Code) noexcept {
    spdlog::error(Code);
    spdlog::error("    {} name '{}' is not strongly-unique."sv,
                  IsImport ? "Import"sv : "Export"sv, N.Original);
    return Unexpect(Code);
  };
  for (const auto &E : Names) {
    if (E.Original == N.Original) {
      return ReportClash(SideCode);
    }
    if (E.Canonical == N.Canonical) {
      return ReportClash(ConflictCode);
    }
  }
  Names.push_back(N);
  return {};
}

Expect<void> Context::checkNameAttributes(const ExternName &CN,
                                          Span<const std::string> Impls,
                                          Span<const std::string> VSuffixes,
                                          bool IsInstance) const noexcept {
  for (const auto &V : VSuffixes) {
    EXPECTED_TRY(CN.checkVersionSuffix(V));
  }
  if (Impls.empty()) {
    return {};
  }
  for (const auto &I : Impls) {
    auto Parsed = parseExternName(I, true);
    if (!Parsed.has_value()) {
      spdlog::error(ErrCode::Value::ComponentImplementsName);
      spdlog::error("    `implements` value `{}` is not a valid name"sv, I);
      return Unexpect(ErrCode::Value::ComponentImplementsName);
    }
    if (Parsed->getKind() != ExternName::Kind::InterfaceType) {
      spdlog::error(ErrCode::Value::ComponentImplementsInterface);
      spdlog::error("    `implements` value `{}` must be an interface"sv, I);
      return Unexpect(ErrCode::Value::ComponentImplementsInterface);
    }
  }
  if (CN.getKind() != ExternName::Kind::Label &&
      CN.getKind() != ExternName::Kind::Constructor &&
      CN.getKind() != ExternName::Kind::Method &&
      CN.getKind() != ExternName::Kind::Static) {
    spdlog::error(ErrCode::Value::ComponentImplementsPlain);
    spdlog::error("    name `{}` is not valid with `implements`"sv,
                  CN.getOriginalName());
    return Unexpect(ErrCode::Value::ComponentImplementsPlain);
  }
  if (!IsInstance) {
    spdlog::error(ErrCode::Value::ComponentImplementsInstance);
    spdlog::error("    only instances can have an `implements`"sv);
    return Unexpect(ErrCode::Value::ComponentImplementsInstance);
  }
  return {};
}

Expect<void> Context::checkAnnotatedName(const ExternName &Name,
                                         const ExternInfo &Info,
                                         bool IsImport) noexcept {
  const auto Kind = Name.getKind();
  if (Kind != ExternName::Kind::Constructor &&
      Kind != ExternName::Kind::Method && Kind != ExternName::Kind::Static) {
    return {};
  }
  if (Info.Kind != ExternKind::FuncType || Info.Func.FT == nullptr) {
    spdlog::error(ErrCode::Value::ComponentIsNotFunc);
    spdlog::error(
        "    Annotated name '{}' is only allowed on function imports/exports."sv,
        Name.getOriginalName());
    return Unexpect(ErrCode::Value::ComponentIsNotFunc);
  }
  const std::string_view ResourceLabel = Name.getDetail().Resource;
  auto &S = getTop();
  const auto &Labels = S.getNameSide(IsImport).ResourceLabels;
  const auto &Names = S.getNameSide(IsImport).ResourceNames;
  const auto &FT = *Info.Func.FT;
  // The signature's resource must be named here as the annotation's label.
  auto CheckTarget = [&](uint32_t Target) noexcept -> Expect<void> {
    auto NameIt = Names.find(Target);
    if (NameIt == Names.end()) {
      spdlog::error(ErrCode::Value::ComponentResourceNotNamed);
      spdlog::error("    Resource used in '{}' has no name in this "
                    "context."sv,
                    Name.getOriginalName());
      return Unexpect(ErrCode::Value::ComponentResourceNotNamed);
    }
    if (NameIt->second != ResourceLabel) {
      spdlog::error(ErrCode::Value::AnnotatedFuncResourceName);
      spdlog::error("    '{}' does not match resource '{}'."sv,
                    Name.getOriginalName(), NameIt->second);
      return Unexpect(ErrCode::Value::AnnotatedFuncResourceName);
    }
    return {};
  };

  // Resolve a valtype to the resource behind its own or borrow handle.
  auto HandleOf = [this](const QualValType &Q,
                         bool WantOwn) noexcept -> std::optional<uint32_t> {
    TypeEntry Storage;
    const auto *Entry = getTypeEntry(Q, Storage);
    const auto *Def = Entry != nullptr ? Entry->getDefValType() : nullptr;
    if (Def == nullptr) {
      return std::nullopt;
    }
    const auto &DVT = *Def;
    uint32_t HandleIdx = 0;
    if (WantOwn && DVT.isOwnTy()) {
      HandleIdx = DVT.getOwn().Idx;
    } else if (!WantOwn && DVT.isBorrowTy()) {
      HandleIdx = DVT.getBorrow().Idx;
    } else {
      return std::nullopt;
    }
    const auto *Res = Entry->Home->getType(HandleIdx);
    if (Res == nullptr || !Res->isResource()) {
      return std::nullopt;
    }
    return applyRemap(Entry->Remap, *Res->ResourceId);
  };

  if (Kind == ExternName::Kind::Constructor) {
    // Signature first: exactly one result of (own T) or (result (own T) e?).
    if (!FT.getResult().has_value()) {
      spdlog::error(ErrCode::Value::AnnotatedCtorReturnOne);
      spdlog::error("    Constructor '{}' should return one value."sv,
                    Name.getOriginalName());
      return Unexpect(ErrCode::Value::AnnotatedCtorReturnOne);
    }
    QualValType Q{*FT.getResult(), Info.Func.Home, Info.Func.Remap};
    auto Target = HandleOf(Q, true);
    if (!Target.has_value()) {
      // Unwrap (result (own T) e?).
      TypeEntry Storage;
      const auto *Entry = getTypeEntry(Q, Storage);
      const auto *Def = Entry != nullptr ? Entry->getDefValType() : nullptr;
      if (Def != nullptr && Def->isResultTy()) {
        const auto &Res = Def->getResult();
        if (Res.ValTy.has_value()) {
          Target = HandleOf({*Res.ValTy, Entry->Home, Entry->Remap}, true);
        }
      }
    }
    if (!Target.has_value()) {
      spdlog::error(ErrCode::Value::AnnotatedCtorReturn);
      spdlog::error("    Constructor '{}' must return (own {})."sv,
                    Name.getOriginalName(), ResourceLabel);
      return Unexpect(ErrCode::Value::AnnotatedCtorReturn);
    }
    EXPECTED_TRY(CheckTarget(*Target));
  } else if (Kind == ExternName::Kind::Method) {
    if (FT.getParamList().empty()) {
      spdlog::error(ErrCode::Value::AnnotatedMethodArgs);
      spdlog::error("    Method '{}' should have at least one argument."sv,
                    Name.getOriginalName());
      return Unexpect(ErrCode::Value::AnnotatedMethodArgs);
    }
    const auto Params = FT.getParamList();
    const auto &Self = Params[0];
    if (Self.getLabel() != "self"sv) {
      spdlog::error(ErrCode::Value::AnnotatedMethodSelf);
      spdlog::error(
          "    Method '{}' should have a first argument called `self`."sv,
          Name.getOriginalName());
      return Unexpect(ErrCode::Value::AnnotatedMethodSelf);
    }
    auto Target =
        HandleOf({Self.getValType(), Info.Func.Home, Info.Func.Remap}, false);
    if (!Target.has_value()) {
      spdlog::error(ErrCode::Value::AnnotatedMethodBorrow);
      spdlog::error(
          "    Method '{}' should take a first argument of (borrow {})."sv,
          Name.getOriginalName(), ResourceLabel);
      return Unexpect(ErrCode::Value::AnnotatedMethodBorrow);
    }
    EXPECTED_TRY(CheckTarget(*Target));
  } else {
    const bool LabelKnown = Labels.count(std::string(ResourceLabel)) != 0;
    if (!LabelKnown) {
      spdlog::error(ErrCode::Value::AnnotatedStaticUnknown);
      spdlog::error(
          "    Static '{}' resource name is not known in this context."sv,
          Name.getOriginalName());
      return Unexpect(ErrCode::Value::AnnotatedStaticUnknown);
    }
  }
  return {};
}

void Context::addResourceLabel(const ExternName &Name, const ExternInfo &Info,
                               bool IsImport) noexcept {
  if (Name.getKind() == ExternName::Kind::Label &&
      Info.Kind == ExternKind::TypeBound && Info.Type.isResource()) {
    auto &S = getTop();
    auto &Labels = S.getNameSide(IsImport).ResourceLabels;
    auto &Names = S.getNameSide(IsImport).ResourceNames;
    Labels.emplace(std::string(Name.getOriginalName()), *Info.Type.ResourceId);
    Names.emplace(*Info.Type.ResourceId, std::string(Name.getOriginalName()));
  }
}

Expect<void> Context::checkNamedTypesRule(const ExternInfo &Info,
                                          bool IsImport) noexcept {
  // Instance-type declarations do not enforce the named-types rule.
  if (getTop().Kind == ScopeKind::InstanceType) {
    return {};
  }
  if (introduceExternTypes(Info, IsImport)) {
    return {};
  }
  ErrCode::Value Code;
  switch (Info.Kind) {
  case ExternKind::FuncType:
    Code = IsImport ? ErrCode::Value::ComponentFuncNotValidImport
                    : ErrCode::Value::ComponentFuncNotValidExport;
    break;
  case ExternKind::InstanceType:
    Code = IsImport ? ErrCode::Value::ComponentInstanceNotValidImport
                    : ErrCode::Value::ComponentInstanceNotValidExport;
    break;
  default:
    Code = IsImport ? ErrCode::Value::ComponentTypeNotValidImport
                    : ErrCode::Value::ComponentTypeNotValidExport;
    break;
  }
  spdlog::error(Code);
  spdlog::error(
      "    A referenced type or resource was not introduced by a preceding "
      "{}."sv,
      IsImport ? "import"sv : "import or export"sv);
  return Unexpect(Code);
}

// ---------------------------------------------------------------------------
// Declaring an import or an export; name errors precede ascription errors.
// ---------------------------------------------------------------------------

Expect<ExternInfo>
Context::addImport(std::string_view Name, const ExternInfo &Resolved,
                   Span<const std::string> Impls,
                   Span<const std::string> VSuffixes) noexcept {
  // Each instance import mints fresh identities for its declared resources.
  ExternInfo Info = Resolved;
  if (Info.Kind == ExternKind::InstanceType) {
    Info.Shape = copyDeclaredResources(Info.Shape, true);
  }
  EXPECTED_TRY(ExternName CN, parseExternName(Name, true));
  EXPECTED_TRY(checkNameAttributes(CN, Impls, VSuffixes,
                                   Info.Kind == ExternKind::InstanceType));
  EXPECTED_TRY(addUniqueName(getTop().ImportSide.Names, NameRecord(CN), true));
  addExtern(Info);
  EXPECTED_TRY(checkNamedTypesRule(Info, true));
  EXPECTED_TRY(checkAnnotatedName(CN, Info, true));
  addResourceLabel(CN, Info, true);
  return Info;
}

Expect<ExternName>
Context::registerExportName(std::string_view Name, bool IsInstance,
                            Span<const std::string> Impls,
                            Span<const std::string> VSuffixes) noexcept {
  EXPECTED_TRY(ExternName CN, parseExternName(Name, false));
  EXPECTED_TRY(checkNameAttributes(CN, Impls, VSuffixes, IsInstance));
  EXPECTED_TRY(addUniqueName(getTop().ExportSide.Names, NameRecord(CN), false));
  return CN;
}

Expect<ExternInfo>
Context::addExport(const ExternName &CN, const ExternInfo &Inferred,
                   const std::optional<ExternInfo> &Ascribed) noexcept {
  ExternInfo Result = Inferred;
  if (Ascribed.has_value()) {
    ResourceMap Subst;
    if (!Matcher::matchExtern(*this, Inferred, *Ascribed, Subst)) {
      spdlog::error(ErrCode::Value::ExportAscriptionIncompatible);
      spdlog::error(
          "    Ascribed type of export '{}' is not compatible with the "
          "exported definition."sv,
          CN.getOriginalName());
      return Unexpect(ErrCode::Value::ExportAscriptionIncompatible);
    }
    Result = *Ascribed;
  }
  // An export of a type re-introduces it under a fresh naming identity.
  if (Result.Kind == ExternKind::TypeBound) {
    Result.Type.NameId = nextNameId();
  }
  if (Result.Kind == ExternKind::InstanceType) {
    Result.Shape = copyDeclaredResources(Result.Shape, false);
  }
  addExtern(Result);
  EXPECTED_TRY(checkNamedTypesRule(Result, false));
  EXPECTED_TRY(checkAnnotatedName(CN, Result, false));
  addResourceLabel(CN, Result, false);
  return Result;
}

// ---------------------------------------------------------------------------
// Canonical options. They resolve core index spaces.
// ---------------------------------------------------------------------------

ValType Context::getCanonPtrType(
    const AST::Component::Canonical &Canon) const noexcept {
  for (const auto &Opt : Canon.getOptions()) {
    if (Opt.getCode() == ComponentCanonOptCode::Memory) {
      const uint32_t Idx = Opt.getIndex();
      if (Idx < getTop().CoreMemories.size()) {
        const auto *Mem = getTop().CoreMemories[Idx];
        return ValType(Mem != nullptr && Mem->getLimit().is64()
                           ? TypeCode::I64
                           : TypeCode::I32);
      }
    }
  }
  return ValType(TypeCode::I32);
}

Expect<void> Context::checkOptions(const AST::Component::Canonical &Canon,
                                   bool IsLift) const noexcept {
  const auto &S = getTop();
  const ValType I32V{TypeCode::I32};
  bool SeenEncoding = false, SeenMemory = false, SeenRealloc = false,
       SeenPostReturn = false, SeenAsync = false, SeenCallback = false;
  // The pointer width of realloc follows the selected memory.
  const ValType Ptr = getCanonPtrType(Canon);
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
      if (Idx >= S.CoreMemories.size()) {
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
      const auto *Func = S.getCoreFunc(Opt.getIndex());
      if (Func == nullptr) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error(
            "    Canonical option realloc function index {} out of bounds."sv,
            Opt.getIndex());
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
      // realloc has type [ptr ptr ptr ptr] -> [ptr] for the selected memory.
      const std::vector<ValType> ReallocParams(4, Ptr);
      const std::vector<ValType> ReallocResults(1, Ptr);
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
      // The signature is checked by the caller, once the flat type is known.
      break;
    case ComponentCanonOptCode::Async:
      if (SeenAsync) {
        spdlog::error(ErrCode::Value::CanonAsyncDuplicated);
        spdlog::error("    async is specified more than once."sv);
        return Unexpect(ErrCode::Value::CanonAsyncDuplicated);
      }
      SeenAsync = true;
      break;
    case ComponentCanonOptCode::Callback: {
      // `callback` may only appear on `canon lift`, and only with `async`.
      if (!IsLift) {
        spdlog::error(ErrCode::Value::CanonCallbackOnLower);
        spdlog::error("    callback cannot be specified for lowerings."sv);
        return Unexpect(ErrCode::Value::CanonCallbackOnLower);
      }
      if (SeenCallback) {
        spdlog::error(ErrCode::Value::CanonCallbackDuplicated);
        spdlog::error("    callback is specified more than once."sv);
        return Unexpect(ErrCode::Value::CanonCallbackDuplicated);
      }
      SeenCallback = true;
      const auto *Func = S.getCoreFunc(Opt.getIndex());
      if (Func == nullptr) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error("    Canonical option callback function index {} out "
                      "of bounds."sv,
                      Opt.getIndex());
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
      // callback has type [i32 i32 i32] -> [i32].
      const std::vector<ValType> CallbackParams(3, I32V);
      const std::vector<ValType> CallbackResults(1, I32V);
      const auto &CT = Func->getCompositeType();
      if (!CT.isFunc() || CT.getFuncType().getParamTypes() != CallbackParams ||
          CT.getFuncType().getReturnTypes() != CallbackResults) {
        spdlog::error(ErrCode::Value::CanonCallbackSignature);
        spdlog::error("    callback must have type [i32 i32 i32] -> [i32]."sv);
        return Unexpect(ErrCode::Value::CanonCallbackSignature);
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

Expect<void>
Context::checkRequiredOptions(const AST::Component::Canonical &Canon,
                              bool NeedMemory, bool NeedRealloc,
                              std::string_view What) const noexcept {
  if (NeedMemory && !Canon.hasOption(ComponentCanonOptCode::Memory)) {
    spdlog::error(ErrCode::Value::CanonMemoryRequired);
    spdlog::error("    {} requires the memory option."sv, What);
    return Unexpect(ErrCode::Value::CanonMemoryRequired);
  }
  if (NeedRealloc && !Canon.hasOption(ComponentCanonOptCode::Realloc)) {
    spdlog::error(ErrCode::Value::CanonReallocRequired);
    spdlog::error("    {} requires the realloc option."sv, What);
    return Unexpect(ErrCode::Value::CanonReallocRequired);
  }
  return {};
}

// ---------------------------------------------------------------------------
// Instantiation and the substitution applied to the views it creates.
// ---------------------------------------------------------------------------

Expect<const Shape *> Context::instantiateComponentShape(
    const Shape &CI,
    Span<const AST::Component::InstantiateArg<AST::Component::SortIndex>>
        Args) noexcept {
  // Resolve the arguments. The names must be unique.
  std::unordered_map<std::string_view, ExternInfo> ArgMap;
  for (const auto &Arg : Args) {
    EXPECTED_TRY(auto Info, getExtern(Arg.getIndex()));
    if (!ArgMap.emplace(Arg.getName(), Info).second) {
      spdlog::error(ErrCode::Value::ComponentDuplicateArg);
      spdlog::error("    Duplicate instantiation argument '{}'."sv,
                    Arg.getName());
      return Unexpect(ErrCode::Value::ComponentDuplicateArg);
    }
    // Values are consumed by being passed as arguments.
    if (!Arg.getIndex().getSort().isCore() &&
        Arg.getIndex().getSort().getSortType() ==
            AST::Component::Sort::SortType::Value) {
      EXPECTED_TRY(getTop().consumeValue(Arg.getIndex().getIdx()));
    }
  }
  // Match every import; Subst accumulates the resource substitution.
  ResourceMap Subst;
  for (const auto &[Name, Req] : CI.Imports) {
    auto It = ArgMap.find(Name);
    if (It == ArgMap.end()) {
      spdlog::error(ErrCode::Value::ComponentMissingImport);
      spdlog::error("    Missing instantiation argument '{}'."sv, Name);
      return Unexpect(ErrCode::Value::ComponentMissingImport);
    }
    if (auto Res = Matcher::matchExtern(*this, It->second, Req, Subst); !Res) {
      spdlog::error(Res.error());
      spdlog::error("    Instantiation argument '{}' has an incompatible "
                    "type."sv,
                    Name);
      return Unexpect(Res.error());
    }
  }

  // Combined remap: substituted imports + freshened defined resources.
  std::unordered_set<uint32_t> Reachable;
  for (const auto &[Name, E] : CI.Exports) {
    collectResources(E, Reachable);
  }
  auto *Node = addResourceMap();
  Node->Map = Subst.Map;
  for (const uint32_t Id : Reachable) {
    if (Node->Map.count(Id) != 0) {
      continue;
    }
    const auto &Entry = getResource(Id);
    if (!Entry.FromImport && CI.DeclScope != nullptr &&
        isDeclaredIn(Id, *CI.DeclScope)) {
      // A fresh resource belongs to the created instance: no definition body.
      Node->Map.emplace(Id, addResource(nullptr, &getTop(), false));
    }
  }

  const auto *Result = copyInstanceExports(CI, Node);
  ExternInfo Probe;
  Probe.Kind = ExternKind::InstanceType;
  Probe.Shape = Result;
  EXPECTED_TRY(checkTypeLimits(Probe));
  return Result;
}

const Shape *Context::copyDeclaredResources(const Shape *Inst,
                                            bool FromImport) noexcept {
  // Only declaration-built shapes carry their own declared resources.
  if (Inst == nullptr || Inst->DeclScope == nullptr ||
      Inst->DeclScope->Kind != ScopeKind::InstanceType) {
    return Inst;
  }
  std::unordered_set<uint32_t> Ids;
  ExternInfo Probe;
  Probe.Kind = ExternKind::InstanceType;
  Probe.Shape = Inst;
  collectResources(Probe, Ids);
  auto *Node = addResourceMap();
  for (const uint32_t Id : Ids) {
    if (isDeclaredIn(Id, *Inst->DeclScope)) {
      Node->Map.emplace(Id, addResource(nullptr, &getTop(), FromImport));
    }
  }
  if (Node->Map.empty()) {
    return Inst;
  }
  std::unordered_map<const Shape *, const Shape *> Copies;
  return copyShape(Inst, Node, Copies);
}

const Shape *Context::copyInstanceExports(const Shape &CI,
                                          const ResourceMap *Node) noexcept {
  std::unordered_map<const Shape *, const Shape *> Copies;
  auto *Result = addShape();
  Result->DeclScope = CI.DeclScope;
  for (const auto &[Name, E] : CI.Exports) {
    Result->Exports.emplace(Name, copyExtern(E, Node, Copies));
    Result->ExportOrder.emplace_back(Name);
  }
  return Result;
}

// ---------------------------------------------------------------------------
// Resolution: the shared substrate of every walk below.
// ---------------------------------------------------------------------------

const TypeEntry *Context::getTypeEntry(const QualValType &Q,
                                       TypeEntry &Storage) noexcept {
  if (Q.VT.isPrimValType() || Q.Home == nullptr) {
    return nullptr;
  }
  const auto *Entry = Q.Home->getType(Q.VT.getTypeIndex());
  if (Entry == nullptr) {
    return nullptr;
  }
  Storage = *Entry;
  if (Entry->isResource()) {
    Storage.ResourceId = applyRemap(Q.Remap, *Entry->ResourceId);
  }
  Storage.Remap = composeRemap(Q.Remap, Entry->Remap);
  return &Storage;
}

// The primitive behind a valtype, through prim-alias indirections.
std::optional<PrimValType>
Context::getPrimValType(const QualValType &Q) noexcept {
  if (Q.VT.isPrimValType()) {
    return Q.VT.getPrimValType();
  }
  TypeEntry Storage;
  const auto *Entry = getTypeEntry(Q, Storage);
  const auto *Def = Entry != nullptr ? Entry->getDefValType() : nullptr;
  if (Def == nullptr || !Def->isPrimValType()) {
    return std::nullopt;
  }
  return Def->getPrimValType();
}

std::optional<uint32_t> Context::getResourceId(const Scope *Home,
                                               const ResourceMap *Remap,
                                               uint32_t Idx) const noexcept {
  if (Home == nullptr) {
    return std::nullopt;
  }
  const auto *Entry = Home->getType(Idx);
  if (Entry == nullptr || !Entry->isResource()) {
    return std::nullopt;
  }
  return applyRemap(Remap, *Entry->ResourceId);
}

// ---------------------------------------------------------------------------
// Resource-id walks.
// ---------------------------------------------------------------------------

bool Context::hasBorrow(const QualValType &Q) noexcept {
  TypeEntry Storage;
  const auto *Entry = getTypeEntry(Q, Storage);
  const auto *Def = Entry != nullptr ? Entry->getDefValType() : nullptr;
  if (Def == nullptr || Def->isPrimValType()) {
    return false;
  }
  const auto &D = *Def;
  if (D.isBorrowTy()) {
    return true;
  }
  // Stream and future payloads are borrow-free, so the containers suffice.
  bool Found = false;
  forEachValType(D, [&](const ComponentValType &VT) noexcept {
    Found = Found || hasBorrow({VT, Entry->Home, Entry->Remap});
  });
  return Found;
}

void Context::collectResources(const ExternInfo &Info,
                               std::unordered_set<uint32_t> &Out) noexcept {
  switch (Info.Kind) {
  case ExternKind::CoreType:
    return;
  case ExternKind::FuncType: {
    if (Info.Func.FT == nullptr) {
      return;
    }
    for (const auto &P : Info.Func.FT->getParamList()) {
      collectResources({P.getValType(), Info.Func.Home, Info.Func.Remap}, Out);
    }
    if (const auto &R = Info.Func.FT->getResult(); R.has_value()) {
      collectResources({*R, Info.Func.Home, Info.Func.Remap}, Out);
    }
    return;
  }
  case ExternKind::ValueBound:
    collectResources(Info.Value, Out);
    return;
  case ExternKind::TypeBound: {
    const auto &E = Info.Type;
    if (E.isResource()) {
      Out.insert(*E.ResourceId);
      return;
    }
    if (E.Inst != nullptr) {
      for (const auto &[Name, Sub] : E.Inst->Exports) {
        collectResources(Sub, Out);
      }
      return;
    }
    if (E.Comp != nullptr) {
      for (const auto &[Name, Sub] : E.Comp->Imports) {
        collectResources(Sub, Out);
      }
      for (const auto &[Name, Sub] : E.Comp->Exports) {
        collectResources(Sub, Out);
      }
      return;
    }
    if (E.getDefValType() != nullptr) {
      collectResources(E, Out);
      return;
    }
    if (const auto *FT = E.getFuncType()) {
      for (const auto &P : FT->getParamList()) {
        collectResources({P.getValType(), E.Home, E.Remap}, Out);
      }
      if (const auto &R = FT->getResult(); R.has_value()) {
        collectResources({*R, E.Home, E.Remap}, Out);
      }
    }
    return;
  }
  case ExternKind::InstanceType:
  case ExternKind::ComponentType:
    // Instances carry no imports, so one walk covers both shapes.
    if (Info.Shape != nullptr) {
      for (const auto &[Name, Sub] : Info.Shape->Imports) {
        collectResources(Sub, Out);
      }
      for (const auto &[Name, Sub] : Info.Shape->Exports) {
        collectResources(Sub, Out);
      }
    }
    return;
  }
}

void Context::collectResources(const QualValType &Q,
                               std::unordered_set<uint32_t> &Out) noexcept {
  TypeEntry Storage;
  if (const auto *Entry = getTypeEntry(Q, Storage)) {
    collectResources(*Entry, Out);
  }
}

bool Context::isDeclaredIn(uint32_t Id, const Scope &S) const noexcept {
  const auto &Entry = getResource(Id);
  for (const auto *Cur = Entry.Origin; Cur != nullptr; Cur = Cur->Parent) {
    if (Cur == &S) {
      return true;
    }
  }
  return false;
}

// ---------------------------------------------------------------------------
// Effective type size and nesting depth.
// ---------------------------------------------------------------------------

uint64_t Context::getTypeSize(const ExternInfo &Info) noexcept {
  switch (Info.Kind) {
  case ExternKind::CoreType:
    return Info.CoreMod != nullptr
               ? 1 + Info.CoreMod->Imports.size() + Info.CoreMod->Exports.size()
               : 1;
  case ExternKind::FuncType: {
    if (Info.Func.FT == nullptr) {
      return 1;
    }
    auto It = TypeSizes.find(Info.Func.FT);
    if (It != TypeSizes.end()) {
      return It->second;
    }
    uint64_t Size = 1;
    for (const auto &P : Info.Func.FT->getParamList()) {
      Size += getTypeSize({P.getValType(), Info.Func.Home, Info.Func.Remap});
    }
    if (const auto &R = Info.Func.FT->getResult(); R.has_value()) {
      Size += getTypeSize({*R, Info.Func.Home, Info.Func.Remap});
    }
    TypeSizes.emplace(Info.Func.FT, Size);
    return Size;
  }
  case ExternKind::ValueBound:
    return 1 + getTypeSize(Info.Value);
  case ExternKind::TypeBound: {
    const auto &E = Info.Type;
    if (E.isResource()) {
      return 1;
    }
    if (E.Inst != nullptr || E.Comp != nullptr) {
      const void *Key = E.Inst != nullptr ? static_cast<const void *>(E.Inst)
                                          : static_cast<const void *>(E.Comp);
      auto It = TypeSizes.find(Key);
      if (It != TypeSizes.end()) {
        return It->second;
      }
      TypeSizes.emplace(Key, 1);
      uint64_t Size = 1;
      if (E.Inst != nullptr) {
        for (const auto &[Name, Sub] : E.Inst->Exports) {
          Size += 1 + getTypeSize(Sub);
        }
      } else {
        for (const auto &[Name, Sub] : E.Comp->Imports) {
          Size += 1 + getTypeSize(Sub);
        }
        for (const auto &[Name, Sub] : E.Comp->Exports) {
          Size += 1 + getTypeSize(Sub);
        }
      }
      TypeSizes[Key] = Size;
      return Size;
    }
    if (E.getDefValType() != nullptr) {
      return getTypeSize(E);
    }
    if (E.getFuncType() != nullptr) {
      ExternInfo FI;
      FI.Kind = ExternKind::FuncType;
      FI.Func = {E.getFuncType(), E.Home, E.Remap};
      return getTypeSize(FI);
    }
    return 1;
  }
  case ExternKind::InstanceType:
  case ExternKind::ComponentType: {
    // Instances carry no imports, so one walk covers both shapes.
    if (Info.Shape == nullptr) {
      return 1;
    }
    auto It = TypeSizes.find(Info.Shape);
    if (It != TypeSizes.end()) {
      return It->second;
    }
    TypeSizes.emplace(Info.Shape, 1);
    uint64_t Size = 1;
    for (const auto &[Name, Sub] : Info.Shape->Imports) {
      Size += 1 + getTypeSize(Sub);
    }
    for (const auto &[Name, Sub] : Info.Shape->Exports) {
      Size += 1 + getTypeSize(Sub);
    }
    TypeSizes[Info.Shape] = Size;
    return Size;
  }
  }
  return 1;
}

uint64_t Context::getTypeDepth(const ExternInfo &Info) noexcept {
  uint64_t Max = 0;
  switch (Info.Kind) {
  case ExternKind::FuncType:
    if (Info.Func.FT != nullptr) {
      for (const auto &P : Info.Func.FT->getParamList()) {
        Max = std::max(Max, getTypeDepth({P.getValType(), Info.Func.Home,
                                          Info.Func.Remap}));
      }
      if (const auto &R = Info.Func.FT->getResult(); R.has_value()) {
        Max =
            std::max(Max, getTypeDepth({*R, Info.Func.Home, Info.Func.Remap}));
      }
    }
    break;
  case ExternKind::TypeBound: {
    const auto &E = Info.Type;
    if (E.Inst != nullptr || E.Comp != nullptr) {
      ExternInfo Probe;
      Probe.Kind = E.Inst != nullptr ? ExternKind::InstanceType
                                     : ExternKind::ComponentType;
      Probe.Shape = E.Inst != nullptr ? E.Inst : E.Comp;
      Max = getTypeDepth(Probe);
    } else if (E.getDefValType() != nullptr) {
      Max = getTypeDepth(E);
    } else if (E.getFuncType() != nullptr) {
      ExternInfo Probe;
      Probe.Kind = ExternKind::FuncType;
      Probe.Func = {E.getFuncType(), E.Home, E.Remap};
      Max = getTypeDepth(Probe);
    }
    break;
  }
  case ExternKind::ValueBound:
    Max = getTypeDepth(Info.Value);
    break;
  case ExternKind::InstanceType:
  case ExternKind::ComponentType: {
    // A shape is one nesting level; one walk covers both shapes.
    if (Info.Shape == nullptr) {
      return 1;
    }
    auto It = TypeDepths.find(Info.Shape);
    if (It != TypeDepths.end()) {
      return It->second;
    }
    TypeDepths.emplace(Info.Shape, 1); // Break cycles defensively.
    for (const auto &[Name, E] : Info.Shape->Imports) {
      Max = std::max(Max, getTypeDepth(E));
    }
    for (const auto &[Name, E] : Info.Shape->Exports) {
      Max = std::max(Max, getTypeDepth(E));
    }
    TypeDepths[Info.Shape] = Max + 1;
    return Max + 1;
  }
  case ExternKind::CoreType:
    // Core types carry no component value types.
    break;
  }
  return Max;
}

Expect<void> Context::checkTypeLimits(const ExternInfo &Info) noexcept {
  const uint64_t Size = getTypeSize(Info);
  if (Size >= MaxTypeSize) {
    spdlog::error(ErrCode::Value::ComponentTypeSizeLimit);
    spdlog::error("    Effective type size {} exceeds the limit of {}."sv, Size,
                  MaxTypeSize);
    return Unexpect(ErrCode::Value::ComponentTypeSizeLimit);
  }
  const uint64_t Depth = getTypeDepth(Info);
  if (Depth > MaxTypeDepth) {
    spdlog::error(ErrCode::Value::ComponentTypeNestingDepth);
    spdlog::error("    Value type nesting depth {} exceeds the limit."sv,
                  Depth);
    return Unexpect(ErrCode::Value::ComponentTypeNestingDepth);
  }
  return {};
}

// ---------------------------------------------------------------------------
// Canonical ABI layout (spec `elem_size` and `alignment`, 64-bit pointers).
// ---------------------------------------------------------------------------

// Element size and alignment of a value type, as the Canonical ABI lays it
// out in a 64-bit memory.
std::pair<uint64_t, uint64_t>
Context::getElemLayout(const QualValType &Q) noexcept {
  if (Q.VT.isPrimValType()) {
    switch (Q.VT.getPrimValType()) {
    case PrimValType::Bool:
    case PrimValType::S8:
    case PrimValType::U8:
      return {1, 1};
    case PrimValType::S16:
    case PrimValType::U16:
      return {2, 2};
    case PrimValType::S64:
    case PrimValType::U64:
    case PrimValType::F64:
      return {8, 8};
    case PrimValType::String:
      return {16, 8};
    default:
      return {4, 4};
    }
  }
  TypeEntry Storage;
  const auto *Entry = getTypeEntry(Q, Storage);
  const auto *Def = Entry != nullptr ? Entry->getDefValType() : nullptr;
  if (Def == nullptr) {
    return {4, 4};
  }
  return getElemLayout(*Def, Entry->Home, Entry->Remap);
}

std::pair<uint64_t, uint64_t>
Context::getElemLayout(const AST::Component::DefValType &D, const Scope *Home,
                       const ResourceMap *Remap) noexcept {
  if (D.isPrimValType()) {
    return getElemLayout({ComponentValType(D.getPrimValType()), Home, Remap});
  }
  auto It = ElemLayouts.find(&D);
  if (It != ElemLayouts.end()) {
    return It->second;
  }
  auto AlignTo = [](uint64_t Ptr, uint64_t Align) noexcept {
    return (Ptr + Align - 1) / Align * Align;
  };
  auto Layout = [&](const ComponentValType &VT) noexcept {
    return getElemLayout({VT, Home, Remap});
  };
  // A record lays its fields out in order.
  auto RecordLayout = [&](const auto &Fields) noexcept {
    uint64_t Size = 0;
    uint64_t Align = 1;
    for (const auto &VT : Fields) {
      const auto [FSize, FAlign] = Layout(VT);
      Size = AlignTo(Size, FAlign) + FSize;
      Align = std::max(Align, FAlign);
    }
    return std::make_pair(AlignTo(Size, Align), Align);
  };
  // A variant is a discriminant followed by the largest payload.
  auto VariantLayout = [&](size_t NumCases, const auto &Payloads) noexcept {
    const uint64_t Disc = NumCases <= (UINT64_C(1) << 8)    ? 1
                          : NumCases <= (UINT64_C(1) << 16) ? 2
                                                            : 4;
    uint64_t CaseSize = 0;
    uint64_t Align = Disc;
    for (const auto &VT : Payloads) {
      const auto [PSize, PAlign] = Layout(VT);
      CaseSize = std::max(CaseSize, PSize);
      Align = std::max(Align, PAlign);
    }
    return std::make_pair(AlignTo(AlignTo(Disc, Align) + CaseSize, Align),
                          Align);
  };
  std::pair<uint64_t, uint64_t> Res{4, 4};
  if (D.isRecordTy()) {
    std::vector<ComponentValType> Fields;
    for (const auto &LT : D.getRecord().LabelTypes) {
      Fields.push_back(LT.getValType());
    }
    Res = RecordLayout(Fields);
  } else if (D.isTupleTy()) {
    Res = RecordLayout(D.getTuple().Types);
  } else if (D.isVariantTy()) {
    std::vector<ComponentValType> Payloads;
    for (const auto &[Label, Ty] : D.getVariant().Cases) {
      if (Ty.has_value()) {
        Payloads.push_back(*Ty);
      }
    }
    Res = VariantLayout(D.getVariant().Cases.size(), Payloads);
  } else if (D.isEnumTy()) {
    Res = VariantLayout(D.getEnum().Labels.size(),
                        std::vector<ComponentValType>{});
  } else if (D.isOptionTy()) {
    Res = VariantLayout(2, std::vector{D.getOption().ValTy});
  } else if (D.isResultTy()) {
    std::vector<ComponentValType> Payloads;
    if (D.getResult().ValTy.has_value()) {
      Payloads.push_back(*D.getResult().ValTy);
    }
    if (D.getResult().ErrTy.has_value()) {
      Payloads.push_back(*D.getResult().ErrTy);
    }
    Res = VariantLayout(2, Payloads);
  } else if (D.isFlagsTy()) {
    const size_t N = D.getFlags().Labels.size();
    const uint64_t Size = N <= 8 ? 1 : N <= 16 ? 2 : 4;
    Res = {Size, Size};
  } else if (D.isListTy()) {
    const auto &List = D.getList();
    if (List.Len.has_value()) {
      const auto [ESize, EAlign] = Layout(List.ValTy);
      Res = {*List.Len * ESize, EAlign};
    } else {
      Res = {16, 8};
    }
  } else if (D.isMapTy()) {
    Res = {16, 8};
  }
  ElemLayouts.emplace(&D, Res);
  return Res;
}

// ---------------------------------------------------------------------------
// Canonical ABI flattening (spec `flatten_functype`).
// ---------------------------------------------------------------------------

// Spec flatten_type: appends the flat core types of Q; false if unflattenable.
bool Context::flattenValType(const QualValType &Q, std::vector<ValType> &Out,
                             const ValType &Ptr) noexcept {
  if (Out.size() > MaxFlatExpand) {
    return true;
  }
  if (const auto Prim = getPrimValType(Q)) {
    switch (*Prim) {
    case PrimValType::Bool:
    case PrimValType::S8:
    case PrimValType::U8:
    case PrimValType::S16:
    case PrimValType::U16:
    case PrimValType::S32:
    case PrimValType::U32:
    case PrimValType::Char:
      Out.push_back(ValType(TypeCode::I32));
      return true;
    case PrimValType::S64:
    case PrimValType::U64:
      Out.push_back(ValType(TypeCode::I64));
      return true;
    case PrimValType::F32:
      Out.push_back(ValType(TypeCode::F32));
      return true;
    case PrimValType::F64:
      Out.push_back(ValType(TypeCode::F64));
      return true;
    case PrimValType::String:
      Out.push_back(Ptr);
      Out.push_back(Ptr);
      return true;
    case PrimValType::ErrorContext:
      Out.push_back(ValType(TypeCode::I32));
      return true;
    default:
      return false;
    }
  }
  TypeEntry Storage;
  const auto *Entry = getTypeEntry(Q, Storage);
  const auto *Def = Entry != nullptr ? Entry->getDefValType() : nullptr;
  if (Def == nullptr) {
    return false;
  }
  const auto &D = *Def;
  auto Sub = [&](const ComponentValType &VT) noexcept {
    return flattenValType({VT, Entry->Home, Entry->Remap}, Out, Ptr);
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
      for (uint32_t I = 0; I < *L.Len && Out.size() <= MaxFlatExpand; ++I) {
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
  if (D.isMapTy()) {
    // (map k v) flattens like (list (tuple k v)).
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
        if (!flattenValType({Ty, Entry->Home, Entry->Remap}, Flat, Ptr)) {
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

// True iff the type transitively contains a list, map, or string.
bool Context::needsMemory(const QualValType &Q) noexcept {
  if (const auto Prim = getPrimValType(Q)) {
    return *Prim == PrimValType::String;
  }
  TypeEntry Storage;
  const auto *Entry = getTypeEntry(Q, Storage);
  const auto *Def = Entry != nullptr ? Entry->getDefValType() : nullptr;
  if (Def == nullptr) {
    return false;
  }
  const auto &D = *Def;
  if (D.isListTy() || D.isMapTy()) {
    return true;
  }
  bool Found = false;
  forEachValType(D, [&](const ComponentValType &VT) noexcept {
    Found = Found || needsMemory({VT, Entry->Home, Entry->Remap});
  });
  return Found;
}

Expect<AST::FunctionType>
Context::flattenFuncType(const FuncInfo &FI, const ValType &Ptr,
                         bool &ParamsNeedMemory,
                         bool &ResultsNeedMemory) noexcept {
  AST::FunctionType Sig;
  ParamsNeedMemory = false;
  ResultsNeedMemory = false;
  if (FI.FT == nullptr) {
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  for (const auto &P : FI.FT->getParamList()) {
    const QualValType Q{P.getValType(), FI.Home, FI.Remap};
    if (!flattenValType(Q, Sig.getParamTypes(), Ptr)) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    ParamsNeedMemory = ParamsNeedMemory || needsMemory(Q);
  }
  if (const auto &R = FI.FT->getResult(); R.has_value()) {
    const QualValType Q{*R, FI.Home, FI.Remap};
    if (!flattenValType(Q, Sig.getReturnTypes(), Ptr)) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    ResultsNeedMemory = ResultsNeedMemory || needsMemory(Q);
  }
  return Sig;
}

// ---------------------------------------------------------------------------
// The named-types rule: introduceExternTypes checks and records an extern.
// ---------------------------------------------------------------------------

// Validate and register an extern for the named-types rule.
bool Context::introduceExternTypes(const ExternInfo &Info,
                                   bool IsImport) noexcept {
  auto &S = getTop();
  switch (Info.Kind) {
  case ExternKind::CoreType:
  case ExternKind::ComponentType:
    return true;
  case ExternKind::TypeBound: {
    if (!areInnerTypesIntroduced(Info.Type, IsImport)) {
      return false;
    }
    // Introduce: imported types are usable by exports as well.
    if (Info.Type.isResource() && Info.Type.NameId.has_value()) {
      if (IsImport) {
        S.ImportSide.NamedResources.insert(*Info.Type.NameId);
        S.ExportSide.NamedResources.insert(*Info.Type.NameId);
      } else {
        S.ExportSide.NamedResources.insert(*Info.Type.NameId);
      }
    } else if (Info.Type.getDefValType() != nullptr) {
      if (IsImport) {
        S.ImportSide.NamedTypes.emplace(Info.Type.DT, Info.Type.Home);
        S.ExportSide.NamedTypes.emplace(Info.Type.DT, Info.Type.Home);
        if (Info.Type.NameId.has_value()) {
          S.ImportSide.NamedIds.insert(*Info.Type.NameId);
          S.ExportSide.NamedIds.insert(*Info.Type.NameId);
        }
      } else {
        S.ExportSide.NamedTypes.emplace(Info.Type.DT, Info.Type.Home);
        if (Info.Type.NameId.has_value()) {
          S.ExportSide.NamedIds.insert(*Info.Type.NameId);
        }
      }
    }
    return true;
  }
  case ExternKind::InstanceType: {
    if (Info.Shape == nullptr) {
      return true;
    }
    auto Walk = [&](const ExternInfo &Sub) noexcept {
      return introduceExternTypes(Sub, IsImport);
    };
    if (!Info.Shape->ExportOrder.empty()) {
      for (const auto &Name : Info.Shape->ExportOrder) {
        auto It = Info.Shape->Exports.find(Name);
        if (It != Info.Shape->Exports.end() && !Walk(It->second)) {
          return false;
        }
      }
      return true;
    }
    for (const auto &[Name, Sub] : Info.Shape->Exports) {
      if (!Walk(Sub)) {
        return false;
      }
    }
    return true;
  }
  case ExternKind::FuncType: {
    if (Info.Func.FT == nullptr) {
      return true;
    }
    for (const auto &P : Info.Func.FT->getParamList()) {
      if (!isIntroduced({P.getValType(), Info.Func.Home, Info.Func.Remap},
                        IsImport)) {
        return false;
      }
    }
    if (const auto &R = Info.Func.FT->getResult();
        R.has_value() &&
        !isIntroduced({*R, Info.Func.Home, Info.Func.Remap}, IsImport)) {
      return false;
    }
    return true;
  }
  case ExternKind::ValueBound:
    return isIntroduced(Info.Value, IsImport);
  }
  return true;
}

bool Context::isIntroduced(const QualValType &Q, bool IsImport) noexcept {
  if (Q.VT.isPrimValType() || Q.Home == nullptr) {
    return true;
  }
  TypeEntry Storage;
  const auto *Entry = getTypeEntry(Q, Storage);
  if (Entry == nullptr) {
    return true;
  }
  return isIntroduced(*Entry, IsImport);
}

bool Context::isIntroduced(const TypeEntry &E, bool IsImport) noexcept {
  auto &S = getTop();
  const auto &NamedTys = S.getNameSide(IsImport).NamedTypes;
  const auto &NamedRes = S.getNameSide(IsImport).NamedResources;
  if (E.isResource()) {
    return E.NameId.has_value() && NamedRes.count(*E.NameId) != 0;
  }
  const auto *Def = E.getDefValType();
  if (Def == nullptr) {
    return true;
  }
  const auto &D = *Def;
  if (D.isPrimValType()) {
    return true;
  }
  // Local: the introduced identity. Foreign: a structurally equal named type.
  if (D.isFlagsTy() || D.isEnumTy() || D.isRecordTy() || D.isVariantTy()) {
    const auto &NamedIds = S.getNameSide(IsImport).NamedIds;
    if (E.Home == &S) {
      return E.NameId.has_value() && NamedIds.count(*E.NameId) != 0;
    }
    if (E.NameId.has_value() && NamedIds.count(*E.NameId) != 0) {
      return true;
    }
    if (NamedTys.count(E.DT) != 0) {
      return true;
    }
    for (const auto &[Named, Home] : NamedTys) {
      if (Named->isDefValType()) {
        TypeEntry Probe;
        Probe.DT = Named;
        Probe.Home = Home;
        ResourceMap Subst;
        if (Matcher::matchValType(*this, E, Probe, Subst)) {
          return true;
        }
      }
    }
    return false;
  }
  auto Sub = [&](const ComponentValType &VT) noexcept {
    return isIntroduced({VT, E.Home, E.Remap}, IsImport);
  };
  if (D.isStreamTy() || D.isFutureTy()) {
    const auto &Elem =
        D.isStreamTy() ? D.getStream().ValTy : D.getFuture().ValTy;
    return !Elem.has_value() || Sub(*Elem);
  }
  if (D.isOwnTy() || D.isBorrowTy()) {
    const uint32_t Idx = D.isOwnTy() ? D.getOwn().Idx : D.getBorrow().Idx;
    const auto *Res = E.Home->getType(Idx);
    if (Res == nullptr || !Res->isResource()) {
      return true;
    }
    const uint32_t Eff = applyRemap(E.Remap, *Res->ResourceId);
    const uint32_t NameId = Eff != *Res->ResourceId
                                ? getResource(Eff).NameId
                                : Res->NameId.value_or(getResource(Eff).NameId);
    return NamedRes.count(NameId) != 0;
  }
  bool All = true;
  forEachValType(
      D, [&](const ComponentValType &VT) noexcept { All = All && Sub(VT); });
  return All;
}

// The introduced type is exempt; its immediate components must be named.
bool Context::areInnerTypesIntroduced(const TypeEntry &E,
                                      bool IsImport) noexcept {
  if (E.isResource()) {
    return true;
  }
  if (E.Comp != nullptr) {
    return true;
  }
  if (E.Inst != nullptr) {
    for (const auto &[Name, Sub] : E.Inst->Exports) {
      if (!introduceExternTypes(Sub, IsImport)) {
        return false;
      }
    }
    return true;
  }
  if (const auto *FT = E.getFuncType()) {
    for (const auto &P : FT->getParamList()) {
      if (!isIntroduced({P.getValType(), E.Home, E.Remap}, IsImport)) {
        return false;
      }
    }
    if (const auto &R = FT->getResult();
        R.has_value() && !isIntroduced({*R, E.Home, E.Remap}, IsImport)) {
      return false;
    }
    return true;
  }
  const auto *Def = E.getDefValType();
  if (Def == nullptr) {
    return true;
  }
  const auto &D = *Def;
  if (D.isPrimValType() || D.isFlagsTy() || D.isEnumTy()) {
    return true;
  }
  auto Sub = [&](const ComponentValType &VT) noexcept {
    return isIntroduced({VT, E.Home, E.Remap}, IsImport);
  };
  if (D.isStreamTy() || D.isFutureTy()) {
    const auto &Elem =
        D.isStreamTy() ? D.getStream().ValTy : D.getFuture().ValTy;
    return !Elem.has_value() || Sub(*Elem);
  }
  if (D.isOwnTy() || D.isBorrowTy()) {
    const auto &NamedRes = getTop().getNameSide(IsImport).NamedResources;
    const uint32_t Idx = D.isOwnTy() ? D.getOwn().Idx : D.getBorrow().Idx;
    const auto *Res = E.Home->getType(Idx);
    if (Res == nullptr || !Res->isResource()) {
      return true;
    }
    const uint32_t Eff = applyRemap(E.Remap, *Res->ResourceId);
    const uint32_t NameId = Eff != *Res->ResourceId
                                ? getResource(Eff).NameId
                                : Res->NameId.value_or(getResource(Eff).NameId);
    return NamedRes.count(NameId) != 0;
  }
  bool All = true;
  forEachValType(
      D, [&](const ComponentValType &VT) noexcept { All = All && Sub(VT); });
  return All;
}

// ---------------------------------------------------------------------------
// The leaves of the walks above.
// ---------------------------------------------------------------------------

void Context::collectResources(const TypeEntry &E,
                               std::unordered_set<uint32_t> &Out) noexcept {
  const auto *Def = E.getDefValType();
  if (Def == nullptr || Def->isPrimValType()) {
    return;
  }
  const auto &D = *Def;
  auto Sub = [&](const ComponentValType &VT) noexcept {
    collectResources({VT, E.Home, E.Remap}, Out);
  };
  if (D.isOwnTy() || D.isBorrowTy()) {
    const uint32_t Idx = D.isOwnTy() ? D.getOwn().Idx : D.getBorrow().Idx;
    if (auto Id = getResourceId(E.Home, E.Remap, Idx)) {
      Out.insert(*Id);
    }
    return;
  }
  // A stream or future payload can reach a resource too.
  if (D.isStreamTy() || D.isFutureTy()) {
    const auto &Elem =
        D.isStreamTy() ? D.getStream().ValTy : D.getFuture().ValTy;
    if (Elem.has_value()) {
      Sub(*Elem);
    }
    return;
  }
  forEachValType(D, Sub);
}

// Effective size of a value type (the spec limit metric), kept per node.
uint64_t Context::getTypeSize(const QualValType &Q) noexcept {
  TypeEntry Storage;
  const auto *Entry = getTypeEntry(Q, Storage);
  return Entry != nullptr ? getTypeSize(*Entry) : 1;
}

uint64_t Context::getTypeSize(const TypeEntry &E) noexcept {
  const auto *Def = E.getDefValType();
  if (Def == nullptr || Def->isPrimValType()) {
    return 1;
  }
  auto It = TypeSizes.find(Def);
  if (It != TypeSizes.end()) {
    return It->second;
  }
  TypeSizes.emplace(Def, 1); // Break cycles defensively.
  const auto &D = *Def;
  uint64_t Size = 1;
  // A payload-less variant case and every flags or enum label count one.
  if (D.isVariantTy()) {
    for (const auto &[Label, Ty] : D.getVariant().Cases) {
      if (!Ty.has_value()) {
        Size += 1;
      }
    }
  } else if (D.isFlagsTy()) {
    Size += D.getFlags().Labels.size();
  } else if (D.isEnumTy()) {
    Size += D.getEnum().Labels.size();
  }
  forEachValType(D, [&](const ComponentValType &VT) noexcept {
    Size += getTypeSize({VT, E.Home, E.Remap});
  });
  TypeSizes[Def] = Size;
  return Size;
}

// Depth of a value type: leaves count 1, wrappers add 1.
uint64_t Context::getTypeDepth(const QualValType &Q) noexcept {
  TypeEntry Storage;
  const auto *Entry = getTypeEntry(Q, Storage);
  return Entry != nullptr ? getTypeDepth(*Entry) : 1;
}

uint64_t Context::getTypeDepth(const TypeEntry &E) noexcept {
  const auto *Def = E.getDefValType();
  if (Def == nullptr || Def->isPrimValType()) {
    return 1;
  }
  auto It = TypeDepths.find(Def);
  if (It != TypeDepths.end()) {
    return It->second;
  }
  TypeDepths.emplace(Def, 1); // Break cycles defensively.
  const auto &D = *Def;
  uint64_t Max = 0;
  forEachValType(D, [&](const ComponentValType &VT) noexcept {
    Max = std::max(Max, getTypeDepth({VT, E.Home, E.Remap}));
  });
  const uint64_t Depth = Max + 1;
  TypeDepths[Def] = Depth;
  return Depth;
}

// ---------------------------------------------------------------------------
// Copies of views under a remap; Copies maps each shape met to its copy.
// ---------------------------------------------------------------------------

// Copy a view across an instantiation boundary under the Node remap.
ExternInfo Context::copyExtern(
    const ExternInfo &E, const ResourceMap *Node,
    std::unordered_map<const Shape *, const Shape *> &Copies) noexcept {
  ExternInfo R = E;
  switch (E.Kind) {
  case ExternKind::CoreType:
    break;
  case ExternKind::FuncType:
    R.Func.Remap = composeRemap(Node, E.Func.Remap);
    break;
  case ExternKind::ValueBound:
    R.Value.Remap = composeRemap(Node, E.Value.Remap);
    break;
  case ExternKind::TypeBound:
    R.Type = copyTypeEntry(E.Type, Node, Copies);
    break;
  case ExternKind::InstanceType:
  case ExternKind::ComponentType:
    R.Shape = copyShape(E.Shape, Node, Copies);
    break;
  }
  return R;
}

TypeEntry Context::copyTypeEntry(
    const TypeEntry &E, const ResourceMap *Node,
    std::unordered_map<const Shape *, const Shape *> &Copies) noexcept {
  TypeEntry R = E;
  if (E.isResource()) {
    R.ResourceId = Node->apply(*E.ResourceId);
    if (*R.ResourceId != *E.ResourceId) {
      R.NameId = getResource(*R.ResourceId).NameId;
    }
  }
  R.Remap = composeRemap(Node, E.Remap);
  if (E.Inst != nullptr) {
    R.Inst = copyShape(E.Inst, Node, Copies);
  }
  if (E.Comp != nullptr) {
    R.Comp = copyShape(E.Comp, Node, Copies);
  }
  return R;
}

// One walk for both shapes; each loop is a no-op for the other kind.
const Shape *Context::copyShape(
    const Shape *S, const ResourceMap *Node,
    std::unordered_map<const Shape *, const Shape *> &Copies) noexcept {
  if (S == nullptr) {
    return nullptr;
  }
  auto It = Copies.find(S);
  if (It != Copies.end()) {
    return It->second;
  }
  auto *R = addShape();
  Copies.emplace(S, R);
  R->DeclScope = S->DeclScope;
  R->ExportOrder = S->ExportOrder;
  for (const auto &[Name, E] : S->Imports) {
    R->Imports.emplace_back(Name, copyExtern(E, Node, Copies));
  }
  for (const auto &[Name, E] : S->Exports) {
    R->Exports.emplace(Name, copyExtern(E, Node, Copies));
  }
  return R;
}

} // namespace Component
} // namespace Validator
} // namespace WasmEdge
