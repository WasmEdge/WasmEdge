// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- component_context.cpp - Component state ---------------------------===//
//
// The bodies of Context.
//
//===----------------------------------------------------------------------===//
#include "validator/component_context.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace WasmEdge {
namespace Validator {
namespace Component {

using namespace std::literals;

// ---------------------------------------------------------------------------
// Index-space registration and resolution.
// ---------------------------------------------------------------------------

void Context::defineExtern(const ExternInfo &Info) noexcept {
  auto &S = top();
  switch (Info.K) {
  case ExternKind::CoreType:
    S.CoreModules.push_back(Info.CoreMod);
    break;
  case ExternKind::FuncType:
    S.Funcs.push_back(Info.Func);
    break;
  case ExternKind::ValueBound:
    S.Values.push_back({Info.Value, false});
    break;
  case ExternKind::TypeBound:
    S.Types.push_back(Info.Type);
    break;
  case ExternKind::InstanceType:
    S.Instances.push_back(Info.Shape);
    break;
  case ExternKind::ComponentType:
    S.Components.push_back(Info.Shape);
    break;
  }
}

Expect<ExternInfo>
Context::resolveSortIndex(const AST::Component::SortIndex &SI) noexcept {
  ExternInfo Info;
  const auto &S = top();
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
      Info.K = ExternKind::CoreType;
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
    Info.K = ExternKind::FuncType;
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
    Info.K = ExternKind::ValueBound;
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
    Info.K = ExternKind::TypeBound;
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
    Info.K = ExternKind::ComponentType;
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
    Info.K = ExternKind::InstanceType;
    Info.Shape = I;
    return Info;
  }
  default:
    spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
    return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
  }
}

// External view of an inline core module: its imports in order and the
// resolved types of its exports.
Expect<const CoreShape *>
Context::buildCoreShape(const AST::Module &Mod) noexcept {
  auto *Info = Types.newCoreShape();
  const auto &ModTypes = Mod.getTypeSection().getContent();

  auto TypeAt = [&ModTypes](uint32_t Idx) noexcept -> const AST::SubType * {
    return Idx < ModTypes.size() ? &ModTypes[Idx] : nullptr;
  };

  // Core index spaces: imports first, then definitions.
  std::vector<CoreExternInfo> Funcs, Tables, Memories, Globals, Tags;
  for (const auto &Imp : Mod.getImportSection().getContent()) {
    CoreExternInfo Ext;
    switch (Imp.getExternalType()) {
    case ExternalType::Function:
      Ext = {ExternalType::Function, TypeAt(Imp.getExternalFuncTypeIdx()),
             nullptr, nullptr, nullptr};
      Funcs.push_back(Ext);
      break;
    case ExternalType::Table:
      Ext = {ExternalType::Table, nullptr, &Imp.getExternalTableType(), nullptr,
             nullptr};
      Tables.push_back(Ext);
      break;
    case ExternalType::Memory:
      Ext = {ExternalType::Memory, nullptr, nullptr,
             &Imp.getExternalMemoryType(), nullptr};
      Memories.push_back(Ext);
      break;
    case ExternalType::Global:
      Ext = {ExternalType::Global, nullptr, nullptr, nullptr,
             &Imp.getExternalGlobalType()};
      Globals.push_back(Ext);
      break;
    case ExternalType::Tag:
      Ext = {ExternalType::Tag, TypeAt(Imp.getExternalTagType().getTypeIdx()),
             nullptr, nullptr, nullptr};
      Tags.push_back(Ext);
      break;
    default:
      break;
    }
    for (const auto &[ModName, Name, Prev] : Info->Imports) {
      if (ModName == Imp.getModuleName() && Name == Imp.getExternalName()) {
        spdlog::error(ErrCode::Value::ComponentDuplicateImportName);
        spdlog::error("    Module import '{}'.'{}' name conflict."sv, ModName,
                      Name);
        return Unexpect(ErrCode::Value::ComponentDuplicateImportName);
      }
    }
    Info->Imports.emplace_back(std::string(Imp.getModuleName()),
                               std::string(Imp.getExternalName()), Ext);
  }
  for (const auto TIdx : Mod.getFunctionSection().getContent()) {
    Funcs.push_back(
        {ExternalType::Function, TypeAt(TIdx), nullptr, nullptr, nullptr});
  }
  for (const auto &Seg : Mod.getTableSection().getContent()) {
    Tables.push_back(
        {ExternalType::Table, nullptr, &Seg.getTableType(), nullptr, nullptr});
  }
  for (const auto &MT : Mod.getMemorySection().getContent()) {
    Memories.push_back({ExternalType::Memory, nullptr, nullptr, &MT, nullptr});
  }
  for (const auto &Seg : Mod.getGlobalSection().getContent()) {
    Globals.push_back({ExternalType::Global, nullptr, nullptr, nullptr,
                       &Seg.getGlobalType()});
  }
  for (const auto &TT : Mod.getTagSection().getContent()) {
    Tags.push_back({ExternalType::Tag, TypeAt(TT.getTypeIdx()), nullptr,
                    nullptr, nullptr});
  }

  for (const auto &Exp : Mod.getExportSection().getContent()) {
    const uint32_t Idx = Exp.getExternalIndex();
    CoreExternInfo Ext;
    switch (Exp.getExternalType()) {
    case ExternalType::Function:
      if (Idx >= Funcs.size()) {
        continue;
      }
      Ext = Funcs[Idx];
      break;
    case ExternalType::Table:
      if (Idx >= Tables.size()) {
        continue;
      }
      Ext = Tables[Idx];
      break;
    case ExternalType::Memory:
      if (Idx >= Memories.size()) {
        continue;
      }
      Ext = Memories[Idx];
      break;
    case ExternalType::Global:
      if (Idx >= Globals.size()) {
        continue;
      }
      Ext = Globals[Idx];
      break;
    case ExternalType::Tag:
      if (Idx >= Tags.size()) {
        continue;
      }
      Ext = Tags[Idx];
      break;
    default:
      continue;
    }
    Info->Exports.emplace(std::string(Exp.getExternalName()), Ext);
  }
  return Info;
}

