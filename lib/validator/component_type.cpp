// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- component_type.cpp - Type / descriptor / declarator validation ----===//
//
// Validation of component-model type definitions, extern descriptors, and the
// declaration bodies of moduletype / instancetype / componenttype, producing
// the resolved info views defined in validator/component_context.h.
//
//===----------------------------------------------------------------------===//

#include "common/errinfo.h"
#include "common/spdlog.h"
#include "validator/validator.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_set>
#include <vector>

namespace WasmEdge {
namespace Validator {

using namespace std::literals;

namespace {

std::string toLowerCopy(std::string_view SV) {
  std::string R(SV);
  std::transform(R.begin(), R.end(), R.begin(), [](unsigned char C) {
    return static_cast<char>(std::tolower(C));
  });
  return R;
}

// Label validity + case-insensitive uniqueness within one type definition.
Expect<void> checkLabel(std::string_view Label,
                        std::unordered_set<std::string> &Seen,
                        std::string_view Where,
                        ErrCode::Value DupCode) noexcept {
  if (Label.empty()) {
    spdlog::error(ErrCode::Value::NameCannotBeEmpty);
    spdlog::error("    {} label cannot be empty."sv, Where);
    return Unexpect(ErrCode::Value::NameCannotBeEmpty);
  }
  if (!isKebabString(Label)) {
    spdlog::error(ErrCode::Value::ComponentNameNotKebab);
    spdlog::error("    {} label '{}' is not in kebab case."sv, Where, Label);
    return Unexpect(ErrCode::Value::ComponentNameNotKebab);
  }
  if (!Seen.insert(toLowerCopy(Label)).second) {
    spdlog::error(DupCode);
    spdlog::error("    {} label '{}' conflicts with a previous label."sv, Where,
                  Label);
    return Unexpect(DupCode);
  }
  return {};
}

} // namespace

// valtype ::= i:<typeidx> | pvt:<primvaltype>. A type index must refer to a
// defvaltype entry in the current scope.
Expect<void> Validator::validate(const ComponentValType &VT) noexcept {
  if (VT.isPrimValType()) {
    return {};
  }
  const auto *Entry = CompCtx.top().getType(VT.getTypeIndex());
  if (Entry == nullptr) {
    spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
    spdlog::error("    Value type index {} out of bounds (size {})."sv,
                  VT.getTypeIndex(), CompCtx.top().Types.size());
    return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
  }
  if (Entry->DT == nullptr || !Entry->DT->isDefValType()) {
    spdlog::error(ErrCode::Value::NotADefinedType);
    spdlog::error("    Value type index {} does not refer to a value type."sv,
                  VT.getTypeIndex());
    return Unexpect(ErrCode::Value::NotADefinedType);
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::DefValType &DVT) noexcept {
  std::unordered_set<std::string> Seen;
  if (DVT.isPrimValType()) {
    return validate(
        ComponentValType(static_cast<ComponentTypeCode>(DVT.getPrimValType())));
  }
  if (DVT.isRecordTy()) {
    const auto &Rec = DVT.getRecord();
    if (Rec.LabelTypes.empty()) {
      spdlog::error(ErrCode::Value::RecordMustHaveField);
      spdlog::error("    Record type must have at least one field."sv);
      return Unexpect(ErrCode::Value::RecordMustHaveField);
    }
    for (const auto &LT : Rec.LabelTypes) {
      EXPECTED_TRY(checkLabel(LT.getLabel(), Seen, "Record field"sv,
                              ErrCode::Value::RecordFieldNameConflicts));
      EXPECTED_TRY(validate(LT.getValType()));
    }
    return {};
  }
  if (DVT.isVariantTy()) {
    const auto &Var = DVT.getVariant();
    if (Var.Cases.empty()) {
      spdlog::error(ErrCode::Value::VariantMustHaveCase);
      spdlog::error("    Variant type must have at least one case."sv);
      return Unexpect(ErrCode::Value::VariantMustHaveCase);
    }
    for (const auto &[Label, Ty] : Var.Cases) {
      EXPECTED_TRY(checkLabel(Label, Seen, "Variant case"sv,
                              ErrCode::Value::VariantCaseNameConflicts));
      if (Ty.has_value()) {
        EXPECTED_TRY(validate(*Ty));
      }
    }
    return {};
  }
  if (DVT.isListTy()) {
    const auto &List = DVT.getList();
    if (List.Len.has_value() && *List.Len == 0) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    Fixed-length list must have a non-zero length."sv);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    return validate(List.ValTy);
  }
  if (DVT.isTupleTy()) {
    const auto &Tup = DVT.getTuple();
    if (Tup.Types.empty()) {
      spdlog::error(ErrCode::Value::TupleMustHaveType);
      spdlog::error("    Tuple type must have at least one element."sv);
      return Unexpect(ErrCode::Value::TupleMustHaveType);
    }
    for (const auto &Ty : Tup.Types) {
      EXPECTED_TRY(validate(Ty));
    }
    return {};
  }
  if (DVT.isFlagsTy()) {
    const auto &Flags = DVT.getFlags();
    if (Flags.Labels.empty()) {
      spdlog::error(ErrCode::Value::FlagsMustHaveEntry);
      spdlog::error("    Flags type must have at least one label."sv);
      return Unexpect(ErrCode::Value::FlagsMustHaveEntry);
    }
    if (Flags.Labels.size() > 32) {
      spdlog::error(ErrCode::Value::CannotHaveMoreThan32Flags);
      spdlog::error("    Flags type has {} labels."sv, Flags.Labels.size());
      return Unexpect(ErrCode::Value::CannotHaveMoreThan32Flags);
    }
    for (const auto &Label : Flags.Labels) {
      EXPECTED_TRY(checkLabel(Label, Seen, "Flags"sv,
                              ErrCode::Value::FlagNameConflicts));
    }
    return {};
  }
  if (DVT.isEnumTy()) {
    const auto &Enum = DVT.getEnum();
    if (Enum.Labels.empty()) {
      spdlog::error(ErrCode::Value::EnumMustHaveVariant);
      spdlog::error("    Enum type must have at least one label."sv);
      return Unexpect(ErrCode::Value::EnumMustHaveVariant);
    }
    for (const auto &Label : Enum.Labels) {
      EXPECTED_TRY(checkLabel(Label, Seen, "Enum"sv,
                              ErrCode::Value::EnumTagNameConflicts));
    }
    return {};
  }
  if (DVT.isOptionTy()) {
    return validate(DVT.getOption().ValTy);
  }
  if (DVT.isResultTy()) {
    const auto &Res = DVT.getResult();
    if (Res.ValTy.has_value()) {
      EXPECTED_TRY(validate(*Res.ValTy));
    }
    if (Res.ErrTy.has_value()) {
      EXPECTED_TRY(validate(*Res.ErrTy));
    }
    return {};
  }
  if (DVT.isOwnTy() || DVT.isBorrowTy()) {
    const uint32_t Idx = DVT.isOwnTy() ? DVT.getOwn().Idx : DVT.getBorrow().Idx;
    const auto *Entry = CompCtx.top().getType(Idx);
    if (Entry == nullptr) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    own/borrow type index {} out of bounds (size {})."sv,
                    Idx, CompCtx.top().Types.size());
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    if (!Entry->ResourceId.has_value()) {
      spdlog::error(ErrCode::Value::ComponentNotResourceType);
      spdlog::error(
          "    own/borrow type index {} does not refer to a resource type."sv,
          Idx);
      return Unexpect(ErrCode::Value::ComponentNotResourceType);
    }
    return {};
  }
  if (DVT.isStreamTy()) {
    const auto &S = DVT.getStream();
    if (S.ValTy.has_value()) {
      if (S.ValTy->isPrimValType() &&
          S.ValTy->getCode() == ComponentTypeCode::Char) {
        // Temporary spec limitation (component-model PR #607).
        spdlog::error(ErrCode::Value::ComponentStreamCharInvalid);
        spdlog::error("    The stream element type cannot be `char`."sv);
        return Unexpect(ErrCode::Value::ComponentStreamCharInvalid);
      }
      return validate(*S.ValTy);
    }
    return {};
  }
  if (DVT.isFutureTy()) {
    const auto &F = DVT.getFuture();
    if (F.ValTy.has_value()) {
      return validate(*F.ValTy);
    }
    return {};
  }
  spdlog::error(ErrCode::Value::ComponentNotImplValidator);
  spdlog::error("    This defined value type is not supported yet."sv);
  return Unexpect(ErrCode::Value::ComponentNotImplValidator);
}

Expect<void> Validator::validate(const AST::Component::FuncType &FT) noexcept {
  std::unordered_set<std::string> Seen;
  for (const auto &Param : FT.getParamList()) {
    if (Param.getLabel().empty()) {
      spdlog::error(ErrCode::Value::NameCannotBeEmpty);
      spdlog::error("    Function parameter name cannot be empty."sv);
      return Unexpect(ErrCode::Value::NameCannotBeEmpty);
    }
    EXPECTED_TRY(checkLabel(Param.getLabel(), Seen, "Function parameter"sv,
                            ErrCode::Value::FuncParamNameConflict));
    EXPECTED_TRY(validate(Param.getValType()));
  }
  if (FT.getResultList().size() > 1) {
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error("    Function types may have at most one result."sv);
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  for (const auto &Result : FT.getResultList()) {
    if (!Result.getLabel().empty()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    Function results cannot be named."sv);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    EXPECTED_TRY(validate(Result.getValType()));
    if (containsBorrow({Result.getValType(), &CompCtx.top(), nullptr})) {
      spdlog::error(ErrCode::Value::FuncResultContainsBorrow);
      spdlog::error("    Function results cannot contain borrow handles."sv);
      return Unexpect(ErrCode::Value::FuncResultContainsBorrow);
    }
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ResourceType &RT) noexcept {
  if (CompCtx.top().K != ComponentContext::Scope::Kind::Component) {
    spdlog::error(ErrCode::Value::ComponentResourceOutsideComponent);
    spdlog::error(
        "    Resource types cannot be defined in component or instance types."sv);
    return Unexpect(ErrCode::Value::ComponentResourceOutsideComponent);
  }
  if (RT.getCallback().has_value()) {
    spdlog::error(ErrCode::Value::ComponentNotImplValidator);
    spdlog::error("    async resource destructors are not supported yet."sv);
    return Unexpect(ErrCode::Value::ComponentNotImplValidator);
  }
  if (RT.isAddrI64()) {
    spdlog::error(ErrCode::Value::ComponentResourceRepI32);
    spdlog::error("    Resources can only be represented by i32."sv);
    return Unexpect(ErrCode::Value::ComponentResourceRepI32);
  }
  if (RT.getDestructor().has_value()) {
    const uint32_t Idx = *RT.getDestructor();
    const auto *Dtor = CompCtx.top().getCoreFunc(Idx);
    if (Dtor == nullptr) {
      spdlog::error(ErrCode::Value::ComponentFunctionIndexOutOfBounds);
      spdlog::error("    Destructor core function index {} out of bounds."sv,
                    Idx);
      return Unexpect(ErrCode::Value::ComponentFunctionIndexOutOfBounds);
    }
    // The destructor must have type [rep] -> [].
    const ValType Rep =
        RT.isAddrI64() ? ValType(TypeCode::I64) : ValType(TypeCode::I32);
    const auto &CT = Dtor->getCompositeType();
    if (!CT.isFunc() || CT.getFuncType().getParamTypes().size() != 1 ||
        CT.getFuncType().getParamTypes()[0] != Rep ||
        !CT.getFuncType().getReturnTypes().empty()) {
      spdlog::error(ErrCode::Value::ComponentDtorSignature);
      spdlog::error("    Resource destructor must have type [{}] -> []."sv,
                    RT.isAddrI64() ? "i64"sv : "i32"sv);
      return Unexpect(ErrCode::Value::ComponentDtorSignature);
    }
  }
  return {};
}

// core:deftype ::= rectype | moduletype. Rectypes push one core:type entry
// per subtype; moduletypes are validated in their own scope.
Expect<void>
Validator::validate(const AST::Component::CoreDefType &DType) noexcept {
  if (DType.isRecType()) {
    // Depth memoization for the core subtype hierarchy checks; resized on
    // demand by the core validator.
    std::vector<uint32_t> SubTypeDepthMap;
    for (const auto &ST : DType.getSubTypes()) {
      // Concrete heap-type references resolve against the component's
      // core:type space, which the core FormChecker cannot see. Check them
      // here and only run the structural core validation when unreferenced.
      bool HasTypeRefs = false;
      auto CheckRefs =
          [this, &HasTypeRefs](Span<const ValType> Types) -> Expect<void> {
        for (const auto &VT : Types) {
          if (VT.isRefType() && !VT.isAbsHeapType()) {
            HasTypeRefs = true;
            const auto *Entry = CompCtx.top().getCoreType(VT.getTypeIndex());
            if (Entry == nullptr) {
              spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
              spdlog::error("    Core type index {} out of bounds."sv,
                            VT.getTypeIndex());
              return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
            }
            if (Entry->Mod != nullptr) {
              spdlog::error(ErrCode::Value::ComponentUnknownModule);
              spdlog::error("    Core type index {} refers to a module type."sv,
                            VT.getTypeIndex());
              return Unexpect(ErrCode::Value::ComponentUnknownModule);
            }
          }
        }
        return {};
      };
      if (ST.getCompositeType().isFunc()) {
        EXPECTED_TRY(
            CheckRefs(ST.getCompositeType().getFuncType().getParamTypes()));
        EXPECTED_TRY(
            CheckRefs(ST.getCompositeType().getFuncType().getReturnTypes()));
      }
      if (!HasTypeRefs) {
        EXPECTED_TRY(validate(ST,
                              static_cast<uint32_t>(Checker.getTypes().size()),
                              SubTypeDepthMap));
      }
      CompCtx.top().CoreTypes.push_back({&ST, nullptr});
    }
    return {};
  }
  EXPECTED_TRY(const auto *Info, validateModuleType(DType.getModuleType()));
  CompCtx.top().CoreTypes.push_back({nullptr, Info});
  return {};
}

Expect<void>
Validator::validate(const AST::Component::DefType &DType) noexcept {
  if (DType.isDefValType()) {
    EXPECTED_TRY(validate(DType.getDefValType()));
    auto &S = CompCtx.top();
    S.Types.push_back(
        {&DType, &S, nullptr, nullptr, nullptr, {}, CompCtx.newNameId()});
  } else if (DType.isFuncType()) {
    EXPECTED_TRY(validate(DType.getFuncType()));
    auto &S = CompCtx.top();
    S.Types.push_back({&DType, &S, nullptr, nullptr, nullptr, {}, {}});
  } else if (DType.isResourceType()) {
    EXPECTED_TRY(validate(DType.getResourceType()));
    auto &S = CompCtx.top();
    const uint32_t Id =
        CompCtx.addResource(&DType.getResourceType(), &S, false);
    S.Types.push_back({&DType, &S, nullptr, nullptr, nullptr, Id,
                       CompCtx.getResource(Id).NameId});
  } else if (DType.isInstanceType()) {
    EXPECTED_TRY(const auto *Info,
                 validateInstanceType(DType.getInstanceType()));
    auto &S = CompCtx.top();
    S.Types.push_back({&DType, &S, nullptr, Info, nullptr, {}, {}});
  } else if (DType.isComponentType()) {
    EXPECTED_TRY(const auto *Info,
                 validateComponentType(DType.getComponentType()));
    auto &S = CompCtx.top();
    S.Types.push_back({&DType, &S, nullptr, nullptr, Info, {}, {}});
  }
  // Effective type-size limit on the freshly defined entry.
  {
    auto &S = CompCtx.top();
    CtxView::ExternInfo Probe;
    Probe.K = CtxView::ExternInfo::Kind::Type;
    Probe.Type = S.Types.back();
    EXPECTED_TRY(checkTypeSize(sizeOfExtern(Probe)));
    EXPECTED_TRY(checkTypeDepth(depthOfExtern(Probe)));
  }
  return {};
}

// core:importdesc inside moduletype declarations; func/tag type indices
// resolve in the moduletype's own core type space.
Expect<ComponentContext::CoreExternInfo>
Validator::validate(const AST::Component::CoreImportDesc &Desc) noexcept {
  ComponentContext::CoreExternInfo Info;
  if (Desc.isFunc()) {
    const auto *Entry = CompCtx.top().getCoreType(Desc.getTypeIndex());
    if (Entry == nullptr) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    Core type index {} out of bounds (size {})."sv,
                    Desc.getTypeIndex(), CompCtx.top().CoreTypes.size());
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    if (Entry->Func == nullptr || !Entry->Func->getCompositeType().isFunc()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    Core type index {} is not a function type."sv,
                    Desc.getTypeIndex());
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    Info.Kind = ExternalType::Function;
    Info.Func = Entry->Func;
  } else if (Desc.isTable()) {
    EXPECTED_TRY(validate(Desc.getTableType()));
    Info.Kind = ExternalType::Table;
    Info.Table = &Desc.getTableType();
  } else if (Desc.isMemory()) {
    EXPECTED_TRY(validate(Desc.getMemoryType()));
    Info.Kind = ExternalType::Memory;
    Info.Memory = &Desc.getMemoryType();
  } else if (Desc.isGlobal()) {
    EXPECTED_TRY(validate(Desc.getGlobalType()));
    Info.Kind = ExternalType::Global;
    Info.Global = &Desc.getGlobalType();
  } else if (Desc.isTag()) {
    const uint32_t Idx = Desc.getTagType().getTypeIdx();
    const auto *Entry = CompCtx.top().getCoreType(Idx);
    if (Entry == nullptr) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    Tag type index {} out of bounds."sv, Idx);
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    if (Entry->Func == nullptr || !Entry->Func->getCompositeType().isFunc() ||
        !Entry->Func->getCompositeType()
             .getFuncType()
             .getReturnTypes()
             .empty()) {
      spdlog::error(ErrCode::Value::InvalidTagResultType);
      spdlog::error("    Tag types must be function types without results."sv);
      return Unexpect(ErrCode::Value::InvalidTagResultType);
    }
    Info.Kind = ExternalType::Tag;
    Info.Func = Entry->Func;
  }
  return Info;
}

// moduletype ::= 0x50 md*:vec(<core:moduledecl>). Runs in a ModuleType scope
// whose core type space is local to the declaration body.
Expect<const ComponentContext::CoreModuleInfo *> Validator::validateModuleType(
    Span<const AST::Component::CoreModuleDecl> Decls) noexcept {
  ComponentContext::ScopedScope Guard(
      CompCtx, ComponentContext::Scope::Kind::ModuleType);
  auto *Info = CompCtx.newCoreModuleInfo();
  for (const auto &Decl : Decls) {
    if (Decl.isImport()) {
      const auto &Imp = Decl.getImport();
      EXPECTED_TRY(auto Ext, validate(Imp.getImportDesc()));
      for (const auto &[ModName, Name, Prev] : Info->Imports) {
        if (ModName == Imp.getModuleName() && Name == Imp.getName()) {
          spdlog::error(ErrCode::Value::ComponentDuplicateImportName);
          spdlog::error("    Module type import '{}'.'{}' name conflict."sv,
                        ModName, Name);
          return Unexpect(ErrCode::Value::ComponentDuplicateImportName);
        }
      }
      Info->Imports.emplace_back(std::string(Imp.getModuleName()),
                                 std::string(Imp.getName()), Ext);
    } else if (Decl.isType()) {
      const auto *T = Decl.getType();
      if (T == nullptr) {
        spdlog::error(ErrCode::Value::InvalidTypeReference);
        return Unexpect(ErrCode::Value::InvalidTypeReference);
      }
      // MVP: module types cannot define nested module types.
      if (T->isModuleType()) {
        spdlog::error(ErrCode::Value::InvalidTypeReference);
        spdlog::error("    Module types cannot define nested module types."sv);
        return Unexpect(ErrCode::Value::InvalidTypeReference);
      }
      EXPECTED_TRY(validate(*T));
    } else if (Decl.isAlias()) {
      EXPECTED_TRY(validate(Decl.getAlias()));
    } else if (Decl.isExport()) {
      const auto &Exp = Decl.getExport();
      EXPECTED_TRY(auto Ext, validate(Exp.getImportDesc()));
      if (!Info->Exports.emplace(std::string(Exp.getName()), Ext).second) {
        spdlog::error(ErrCode::Value::DupExportName);
        spdlog::error("    Module type export '{}' name conflict."sv,
                      Exp.getName());
        return Unexpect(ErrCode::Value::DupExportName);
      }
    }
  }
  return Info;
}

// core:alias inside moduletype declarations: only outer aliases of the core
// type sort are expressible; MVP additionally rejects aliasing module types.
Expect<void>
Validator::validate(const AST::Component::CoreAlias &Alias) noexcept {
  if (Alias.getSort().isCore() &&
      Alias.getSort().getCoreSortType() ==
          AST::Component::Sort::CoreSortType::Type) {
    const auto *Target = CompCtx.scopeUp(Alias.getComponentJump());
    if (Target == nullptr) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error("    Outer alias count {} exceeds enclosing scopes."sv,
                    Alias.getComponentJump());
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
    const auto *Entry = Target->getCoreType(Alias.getIndex());
    if (Entry == nullptr) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    Aliased core type index {} out of bounds."sv,
                    Alias.getIndex());
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    if (Entry->Mod != nullptr) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    Module types cannot be aliased into module types."sv);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    CompCtx.top().CoreTypes.push_back(*Entry);
    return {};
  }
  spdlog::error(ErrCode::Value::MalformedAliasTarget);
  spdlog::error("    Core aliases can only target the core type sort."sv);
  return Unexpect(ErrCode::Value::MalformedAliasTarget);
}

Expect<const ComponentContext::InstanceInfo *> Validator::validateInstanceType(
    const AST::Component::InstanceType &IT) noexcept {
  ComponentContext::ScopedScope Guard(
      CompCtx, ComponentContext::Scope::Kind::InstanceType);
  auto *Info = CompCtx.newInstanceInfo();
  Info->DeclScope = &Guard.get();
  for (const auto &Decl : IT.getDecl()) {
    const auto Before = Info->Exports.size();
    EXPECTED_TRY(validate(Decl, Info->Exports));
    if (Info->Exports.size() != Before && Decl.isExportDecl()) {
      Info->Order.emplace_back(Decl.getExport().getName());
    }
  }
  return Info;
}

Expect<const ComponentContext::ComponentInfo *>
Validator::validateComponentType(
    const AST::Component::ComponentType &CT) noexcept {
  ComponentContext::ScopedScope Guard(
      CompCtx, ComponentContext::Scope::Kind::ComponentType);
  auto *Info = CompCtx.newComponentInfo();
  Info->DeclScope = &Guard.get();
  Info->FromDecl = true;
  for (const auto &Decl : CT.getDecl()) {
    EXPECTED_TRY(validate(Decl, *Info));
  }
  return Info;
}

Expect<void>
Validator::validate(const AST::Component::InstanceDecl &Decl,
                    ComponentContext::ExternMap &Exports) noexcept {
  if (Decl.isCoreType()) {
    const auto *T = Decl.getCoreType();
    if (T == nullptr) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    return validate(*T);
  }
  if (Decl.isType()) {
    const auto *T = Decl.getType();
    if (T == nullptr) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    return validate(*T);
  }
  if (Decl.isAlias()) {
    return validate(Decl.getAlias());
  }
  if (Decl.isExportDecl()) {
    const auto &ED = Decl.getExport();
    // The descriptor is checked before the export name.
    EXPECTED_TRY(auto Info, validate(ED.getExternDesc(), false));
    if (Info.K == ComponentContext::ExternInfo::Kind::Instance) {
      Info.Inst = freshenDeclaredResources(Info.Inst, false);
    }
    EXPECTED_TRY(ComponentName CN, parseExportName(ED.getName()));
    const auto DeclClash = ComponentContext::addUniqueName(
        CompCtx.top().ExportNames, ComponentContext::makeNameRecord(CN));
    if (DeclClash != ComponentContext::NameClash::None) {
      const auto Code = DeclClash == ComponentContext::NameClash::Duplicate
                            ? ErrCode::Value::ComponentNameConflict
                            : ErrCode::Value::ComponentDeclExportNameConflict;
      spdlog::error(Code);
      spdlog::error("    Export name '{}' is not strongly-unique."sv,
                    ED.getName());
      return Unexpect(Code);
    }
    defineExtern(Info);
    EXPECTED_TRY(checkResourceNameability(Info, false));
    EXPECTED_TRY(checkAnnotatedName(CN, Info, false));
    EXPECTED_TRY(checkImplements(
        CN, ED.getImplements(),
        Info.K == ComponentContext::ExternInfo::Kind::Instance));
    recordResourceLabel(CN, Info, false);
    Exports.emplace(std::string(ED.getName()), Info);
    return {};
  }
  spdlog::error(ErrCode::Value::InvalidTypeReference);
  return Unexpect(ErrCode::Value::InvalidTypeReference);
}

Expect<void>
Validator::validate(const AST::Component::ComponentDecl &Decl,
                    ComponentContext::ComponentInfo &Info) noexcept {
  if (Decl.isImportDecl()) {
    const auto &ID = Decl.getImport();
    EXPECTED_TRY(auto Ext, defineImport(ID.getName(), ID.getExternDesc(),
                                        ID.getImplements()));
    Info.Imports.emplace_back(std::string(ID.getName()), Ext);
    return {};
  }
  return validate(Decl.getInstance(), Info.Exports);
}

// externdesc resolution: bounds/kind checks plus entity typing. Sub-resource
// type bounds allocate a fresh abstract id in the current scope.
Expect<ComponentContext::ExternInfo>
Validator::validate(const AST::Component::ExternDesc &Desc,
                    bool ImportSide) noexcept {
  using DescType = AST::Component::ExternDesc::DescType;
  ComponentContext::ExternInfo Info;
  auto &S = CompCtx.top();
  switch (Desc.getDescType()) {
  case DescType::CoreType: {
    const auto *Entry = S.getCoreType(Desc.getTypeIndex());
    if (Entry == nullptr) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    Core type index {} out of bounds."sv,
                    Desc.getTypeIndex());
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    if (Entry->Mod == nullptr) {
      const auto Code = ImportSide ? ErrCode::Value::ComponentUnknownModule
                                   : ErrCode::Value::ComponentNotModuleType;
      spdlog::error(Code);
      spdlog::error("    Core type index {} is not a module type."sv,
                    Desc.getTypeIndex());
      return Unexpect(Code);
    }
    Info.K = ComponentContext::ExternInfo::Kind::CoreModule;
    Info.CoreMod = Entry->Mod;
    return Info;
  }
  case DescType::FuncType: {
    const auto *Entry = S.getType(Desc.getTypeIndex());
    if (Entry == nullptr) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    Type index {} out of bounds (size {})."sv,
                    Desc.getTypeIndex(), S.Types.size());
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    if (Entry->DT == nullptr || !Entry->DT->isFuncType()) {
      const auto Code = ImportSide
                            ? ErrCode::Value::ComponentUnknownFunctionType
                            : ErrCode::Value::ComponentNotFunctionType;
      spdlog::error(Code);
      spdlog::error("    Type index {} is not a function type."sv,
                    Desc.getTypeIndex());
      return Unexpect(Code);
    }
    Info.K = ComponentContext::ExternInfo::Kind::Func;
    Info.Func = {&Entry->DT->getFuncType(), Entry->Home, Entry->Remap};
    return Info;
  }
  case DescType::ValueBound: {
    Info.K = ComponentContext::ExternInfo::Kind::Value;
    if (Desc.isEqType()) {
      const uint32_t Idx = Desc.getTypeIndex();
      if (Idx >= S.Values.size()) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error("    Value index {} out of bounds (size {})."sv, Idx,
                      S.Values.size());
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
      Info.Value = S.Values[Idx].Type;
      return Info;
    }
    EXPECTED_TRY(validate(Desc.getValType()));
    Info.Value = {Desc.getValType(), &S, nullptr};
    return Info;
  }
  case DescType::TypeBound: {
    Info.K = ComponentContext::ExternInfo::Kind::Type;
    if (Desc.isEqType()) {
      const auto *Entry = S.getType(Desc.getTypeIndex());
      if (Entry == nullptr) {
        spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
        spdlog::error("    Type index {} out of bounds (size {})."sv,
                      Desc.getTypeIndex(), S.Types.size());
        return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
      }
      Info.Type = *Entry;
      // The created index carries a fresh naming identity.
      Info.Type.NameId = CompCtx.newNameId();
      return Info;
    }
    // (sub resource): fresh abstract resource type.
    const uint32_t Id = CompCtx.addResource(nullptr, &S, ImportSide);
    Info.Type = {nullptr,
                 &S,
                 nullptr,
                 nullptr,
                 nullptr,
                 Id,
                 CompCtx.getResource(Id).NameId};
    return Info;
  }
  case DescType::ComponentType: {
    const auto *Entry = S.getType(Desc.getTypeIndex());
    if (Entry == nullptr) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    Type index {} out of bounds (size {})."sv,
                    Desc.getTypeIndex(), S.Types.size());
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    if (Entry->Comp == nullptr) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    Type index {} is not a component type."sv,
                    Desc.getTypeIndex());
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    Info.K = ComponentContext::ExternInfo::Kind::Component;
    Info.Comp = Entry->Comp;
    return Info;
  }
  case DescType::InstanceType: {
    const auto *Entry = S.getType(Desc.getTypeIndex());
    if (Entry == nullptr) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    Type index {} out of bounds (size {})."sv,
                    Desc.getTypeIndex(), S.Types.size());
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    if (Entry->Inst == nullptr) {
      const auto Code = ImportSide
                            ? ErrCode::Value::ComponentUnknownInstanceType
                            : ErrCode::Value::ComponentNotInstanceType;
      spdlog::error(Code);
      spdlog::error("    Type index {} is not an instance type."sv,
                    Desc.getTypeIndex());
      return Unexpect(Code);
    }
    Info.K = ComponentContext::ExternInfo::Kind::Instance;
    Info.Inst = Entry->Inst;
    return Info;
  }
  default:
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
}

void Validator::defineExtern(
    const ComponentContext::ExternInfo &Info) noexcept {
  auto &S = CompCtx.top();
  using Kind = ComponentContext::ExternInfo::Kind;
  switch (Info.K) {
  case Kind::CoreModule:
    S.CoreModules.push_back(Info.CoreMod);
    break;
  case Kind::Func:
    S.Funcs.push_back(Info.Func);
    break;
  case Kind::Value:
    S.Values.push_back({Info.Value, false});
    break;
  case Kind::Type:
    S.Types.push_back(Info.Type);
    break;
  case Kind::Instance:
    S.Instances.push_back(Info.Inst);
    break;
  case Kind::Component:
    S.Components.push_back(Info.Comp);
    break;
  }
}

Expect<ComponentName>
Validator::parseImportName(std::string_view Name) noexcept {
  // `relative-url=` is not part of the extern-name grammar.
  if (Name.rfind("relative-url="sv, 0) == 0) {
    spdlog::error(ErrCode::Value::InvalidExternName);
    spdlog::error("    Import name '{}' is not a valid extern name."sv, Name);
    return Unexpect(ErrCode::Value::InvalidExternName);
  }
  EXPECTED_TRY(ComponentName CN, ComponentName::parse(Name));
  if (CN.getKind() == ComponentNameKind::Invalid) {
    spdlog::error(ErrCode::Value::InvalidExternName);
    spdlog::error("    Import name '{}' is not a valid extern name."sv, Name);
    return Unexpect(ErrCode::Value::InvalidExternName);
  }
  return CN;
}