// ---------------------------------------------------------------------------
// Canonical options.
// ---------------------------------------------------------------------------

bool Context::hasOption(const AST::Component::Canonical &Canon,
                        ComponentCanonOptCode Code) noexcept {
  for (const auto &Opt : Canon.getOptions()) {
    if (Opt.getCode() == Code) {
      return true;
    }
  }
  return false;
}

bool Context::canonMemoryIs64(
    const AST::Component::Canonical &Canon) const noexcept {
  for (const auto &Opt : Canon.getOptions()) {
    if (Opt.getCode() == ComponentCanonOptCode::Memory) {
      const uint32_t Idx = Opt.getIndex();
      if (Idx < top().CoreMemories.size()) {
        const auto *Mem = top().CoreMemories[Idx];
        return Mem != nullptr && Mem->getLimit().is64();
      }
    }
  }
  return false;
}

Expect<void> Context::checkOptions(const AST::Component::Canonical &Canon,
                                   bool IsLift) const noexcept {
  const auto &S = top();
  const ValType I32V{TypeCode::I32};
  bool SeenEncoding = false, SeenMemory = false, SeenRealloc = false,
       SeenPostReturn = false, SeenAsync = false, SeenCallback = false;
  // The pointer width of realloc follows the selected memory.
  const ValType Ptr = canonPtrType(Canon);
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
      // realloc must have type [ptr ptr ptr ptr] -> [ptr], where ptr is the
      // index type of the selected memory.
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

// ---------------------------------------------------------------------------
// Name grammar and strong uniqueness.
// ---------------------------------------------------------------------------

Expect<ExternName> Context::parseExternName(std::string_view Name,
                                            bool IsImport) const noexcept {
  const auto Position = IsImport ? "Import"sv : "Export"sv;
  // `relative-url=` is not part of the extern-name grammar.
  if (Name.rfind("relative-url="sv, 0) == 0) {
    spdlog::error(ErrCode::Value::InvalidExternName);
    spdlog::error("    {} name '{}' is not a valid extern name."sv, Position,
                  Name);
    return Unexpect(ErrCode::Value::InvalidExternName);
  }
  ExternName CN;
  EXPECTED_TRY(CN.parse(Name));
  if (CN.getKind() == ExternName::Kind::Invalid) {
    spdlog::error(ErrCode::Value::InvalidExternName);
    spdlog::error("    {} name '{}' is not a valid extern name."sv, Position,
                  Name);
    return Unexpect(ErrCode::Value::InvalidExternName);
  }
  // Dep / url / hash names are import-only.
  if (!IsImport && (CN.getKind() == ExternName::Kind::LockedDep ||
                    CN.getKind() == ExternName::Kind::UnlockedDep ||
                    CN.getKind() == ExternName::Kind::Url ||
                    CN.getKind() == ExternName::Kind::Integrity)) {
    spdlog::error(ErrCode::Value::InvalidExportName);
    spdlog::error("    Export name '{}' kind is not valid for exports."sv,
                  Name);
    return Unexpect(ErrCode::Value::InvalidExportName);
  }
  return CN;
}

NameRecord Context::makeNameRecord(const ExternName &Name) const noexcept {
  NameRecord R;
  R.Original = std::string(Name.getOriginalName());
  switch (Name.getKind()) {
  case ExternName::Kind::Constructor:
    R.HasAnnotation = true;
    R.IsConstructor = true;
    R.StrippedExact = std::string(Name.getNoTagName());
    break;
  case ExternName::Kind::Method:
  case ExternName::Kind::Static: {
    R.HasAnnotation = true;
    R.StrippedExact = std::string(Name.getNoTagName());
    auto Dot = R.StrippedExact.find('.');
    if (Dot != std::string::npos) {
      R.DottedFirst = R.StrippedExact.substr(0, Dot);
      R.IsDottedSame = (R.DottedFirst == R.StrippedExact.substr(Dot + 1));
    }
    break;
  }
  case ExternName::Kind::Label:
    R.IsPlainLabel = true;
    R.StrippedExact = std::string(Name.getOriginalName());
    break;
  default:
    R.StrippedExact = std::string(Name.getOriginalName());
    break;
  }
  // Case-folding models the acronym rule, which applies only to labels and
  // interface names. Dep, url, and integrity names compare exactly.
  switch (Name.getKind()) {
  case ExternName::Kind::LockedDep:
  case ExternName::Kind::UnlockedDep:
  case ExternName::Kind::Url:
  case ExternName::Kind::Integrity:
    R.Stripped = R.StrippedExact;
    break;
  default:
    R.Stripped = R.StrippedExact;
    std::transform(
        R.Stripped.begin(), R.Stripped.end(), R.Stripped.begin(),
        [](unsigned char C) { return static_cast<char>(std::tolower(C)); });
    break;
  }
  return R;
}

Expect<void> Context::addUniqueName(std::vector<NameRecord> &Names,
                                    const NameRecord &N,
                                    bool IsImport) const noexcept {
  // A strong-uniqueness violation inside a declarator carries a side-specific
  // code. An exact duplicate is a plain name conflict everywhere.
  const auto SideCode = IsImport ? ErrCode::Value::ComponentImportNameConflict
                                 : ErrCode::Value::ComponentExportNameConflict;
  const auto ConflictCode =
      top().K == Scope::Kind::Component
          ? SideCode
          : (IsImport ? ErrCode::Value::ComponentDeclImportNameConflict
                      : ErrCode::Value::ComponentDeclExportNameConflict);
  auto reportClash = [&](ErrCode::Value Code) noexcept {
    spdlog::error(Code);
    spdlog::error("    {} name '{}' is not strongly-unique."sv,
                  IsImport ? "Import"sv : "Export"sv, N.Original);
    return Unexpect(Code);
  };
  for (const auto &E : Names) {
    if (E.Original == N.Original) {
      return reportClash(SideCode);
    }
    if (E.Stripped == N.Stripped) {
      // `l` and `[constructor]l` (for the *same* label) are the one
      // strongly-unique annotated pair.
      const bool CtorException = ((E.IsConstructor && N.IsPlainLabel) ||
                                  (N.IsConstructor && E.IsPlainLabel)) &&
                                 E.StrippedExact == N.StrippedExact;
      if (!CtorException) {
        return reportClash(ConflictCode);
      }
      continue;
    }
    // `l` clashes with `[method]l.l` / `[static]l.l` for the same label.
    if ((N.IsPlainLabel && E.IsDottedSame &&
         E.DottedFirst == N.StrippedExact) ||
        (E.IsPlainLabel && N.IsDottedSame &&
         N.DottedFirst == E.StrippedExact)) {
      return reportClash(ConflictCode);
    }
  }
  Names.push_back(N);
  return {};
}

Expect<void> Context::checkNameAttributes(const ExternName &CN,
                                          Span<const std::string> Impls,
                                          Span<const std::string> ExtIds,
                                          Span<const std::string> VSuffixes,
                                          bool IsInstance) const noexcept {
  if (Impls.size() > 1) {
    spdlog::error(ErrCode::Value::ComponentImplementsDuplicate);
    spdlog::error("    name `{}` has more than one `implements`"sv,
                  CN.getOriginalName());
    return Unexpect(ErrCode::Value::ComponentImplementsDuplicate);
  }
  if (ExtIds.size() > 1) {
    spdlog::error(ErrCode::Value::ComponentExternalIdDuplicate);
    spdlog::error("    name `{}` has more than one `external-id`"sv,
                  CN.getOriginalName());
    return Unexpect(ErrCode::Value::ComponentExternalIdDuplicate);
  }
  if (VSuffixes.size() > 1) {
    spdlog::error(ErrCode::Value::ComponentVersionSuffixDuplicate);
    spdlog::error("    name `{}` has more than one `versionsuffix`"sv,
                  CN.getOriginalName());
    return Unexpect(ErrCode::Value::ComponentVersionSuffixDuplicate);
  }
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

// ---------------------------------------------------------------------------
// Declaring an import or an export.
// ---------------------------------------------------------------------------

Expect<ExternInfo>
Context::defineImport(std::string_view Name, const ExternInfo &Resolved,
                      Span<const std::string> Impls,
                      Span<const std::string> ExtIds,
                      Span<const std::string> VSuffixes) noexcept {
  // Each instance import mints fresh identities for the resources the
  // instance type itself declares (two imports of one shape differ).
  ExternInfo Info = Resolved;
  if (Info.K == ExternKind::InstanceType) {
    Info.Shape = freshenDeclaredResources(Info.Shape, true);
  }
  EXPECTED_TRY(ExternName CN, parseExternName(Name, true));
  EXPECTED_TRY(checkNameAttributes(CN, Impls, ExtIds, VSuffixes,
                                   Info.K == ExternKind::InstanceType));
  EXPECTED_TRY(addUniqueName(top().ImportSide.Names, makeNameRecord(CN), true));
  defineExtern(Info);
  EXPECTED_TRY(checkNamedTypesRule(Info, true));
  EXPECTED_TRY(checkAnnotatedName(CN, Info, true));
  recordResourceLabel(CN, Info, true);
  return Info;
}

Expect<ExternName>
Context::registerExportName(std::string_view Name, bool IsInstance,
                            Span<const std::string> Impls,
                            Span<const std::string> ExtIds,
                            Span<const std::string> VSuffixes) noexcept {
  EXPECTED_TRY(ExternName CN, parseExternName(Name, false));
  EXPECTED_TRY(checkNameAttributes(CN, Impls, ExtIds, VSuffixes, IsInstance));
  EXPECTED_TRY(
      addUniqueName(top().ExportSide.Names, makeNameRecord(CN), false));
  return CN;
}

Expect<ExternInfo>
Context::defineExport(const ExternName &CN, const ExternInfo &Inferred,
                      const std::optional<ExternInfo> &Ascribed) noexcept {
  ExternInfo Result = Inferred;
  if (Ascribed.has_value()) {
    Matcher M(Types);
    if (!M.matchExtern(Inferred, *Ascribed)) {
      spdlog::error(ErrCode::Value::ExportAscriptionIncompatible);
      spdlog::error(
          "    Ascribed type of export '{}' is not compatible with the "
          "exported definition."sv,
          CN.getOriginalName());
      return Unexpect(ErrCode::Value::ExportAscriptionIncompatible);
    }
    Result = *Ascribed;
  }
  // An export of a type re-introduces it under a fresh naming identity. The
  // underlying resource or structural identity stays, for matching.
  if (Result.K == ExternKind::TypeBound) {
    Result.Type.NameId = Types.newNameId();
  }
  if (Result.K == ExternKind::InstanceType) {
    Result.Shape = freshenDeclaredResources(Result.Shape, false);
  }
  defineExtern(Result);
  EXPECTED_TRY(checkNamedTypesRule(Result, false));
  EXPECTED_TRY(checkAnnotatedName(CN, Result, false));
  recordResourceLabel(CN, Result, false);
  return Result;
}

// ---------------------------------------------------------------------------
// Annotated plainnames.
// ---------------------------------------------------------------------------

Expect<void> Context::checkAnnotatedName(const ExternName &Name,
                                         const ExternInfo &Info,
                                         bool IsImport) noexcept {
  const auto Kind = Name.getKind();
  if (Kind != ExternName::Kind::Constructor &&
      Kind != ExternName::Kind::Method && Kind != ExternName::Kind::Static) {
    return {};
  }
  if (Info.K != ExternKind::FuncType || Info.Func.FT == nullptr) {
    spdlog::error(ErrCode::Value::ComponentIsNotFunc);
    spdlog::error(
        "    Annotated name '{}' is only allowed on function imports/exports."sv,
        Name.getOriginalName());
    return Unexpect(ErrCode::Value::ComponentIsNotFunc);
  }
  const std::string_view ResourceLabel = Name.getDetail().Resource;
  auto &S = top();
  const auto &Labels = S.nameSide(IsImport).ResourceLabels;
  const auto &Names = S.nameSide(IsImport).ResourceNames;
  const auto &FT = *Info.Func.FT;
  // The resource reached through the signature must carry a name on this side
  // which matches the annotation's first label.
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

  // Resolve a valtype to an own or borrow of a resource. Id filters when the
  // caller sets it.
  auto HandleOf = [this](const QualValType &Q,
                         bool WantOwn) noexcept -> std::optional<uint32_t> {
    TypeEntry Storage;
    const auto *Entry = Types.resolveQualType(Q, Storage);
    if (Entry == nullptr || Entry->DT == nullptr ||
        !Entry->DT->isDefValType()) {
      return std::nullopt;
    }
    const auto &DVT = Entry->DT->getDefValType();
    uint32_t HandleIdx = 0;
    if (WantOwn && DVT.isOwnTy()) {
      HandleIdx = DVT.getOwn().Idx;
    } else if (!WantOwn && DVT.isBorrowTy()) {
      HandleIdx = DVT.getBorrow().Idx;
    } else {
      return std::nullopt;
    }
    const auto *Res = Entry->Home->getType(HandleIdx);
    if (Res == nullptr || !Res->ResourceId.has_value()) {
      return std::nullopt;
    }
    return Types.applyRemap(Entry->Remap, *Res->ResourceId);
  };

  if (Kind == ExternName::Kind::Constructor) {
    // Signature first: exactly one result of (own T) or (result (own T) e?).
    if (FT.getResultList().size() != 1) {
      spdlog::error(ErrCode::Value::AnnotatedCtorReturnOne);
      spdlog::error("    Constructor '{}' should return one value."sv,
                    Name.getOriginalName());
      return Unexpect(ErrCode::Value::AnnotatedCtorReturnOne);
    }
    QualValType Q{FT.getResultList()[0].getValType(), Info.Func.Home,
                  Info.Func.Remap};
    auto Target = HandleOf(Q, true);
    if (!Target.has_value()) {
      // Unwrap (result (own T) e?).
      TypeEntry Storage;
      const auto *Entry = Types.resolveQualType(Q, Storage);
      if (Entry != nullptr && Entry->DT != nullptr &&
          Entry->DT->isDefValType() &&
          Entry->DT->getDefValType().isResultTy()) {
        const auto &Res = Entry->DT->getDefValType().getResult();
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
    const auto &Self = FT.getParamList()[0];
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

void Context::recordResourceLabel(const ExternName &Name,
                                  const ExternInfo &Info,
                                  bool IsImport) noexcept {
  if (Name.getKind() == ExternName::Kind::Label &&
      Info.K == ExternKind::TypeBound && Info.Type.ResourceId.has_value()) {
    auto &S = top();
    auto &Labels = S.nameSide(IsImport).ResourceLabels;
    auto &Names = S.nameSide(IsImport).ResourceNames;
    Labels.emplace(std::string(Name.getOriginalName()), *Info.Type.ResourceId);
    Names.emplace(*Info.Type.ResourceId, std::string(Name.getOriginalName()));
  }
}

// ---------------------------------------------------------------------------
// The named-types rule. Flags, enums, records, variants, and resources are
// never anonymous. Tuples, lists, options, and results can be.
// ---------------------------------------------------------------------------

bool Context::isValTypeIntroduced(const QualValType &Q,
                                  bool IsImport) noexcept {
  if (Q.VT.isPrimValType() || Q.Home == nullptr) {
    return true;
  }
  TypeEntry Storage;
  const auto *Entry = Types.resolveQualType(Q, Storage);
  if (Entry == nullptr) {
    return true;
  }
  return isTypeEntryIntroduced(*Entry, IsImport);
}

bool Context::isTypeEntryIntroduced(const TypeEntry &E,
                                    bool IsImport) noexcept {
  auto &S = top();
  const auto &NamedTys = S.nameSide(IsImport).NamedTypes;
  const auto &NamedRes = S.nameSide(IsImport).NamedResources;
  if (E.ResourceId.has_value()) {
    return E.NameId.has_value() && NamedRes.count(*E.NameId) != 0;
  }
  if (E.DT == nullptr || !E.DT->isDefValType()) {
    return true;
  }
  const auto &D = E.DT->getDefValType();
  if (D.isPrimValType()) {
    return true;
  }
  // A local reference must name the introduced identity. A foreign entry
  // accepts a structurally equal named type.
  if (D.isFlagsTy() || D.isEnumTy() || D.isRecordTy() || D.isVariantTy()) {
    const auto &NamedIds = S.nameSide(IsImport).NamedIds;
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
        Matcher M(Types);
        if (M.matchNormalVal(Types.normalizeEntry(E),
                             Types.normalizeEntry(Probe))) {
          return true;
        }
      }
    }
    return false;
  }
  auto Sub = [&](const ComponentValType &VT) noexcept {
    return isValTypeIntroduced({VT, E.Home, E.Remap}, IsImport);
  };
  if (D.isTupleTy()) {
    for (const auto &Ty : D.getTuple().Types) {
      if (!Sub(Ty)) {
        return false;
      }
    }
    return true;
  }
  if (D.isListTy()) {
    return Sub(D.getList().ValTy);
  }
  if (D.isMapTy()) {
    return Sub(D.getMap().KeyTy) && Sub(D.getMap().ValTy);
  }
  if (D.isOptionTy()) {
    return Sub(D.getOption().ValTy);
  }
  if (D.isResultTy()) {
    const auto &R = D.getResult();
    return (!R.ValTy.has_value() || Sub(*R.ValTy)) &&
           (!R.ErrTy.has_value() || Sub(*R.ErrTy));
  }
  if (D.isStreamTy() || D.isFutureTy()) {
    const auto &Elem =
        D.isStreamTy() ? D.getStream().ValTy : D.getFuture().ValTy;
    return !Elem.has_value() || Sub(*Elem);
  }
  if (D.isOwnTy() || D.isBorrowTy()) {
    const uint32_t Idx = D.isOwnTy() ? D.getOwn().Idx : D.getBorrow().Idx;
    const auto *Res = E.Home->getType(Idx);
    if (Res == nullptr || !Res->ResourceId.has_value()) {
      return true;
    }
    const uint32_t Eff = Types.applyRemap(E.Remap, *Res->ResourceId);
    const uint32_t NameId =
        Eff != *Res->ResourceId
            ? Types.getResource(Eff).NameId
            : Res->NameId.value_or(Types.getResource(Eff).NameId);
    return NamedRes.count(NameId) != 0;
  }
  return true;
}

// Checks the type being introduced itself: its immediate components must be
// named, while the type itself is exempt (the extern names it).
bool Context::areInnerTypesIntroduced(const TypeEntry &E,
                                      bool IsImport) noexcept {
  if (E.ResourceId.has_value()) {
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
  if (E.DT == nullptr) {
    return true;
  }
  if (E.DT->isFuncType()) {
    const auto &FT = E.DT->getFuncType();
    for (const auto &P : FT.getParamList()) {
      if (!isValTypeIntroduced({P.getValType(), E.Home, E.Remap}, IsImport)) {
        return false;
      }
    }
    for (const auto &R : FT.getResultList()) {
      if (!isValTypeIntroduced({R.getValType(), E.Home, E.Remap}, IsImport)) {
        return false;
      }
    }
    return true;
  }
  if (!E.DT->isDefValType()) {
    return true;
  }
  const auto &D = E.DT->getDefValType();
  if (D.isPrimValType() || D.isFlagsTy() || D.isEnumTy()) {
    return true;
  }
  auto Sub = [&](const ComponentValType &VT) noexcept {
    return isValTypeIntroduced({VT, E.Home, E.Remap}, IsImport);
  };
  if (D.isRecordTy()) {
    for (const auto &LT : D.getRecord().LabelTypes) {
      if (!Sub(LT.getValType())) {
        return false;
      }
    }
    return true;
  }
  if (D.isVariantTy()) {
    for (const auto &[Label, Ty] : D.getVariant().Cases) {
      if (Ty.has_value() && !Sub(*Ty)) {
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
    return Sub(D.getList().ValTy);
  }
  if (D.isMapTy()) {
    return Sub(D.getMap().KeyTy) && Sub(D.getMap().ValTy);
  }
  if (D.isOptionTy()) {
    return Sub(D.getOption().ValTy);
  }
  if (D.isResultTy()) {
    const auto &R = D.getResult();
    return (!R.ValTy.has_value() || Sub(*R.ValTy)) &&
           (!R.ErrTy.has_value() || Sub(*R.ErrTy));
  }
  if (D.isStreamTy() || D.isFutureTy()) {
    const auto &Elem =
        D.isStreamTy() ? D.getStream().ValTy : D.getFuture().ValTy;
    return !Elem.has_value() || Sub(*Elem);
  }
  if (D.isOwnTy() || D.isBorrowTy()) {
    const auto &NamedRes = top().nameSide(IsImport).NamedResources;
    const uint32_t Idx = D.isOwnTy() ? D.getOwn().Idx : D.getBorrow().Idx;
    const auto *Res = E.Home->getType(Idx);
    if (Res == nullptr || !Res->ResourceId.has_value()) {
      return true;
    }
    const uint32_t Eff = Types.applyRemap(E.Remap, *Res->ResourceId);
    const uint32_t NameId =
        Eff != *Res->ResourceId
            ? Types.getResource(Eff).NameId
            : Res->NameId.value_or(Types.getResource(Eff).NameId);
    return NamedRes.count(NameId) != 0;
  }
  return true;
}

// Validate and register an extern for the named-types rule. An instance
// extern recurses and introduces its exports in declaration order.
bool Context::introduceExternTypes(const ExternInfo &Info,
                                   bool IsImport) noexcept {
  auto &S = top();
  switch (Info.K) {
  case ExternKind::CoreType:
  case ExternKind::ComponentType:
    return true;
  case ExternKind::TypeBound: {
    if (!areInnerTypesIntroduced(Info.Type, IsImport)) {
      return false;
    }
    // Introduce: imported types are usable by exports as well.
    if (Info.Type.ResourceId.has_value() && Info.Type.NameId.has_value()) {
      if (IsImport) {
        S.ImportSide.NamedResources.insert(*Info.Type.NameId);
        S.ExportSide.NamedResources.insert(*Info.Type.NameId);
      } else {
        S.ExportSide.NamedResources.insert(*Info.Type.NameId);
      }
    } else if (Info.Type.DT != nullptr && Info.Type.DT->isDefValType()) {
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
      if (!isValTypeIntroduced(
              {P.getValType(), Info.Func.Home, Info.Func.Remap}, IsImport)) {
        return false;
      }
    }
    for (const auto &R : Info.Func.FT->getResultList()) {
      if (!isValTypeIntroduced(
              {R.getValType(), Info.Func.Home, Info.Func.Remap}, IsImport)) {
        return false;
      }
    }
    return true;
  }
  case ExternKind::ValueBound:
    return isValTypeIntroduced(Info.Value, IsImport);
  }
  return true;
}

Expect<void> Context::checkNamedTypesRule(const ExternInfo &Info,
                                          bool IsImport) noexcept {
  // Instance-type declarations do not enforce the named-types rule. An
  // import or an export checks their shapes again.
  if (top().K == Scope::Kind::InstanceType) {
    return {};
  }
  if (introduceExternTypes(Info, IsImport)) {
    return {};
  }
  ErrCode::Value Code;
  switch (Info.K) {
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
// Instantiation.
// ---------------------------------------------------------------------------

Expect<const Shape *> Context::instantiateComponentShape(
    const Shape &CI,
    Span<const AST::Component::InstantiateArg<AST::Component::SortIndex>>
        Args) noexcept {
  // Resolve the arguments. The names must be unique.
  std::unordered_map<std::string_view, ExternInfo> ArgMap;
  for (const auto &Arg : Args) {
    EXPECTED_TRY(auto Info, resolveSortIndex(Arg.getIndex()));
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
      auto &VE = top().Values[Arg.getIndex().getIdx()];
      if (VE.Consumed) {
        spdlog::error(ErrCode::Value::ComponentValueAlreadyConsumed);
        return Unexpect(ErrCode::Value::ComponentValueAlreadyConsumed);
      }
      VE.Consumed = true;
    }
  }
  // Match every import. The matcher accumulates the resource substitution.
  Matcher M(Types);
  for (const auto &[Name, Req] : CI.Imports) {
    auto It = ArgMap.find(Name);
    if (It == ArgMap.end()) {
      spdlog::error(ErrCode::Value::ComponentMissingImport);
      spdlog::error("    Missing instantiation argument '{}'."sv, Name);
      return Unexpect(ErrCode::Value::ComponentMissingImport);
    }
    if (!M.matchExtern(It->second, Req)) {
      const auto Code = M.getFailCode() != ErrCode::Value::Success
                            ? M.getFailCode()
                            : ErrCode::Value::ArgTypeMismatch;
      spdlog::error(Code);
      spdlog::error("    Instantiation argument '{}' has an incompatible "
                    "type."sv,
                    Name);
      return Unexpect(Code);
    }
  }

  // Combined remap: substituted imports + freshened defined resources.
  std::unordered_set<uint32_t> Reachable;
  for (const auto &[Name, E] : CI.Exports) {
    Types.collectResources(E, Reachable);
  }
  auto *Node = Types.newResourceMap();
  Node->Map = M.getSubst().Map;
  for (const uint32_t Id : Reachable) {
    if (Node->Map.count(Id) != 0) {
      continue;
    }
    const auto &Entry = Types.getResource(Id);
    if (!Entry.FromImport && CI.DeclScope != nullptr &&
        Types.originatesIn(Id, *CI.DeclScope)) {
      // Freshened ids carry no definition body: the fresh resource belongs to
      // the created instance, so it is never canon-local here.
      Node->Map.emplace(Id, Types.addResource(nullptr, &top(), false));
    }
  }

  const auto *Result = Types.rebuildInstanceExports(CI, Node);
  ExternInfo Probe;
  Probe.K = ExternKind::InstanceType;
  Probe.Shape = Result;
  EXPECTED_TRY(Types.checkTypeLimits(Probe));
  return Result;
}

} // namespace Component
} // namespace Validator
} // namespace WasmEdge