Expect<ComponentName>
Validator::parseExportName(std::string_view Name) noexcept {
  // `relative-url=` is not part of the extern-name grammar.
  if (Name.rfind("relative-url="sv, 0) == 0) {
    spdlog::error(ErrCode::Value::InvalidExternName);
    spdlog::error("    Export name '{}' is not a valid extern name."sv, Name);
    return Unexpect(ErrCode::Value::InvalidExternName);
  }
  EXPECTED_TRY(ComponentName CN, ComponentName::parse(Name));
  switch (CN.getKind()) {
  case ComponentNameKind::Label:
  case ComponentNameKind::Constructor:
  case ComponentNameKind::Method:
  case ComponentNameKind::Static:
  case ComponentNameKind::InterfaceType:
    return CN;
  case ComponentNameKind::Invalid:
    spdlog::error(ErrCode::Value::InvalidExternName);
    spdlog::error("    Export name '{}' is not a valid extern name."sv, Name);
    return Unexpect(ErrCode::Value::InvalidExternName);
  default:
    // Dep / url / hash names are import-only.
    spdlog::error(ErrCode::Value::InvalidExportName);
    spdlog::error("    Export name '{}' kind is not valid for exports."sv,
                  Name);
    return Unexpect(ErrCode::Value::InvalidExportName);
  }
}

Expect<void> Validator::checkImplements(const ComponentName &CN,
                                        Span<const std::string> Impls,
                                        bool IsInstance) noexcept {
  if (Impls.empty()) {
    return {};
  }
  for (const auto &I : Impls) {
    auto Parsed = parseImportName(I);
    if (!Parsed.has_value()) {
      spdlog::error(ErrCode::Value::ComponentImplementsName);
      spdlog::error("    `implements` value `{}` is not a valid name"sv, I);
      return Unexpect(ErrCode::Value::ComponentImplementsName);
    }
    if (Parsed->getKind() != ComponentNameKind::InterfaceType) {
      spdlog::error(ErrCode::Value::ComponentImplementsInterface);
      spdlog::error("    `implements` value `{}` must be an interface"sv, I);
      return Unexpect(ErrCode::Value::ComponentImplementsInterface);
    }
  }
  if (CN.getKind() != ComponentNameKind::Label &&
      CN.getKind() != ComponentNameKind::Constructor &&
      CN.getKind() != ComponentNameKind::Method &&
      CN.getKind() != ComponentNameKind::Static) {
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

Expect<ComponentContext::ExternInfo>
Validator::defineImport(std::string_view Name,
                        const AST::Component::ExternDesc &Desc,
                        Span<const std::string> Impls) noexcept {
  // The descriptor is checked before the import name.
  EXPECTED_TRY(auto Info, validate(Desc, true));
  // Each instance import mints fresh identities for the resources the
  // instance type itself declares (two imports of one shape differ).
  if (Info.K == ComponentContext::ExternInfo::Kind::Instance) {
    Info.Inst = freshenDeclaredResources(Info.Inst, true);
  }
  (void)0;
  EXPECTED_TRY(ComponentName CN, parseImportName(Name));
  EXPECTED_TRY(checkImplements(
      CN, Impls, Info.K == ComponentContext::ExternInfo::Kind::Instance));
  const auto Rec = ComponentContext::makeNameRecord(CN);
  const auto Clash =
      ComponentContext::addUniqueName(CompCtx.top().ImportNames, Rec);
  if (Clash != ComponentContext::NameClash::None) {
    ErrCode::Value Code;
    if (CompCtx.top().K != ComponentContext::Scope::Kind::Component) {
      Code = Clash == ComponentContext::NameClash::Duplicate
                 ? ErrCode::Value::ComponentNameConflict
                 : ErrCode::Value::ComponentDeclImportNameConflict;
    } else if (!Rec.IsPlainish) {
      Code = ErrCode::Value::ComponentNameConflict;
    } else {
      Code = ErrCode::Value::ComponentNameConflict;
    }
    spdlog::error(Code);
    spdlog::error("    Import name '{}' is not strongly-unique."sv, Name);
    return Unexpect(Code);
  }
  defineExtern(Info);
  EXPECTED_TRY(checkResourceNameability(Info, true));
  EXPECTED_TRY(checkAnnotatedName(CN, Info, true));
  recordResourceLabel(CN, Info, true);
  return Info;
}

Expect<ComponentContext::ExternInfo> Validator::defineExport(
    std::string_view Name, const ComponentContext::ExternInfo &Inferred,
    const std::optional<AST::Component::ExternDesc> &Ascribed,
    Span<const std::string> Impls) noexcept {
  EXPECTED_TRY(ComponentName CN, parseExportName(Name));
  EXPECTED_TRY(checkImplements(
      CN, Impls, Inferred.K == ComponentContext::ExternInfo::Kind::Instance));
  const auto Rec = ComponentContext::makeNameRecord(CN);
  const auto Clash =
      ComponentContext::addUniqueName(CompCtx.top().ExportNames, Rec);
  if (Clash != ComponentContext::NameClash::None) {
    ErrCode::Value Code;
    if (CompCtx.top().K != ComponentContext::Scope::Kind::Component) {
      Code = ErrCode::Value::ComponentDeclExportNameConflict;
    } else if (!Rec.IsPlainish ||
               Clash == ComponentContext::NameClash::Duplicate) {
      Code = ErrCode::Value::ComponentNameConflict;
    } else {
      Code = ErrCode::Value::ComponentNameConflict;
    }
    spdlog::error(Code);
    spdlog::error("    Export name '{}' is not strongly-unique."sv, Name);
    return Unexpect(Code);
  }
  ComponentContext::ExternInfo Result = Inferred;
  if (Ascribed.has_value()) {
    EXPECTED_TRY(auto Asc, validate(*Ascribed, false));
    ResourceSubst Subst;
    if (!matchExtern(Inferred, Asc, Subst)) {
      spdlog::error(ErrCode::Value::ExportAscriptionIncompatible);
      spdlog::error(
          "    Ascribed type of export '{}' is not compatible with the "
          "exported definition."sv,
          Name);
      return Unexpect(ErrCode::Value::ExportAscriptionIncompatible);
    }
    Result = Asc;
  }
  // Exporting a type re-introduces it under a fresh naming identity; the
  // underlying (resource or structural) identity is preserved for matching.
  if (Result.K == ComponentContext::ExternInfo::Kind::Type) {
    Result.Type.NameId = CompCtx.newNameId();
  }
  if (Result.K == ComponentContext::ExternInfo::Kind::Instance) {
    Result.Inst = freshenDeclaredResources(Result.Inst, false);
  }
  defineExtern(Result);
  EXPECTED_TRY(checkResourceNameability(Result, false));
  EXPECTED_TRY(checkAnnotatedName(CN, Result, false));
  recordResourceLabel(CN, Result, false);
  return Result;
}

// Annotated plainnames: [constructor]r / [method]r.m / [static]r.m are only
// valid on funcs whose signature agrees with a preceding resource r on the
// same (import or export) side.
Expect<void>
Validator::checkAnnotatedName(const ComponentName &Name,
                              const ComponentContext::ExternInfo &Info,
                              bool IsImport) noexcept {
  const auto Kind = Name.getKind();
  if (Kind != ComponentNameKind::Constructor &&
      Kind != ComponentNameKind::Method && Kind != ComponentNameKind::Static) {
    return {};
  }
  if (Info.K != ComponentContext::ExternInfo::Kind::Func ||
      Info.Func.FT == nullptr) {
    spdlog::error(ErrCode::Value::ComponentIsNotFunc);
    spdlog::error(
        "    Annotated name '{}' is only allowed on function imports/exports."sv,
        Name.getOriginalName());
    return Unexpect(ErrCode::Value::ComponentIsNotFunc);
  }
  std::string_view ResourceLabel;
  if (Kind == ComponentNameKind::Constructor) {
    ResourceLabel = Name.getDetail().get<ConstructorDetail>().Label;
  } else if (Kind == ComponentNameKind::Method) {
    ResourceLabel = Name.getDetail().get<MethodDetail>().Resource;
  } else {
    ResourceLabel = Name.getDetail().get<StaticDetail>().Resource;
  }
  auto &Scope = CompCtx.top();
  const auto &Labels =
      IsImport ? Scope.ImportResourceLabels : Scope.ExportResourceLabels;
  const auto &Names =
      IsImport ? Scope.ImportResourceNames : Scope.ExportResourceNames;
  const auto &FT = *Info.Func.FT;
  // The resource reached through the signature must carry a name on this
  // side which matches the annotation's first label.
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

  // Resolves a valtype to own/borrow of a resource; Id filters when set.
  auto HandleOf = [this](const ComponentContext::QualValType &Q,
                         bool WantOwn) noexcept -> std::optional<uint32_t> {
    ComponentContext::TypeEntry Storage;
    const auto *Entry = resolveQualType(Q, Storage);
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
    return ComponentContext::ResourceMap::apply(Entry->Remap, *Res->ResourceId);
  };

  if (Kind == ComponentNameKind::Constructor) {
    // Shape first: exactly one result of (own T) or (result (own T) e?).
    if (FT.getResultList().size() != 1) {
      spdlog::error(ErrCode::Value::AnnotatedCtorReturnOne);
      spdlog::error("    Constructor '{}' should return one value."sv,
                    Name.getOriginalName());
      return Unexpect(ErrCode::Value::AnnotatedCtorReturnOne);
    }
    ComponentContext::QualValType Q{FT.getResultList()[0].getValType(),
                                    Info.Func.Home, Info.Func.Remap};
    auto Target = HandleOf(Q, true);
    if (!Target.has_value()) {
      // Unwrap (result (own T) e?).
      ComponentContext::TypeEntry Storage;
      const auto *Entry = resolveQualType(Q, Storage);
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
  } else if (Kind == ComponentNameKind::Method) {
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

void Validator::recordResourceLabel(const ComponentName &Name,
                                    const ComponentContext::ExternInfo &Info,
                                    bool IsImport) noexcept {
  if (Name.getKind() == ComponentNameKind::Label &&
      Info.K == ComponentContext::ExternInfo::Kind::Type &&
      Info.Type.ResourceId.has_value()) {
    auto &S = CompCtx.top();
    auto &Labels = IsImport ? S.ImportResourceLabels : S.ExportResourceLabels;
    auto &Names = IsImport ? S.ImportResourceNames : S.ExportResourceNames;
    Labels.emplace(std::string(Name.getOriginalName()), *Info.Type.ResourceId);
    Names.emplace(*Info.Type.ResourceId, std::string(Name.getOriginalName()));
  }
}

// Resources referenced by a non-type extern must have been introduced by a
// preceding import (for imports) or export (for exports) on the same side.
// The named-types rule (mirrors the reference validator): flags, enums,
// records, variants, and resources are never anonymous — any reference to
// them from an imported/exported type must have been introduced by a
// preceding import (for imports) or import/export (for exports). Tuples,
// lists, options, and results may stay anonymous but their components are
// checked recursively. Instance externs introduce their exports in order.
// NOLINTBEGIN(misc-no-recursion)
bool Validator::namedValType(const ComponentContext::QualValType &Q,
                             bool IsImport) noexcept {
  if (Q.VT.isPrimValType() || Q.Home == nullptr) {
    return true;
  }
  ComponentContext::TypeEntry Storage;
  const auto *Entry = resolveQualType(Q, Storage);
  if (Entry == nullptr) {
    return true;
  }
  return namedTypeEntry(*Entry, IsImport);
}

bool Validator::namedTypeEntry(const ComponentContext::TypeEntry &E,
                               bool IsImport) noexcept {
  auto &S = CompCtx.top();
  const auto &NamedTys = IsImport ? S.ImportNamedTypes : S.ExportNamedTypes;
  const auto &NamedRes =
      IsImport ? S.ImportNamedResources : S.ExportNamedResources;
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
  // Never-anonymous shapes must be in the named set themselves. Local
  // references must name the introduced identity: a definition is not
  // "named" by merely being the target of an introduced import/export.
  // Foreign entries (from instantiated or declared shapes) allow a
  // structurally equal named type, since instantiation-time substitution
  // preserves structure.
  if (D.isFlagsTy() || D.isEnumTy() || D.isRecordTy() || D.isVariantTy()) {
    const auto &NamedIds = IsImport ? S.ImportNamedIds : S.ExportNamedIds;
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
        ComponentContext::TypeEntry Probe;
        Probe.DT = Named;
        Probe.Home = Home;
        ResourceSubst Scratch;
        if (matchNormalVal(normalizeEntry(E), normalizeEntry(Probe), Scratch)) {
          return true;
        }
      }
    }
    return false;
  }
  auto Sub = [&](const ComponentValType &VT) noexcept {
    return namedValType({VT, E.Home, E.Remap}, IsImport);
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
  if (D.isOptionTy()) {
    return Sub(D.getOption().ValTy);
  }
  if (D.isResultTy()) {
    const auto &R = D.getResult();
    return (!R.ValTy.has_value() || Sub(*R.ValTy)) &&
           (!R.ErrTy.has_value() || Sub(*R.ErrTy));
  }
  if (D.isOwnTy() || D.isBorrowTy()) {
    const uint32_t Idx = D.isOwnTy() ? D.getOwn().Idx : D.getBorrow().Idx;
    const auto *Res = E.Home->getType(Idx);
    if (Res == nullptr || !Res->ResourceId.has_value()) {
      return true;
    }
    const uint32_t Eff =
        ComponentContext::ResourceMap::apply(E.Remap, *Res->ResourceId);
    const uint32_t NameId =
        Eff != *Res->ResourceId
            ? CompCtx.getResource(Eff).NameId
            : Res->NameId.value_or(CompCtx.getResource(Eff).NameId);
    return NamedRes.count(NameId) != 0;
  }
  return true;
}

// Checks the type being introduced itself: its immediate components must be
// named, while the type itself is exempt (the extern names it).
bool Validator::allValTypesNamed(const ComponentContext::TypeEntry &E,
                                 bool IsImport) noexcept {
  if (E.ResourceId.has_value()) {
    return true;
  }
  if (E.Comp != nullptr) {
    return true;
  }
  if (E.Inst != nullptr) {
    for (const auto &[Name, Sub] : E.Inst->Exports) {
      if (!namedExtern(Sub, IsImport)) {
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
      if (!namedValType({P.getValType(), E.Home, E.Remap}, IsImport)) {
        return false;
      }
    }
    for (const auto &R : FT.getResultList()) {
      if (!namedValType({R.getValType(), E.Home, E.Remap}, IsImport)) {
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
    return namedValType({VT, E.Home, E.Remap}, IsImport);
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
  if (D.isOptionTy()) {
    return Sub(D.getOption().ValTy);
  }
  if (D.isResultTy()) {
    const auto &R = D.getResult();
    return (!R.ValTy.has_value() || Sub(*R.ValTy)) &&
           (!R.ErrTy.has_value() || Sub(*R.ErrTy));
  }
  if (D.isOwnTy() || D.isBorrowTy()) {
    const auto &NamedRes = IsImport ? CompCtx.top().ImportNamedResources
                                    : CompCtx.top().ExportNamedResources;
    const uint32_t Idx = D.isOwnTy() ? D.getOwn().Idx : D.getBorrow().Idx;
    const auto *Res = E.Home->getType(Idx);
    if (Res == nullptr || !Res->ResourceId.has_value()) {
      return true;
    }
    const uint32_t Eff =
        ComponentContext::ResourceMap::apply(E.Remap, *Res->ResourceId);
    const uint32_t NameId =
        Eff != *Res->ResourceId
            ? CompCtx.getResource(Eff).NameId
            : Res->NameId.value_or(CompCtx.getResource(Eff).NameId);
    return NamedRes.count(NameId) != 0;
  }
  return true;
}

// Validates and registers an extern for the named-types rule; instance
// externs recurse and introduce their exports in declaration order.
bool Validator::namedExtern(const ComponentContext::ExternInfo &Info,
                            bool IsImport) noexcept {
  using Kind = ComponentContext::ExternInfo::Kind;
  auto &S = CompCtx.top();
  switch (Info.K) {
  case Kind::CoreModule:
  case Kind::Component:
    return true;
  case Kind::Type: {
    if (!allValTypesNamed(Info.Type, IsImport)) {
      return false;
    }
    // Introduce: imported types are usable by exports as well.
    if (Info.Type.ResourceId.has_value() && Info.Type.NameId.has_value()) {
      if (IsImport) {
        S.ImportNamedResources.insert(*Info.Type.NameId);
        S.ExportNamedResources.insert(*Info.Type.NameId);
      } else {
        S.ExportNamedResources.insert(*Info.Type.NameId);
      }
    } else if (Info.Type.DT != nullptr && Info.Type.DT->isDefValType()) {
      if (IsImport) {
        S.ImportNamedTypes.emplace(Info.Type.DT, Info.Type.Home);
        S.ExportNamedTypes.emplace(Info.Type.DT, Info.Type.Home);
        if (Info.Type.NameId.has_value()) {
          S.ImportNamedIds.insert(*Info.Type.NameId);
          S.ExportNamedIds.insert(*Info.Type.NameId);
        }
      } else {
        S.ExportNamedTypes.emplace(Info.Type.DT, Info.Type.Home);
        if (Info.Type.NameId.has_value()) {
          S.ExportNamedIds.insert(*Info.Type.NameId);
        }
      }
    }
    return true;
  }
  case Kind::Instance: {
    if (Info.Inst == nullptr) {
      return true;
    }
    auto Walk = [&](const ComponentContext::ExternInfo &Sub) noexcept {
      return namedExtern(Sub, IsImport);
    };
    if (!Info.Inst->Order.empty()) {
      for (const auto &Name : Info.Inst->Order) {
        auto It = Info.Inst->Exports.find(Name);
        if (It != Info.Inst->Exports.end() && !Walk(It->second)) {
          return false;
        }
      }
      return true;
    }
    for (const auto &[Name, Sub] : Info.Inst->Exports) {
      if (!Walk(Sub)) {
        return false;
      }
    }
    return true;
  }
  case Kind::Func: {
    if (Info.Func.FT == nullptr) {
      return true;
    }
    for (const auto &P : Info.Func.FT->getParamList()) {
      if (!namedValType({P.getValType(), Info.Func.Home, Info.Func.Remap},
                        IsImport)) {
        return false;
      }
    }
    for (const auto &R : Info.Func.FT->getResultList()) {
      if (!namedValType({R.getValType(), Info.Func.Home, Info.Func.Remap},
                        IsImport)) {
        return false;
      }
    }
    return true;
  }
  case Kind::Value:
    return namedValType(Info.Value, IsImport);
  }
  return true;
}
// NOLINTEND(misc-no-recursion)

Expect<void>
Validator::checkResourceNameability(const ComponentContext::ExternInfo &Info,
                                    bool IsImport) noexcept {
  // Instance-type declarations do not enforce the named-types rule; their
  // shapes are re-checked when imported or exported.
  if (CompCtx.top().K == ComponentContext::Scope::Kind::InstanceType) {
    return {};
  }
  if (namedExtern(Info, IsImport)) {
    return {};
  }
  using Kind = ComponentContext::ExternInfo::Kind;
  ErrCode::Value Code;
  switch (Info.K) {
  case Kind::Func:
    Code = IsImport ? ErrCode::Value::ComponentFuncNotValidImport
                    : ErrCode::Value::ComponentFuncNotValidExport;
    break;
  case Kind::Instance:
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

// External view of an inline core module: its imports in order and the
// resolved types of its exports.
Expect<const ComponentContext::CoreModuleInfo *>
Validator::buildCoreModuleInfo(const AST::Module &Mod) noexcept {
  auto *Info = CompCtx.newCoreModuleInfo();
  const auto &Types = Mod.getTypeSection().getContent();

  auto TypeAt = [&Types](uint32_t Idx) noexcept -> const AST::SubType * {
    return Idx < Types.size() ? &Types[Idx] : nullptr;
  };

  // Core index spaces: imports first, then definitions.
  std::vector<ComponentContext::CoreExternInfo> Funcs, Tables, Memories,
      Globals, Tags;
  for (const auto &Imp : Mod.getImportSection().getContent()) {
    ComponentContext::CoreExternInfo Ext;
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
    ComponentContext::CoreExternInfo Ext;
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

} // namespace Validator
} // namespace WasmEdge
