// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- component_type.cpp - Type / descriptor / declarator validation ----===//
//
// Validation of type definitions, extern descriptors, and declaration bodies.
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

// Label validity + case-insensitive uniqueness within one type definition.
Expect<void> checkLabel(std::string_view Label,
                        std::unordered_set<std::string> &Seen,
                        std::string_view Where, ErrCode::Value EmptyCode,
                        ErrCode::Value DupCode) noexcept {
  if (Label.empty()) {
    spdlog::error(EmptyCode);
    spdlog::error("    {} label cannot be empty."sv, Where);
    return Unexpect(EmptyCode);
  }
  if (!Component::ExternName::isKebabString(Label)) {
    spdlog::error(ErrCode::Value::ComponentNameNotKebab);
    spdlog::error("    {} label '{}' is not in kebab case."sv, Where, Label);
    return Unexpect(ErrCode::Value::ComponentNameNotKebab);
  }
  std::string Lower(Label);
  std::transform(
      Lower.begin(), Lower.end(), Lower.begin(),
      [](unsigned char C) { return static_cast<char>(std::tolower(C)); });
  if (!Seen.insert(std::move(Lower)).second) {
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
    spdlog::error(ErrCode::Value::ComponentTypeIndexOutOfBounds);
    spdlog::error("    Value type index {} out of bounds (size {})."sv,
                  VT.getTypeIndex(), CompCtx.top().Types.size());
    return Unexpect(ErrCode::Value::ComponentTypeIndexOutOfBounds);
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
                              ErrCode::Value::RecordFieldNameEmpty,
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
                              ErrCode::Value::VariantCaseNameEmpty,
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
    if (List.Len.has_value() &&
        *List.Len > Component::TypeSystem::MaxFixedListElems) {
      spdlog::error(ErrCode::Value::ComponentFixedListTooLarge);
      spdlog::error("    Fixed-length list of {} elements exceeds the limit "
                    "of {}."sv,
                    *List.Len, Component::TypeSystem::MaxFixedListElems);
      return Unexpect(ErrCode::Value::ComponentFixedListTooLarge);
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
                              ErrCode::Value::FlagNameEmpty,
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
                              ErrCode::Value::EnumTagNameEmpty,
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
      spdlog::error(ErrCode::Value::ComponentTypeIndexOutOfBounds);
      spdlog::error("    own/borrow type index {} out of bounds (size {})."sv,
                    Idx, CompCtx.top().Types.size());
      return Unexpect(ErrCode::Value::ComponentTypeIndexOutOfBounds);
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
      EXPECTED_TRY(validate(*S.ValTy));
      // A borrow outlives no call once it is carried by a stream.
      if (CompTypes.containsBorrow({*S.ValTy, &CompCtx.top(), nullptr})) {
        spdlog::error(ErrCode::Value::ComponentStreamFutureBorrow);
        spdlog::error(
            "    The stream element type cannot contain a `borrow`."sv);
        return Unexpect(ErrCode::Value::ComponentStreamFutureBorrow);
      }
    }
    return {};
  }
  if (DVT.isFutureTy()) {
    const auto &F = DVT.getFuture();
    if (F.ValTy.has_value()) {
      EXPECTED_TRY(validate(*F.ValTy));
      if (CompTypes.containsBorrow({*F.ValTy, &CompCtx.top(), nullptr})) {
        spdlog::error(ErrCode::Value::ComponentStreamFutureBorrow);
        spdlog::error(
            "    The future element type cannot contain a `borrow`."sv);
        return Unexpect(ErrCode::Value::ComponentStreamFutureBorrow);
      }
    }
    return {};
  }
  if (DVT.isMapTy()) {
    // keytype ::= bool | s8 | u8 | s16 | u16 | s32 | u32 | s64 | u64 | char
    //           | string
    const auto &Map = DVT.getMap();
    switch (Map.KeyTy.getCode()) {
    case ComponentTypeCode::Bool:
    case ComponentTypeCode::S8:
    case ComponentTypeCode::U8:
    case ComponentTypeCode::S16:
    case ComponentTypeCode::U16:
    case ComponentTypeCode::S32:
    case ComponentTypeCode::U32:
    case ComponentTypeCode::S64:
    case ComponentTypeCode::U64:
    case ComponentTypeCode::Char:
    case ComponentTypeCode::String:
      break;
    default:
      spdlog::error(ErrCode::Value::ComponentMapKeyType);
      spdlog::error("    Map key type is not one of the key types."sv);
      return Unexpect(ErrCode::Value::ComponentMapKeyType);
    }
    EXPECTED_TRY(validate(Map.KeyTy));
    return validate(Map.ValTy);
  }
  spdlog::error(ErrCode::Value::ComponentNotImplValidator);
  spdlog::error("    This defined value type is not supported yet."sv);
  return Unexpect(ErrCode::Value::ComponentNotImplValidator);
}

Expect<void> Validator::validate(const AST::Component::FuncType &FT) noexcept {
  std::unordered_set<std::string> Seen;
  for (const auto &Param : FT.getParamList()) {
    if (Param.getLabel().empty()) {
      spdlog::error(ErrCode::Value::FuncParamNameEmpty);
      spdlog::error("    Function parameter name cannot be empty."sv);
      return Unexpect(ErrCode::Value::FuncParamNameEmpty);
    }
    EXPECTED_TRY(checkLabel(Param.getLabel(), Seen, "Function parameter"sv,
                            ErrCode::Value::FuncParamNameEmpty,
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
    if (CompTypes.containsBorrow(
            {Result.getValType(), &CompCtx.top(), nullptr})) {
      spdlog::error(ErrCode::Value::FuncResultContainsBorrow);
      spdlog::error("    Function results cannot contain borrow handles."sv);
      return Unexpect(ErrCode::Value::FuncResultContainsBorrow);
    }
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ResourceType &RT) noexcept {
  if (CompCtx.top().K != Component::Scope::Kind::Component) {
    spdlog::error(ErrCode::Value::ComponentResourceOutsideComponent);
    spdlog::error(
        "    Resource types cannot be defined in component or instance types."sv);
    return Unexpect(ErrCode::Value::ComponentResourceOutsideComponent);
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
    // Depth memoization for the core subtype hierarchy checks. The core
    // validator resizes it on demand.
    std::vector<uint32_t> SubTypeDepthMap;
    for (const auto &ST : DType.getSubTypes()) {
      // Concrete heap-type references resolve against the component
      // core:type space, which the core FormChecker cannot see.
      bool HasTypeRefs = false;
      auto CheckRefs =
          [this, &HasTypeRefs](Span<const ValType> Types) -> Expect<void> {
        for (const auto &VT : Types) {
          if (VT.isRefType() && !VT.isAbsHeapType()) {
            HasTypeRefs = true;
            const auto *Entry = CompCtx.top().getCoreType(VT.getTypeIndex());
            if (Entry == nullptr) {
              spdlog::error(ErrCode::Value::ComponentTypeIndexOutOfBounds);
              spdlog::error("    Core type index {} out of bounds."sv,
                            VT.getTypeIndex());
              return Unexpect(ErrCode::Value::ComponentTypeIndexOutOfBounds);
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
  auto *Shape = CompTypes.newCoreShape();
  EXPECTED_TRY(validate(DType.getModuleType(), *Shape));
  CompCtx.top().CoreTypes.push_back({nullptr, Shape});
  return {};
}

Expect<void>
Validator::validate(const AST::Component::DefType &DType) noexcept {
  if (DType.isDefValType()) {
    EXPECTED_TRY(validate(DType.getDefValType()));
    auto &S = CompCtx.top();
    S.Types.push_back(
        {&DType, &S, nullptr, nullptr, nullptr, {}, CompTypes.newNameId()});
  } else if (DType.isFuncType()) {
    EXPECTED_TRY(validate(DType.getFuncType()));
    auto &S = CompCtx.top();
    S.Types.push_back({&DType, &S, nullptr, nullptr, nullptr, {}, {}});
  } else if (DType.isResourceType()) {
    EXPECTED_TRY(validate(DType.getResourceType()));
    auto &S = CompCtx.top();
    const uint32_t Id =
        CompTypes.addResource(&DType.getResourceType(), &S, false);
    S.Types.push_back({&DType, &S, nullptr, nullptr, nullptr, Id,
                       CompTypes.getResource(Id).NameId});
  } else if (DType.isInstanceType()) {
    auto *Shape = CompTypes.newShape();
    EXPECTED_TRY(validate(DType.getInstanceType(), *Shape));
    auto &S = CompCtx.top();
    S.Types.push_back({&DType, &S, nullptr, Shape, nullptr, {}, {}});
  } else if (DType.isComponentType()) {
    auto *Shape = CompTypes.newShape();
    EXPECTED_TRY(validate(DType.getComponentType(), *Shape));
    auto &S = CompCtx.top();
    S.Types.push_back({&DType, &S, nullptr, nullptr, Shape, {}, {}});
  }
  // Effective type-size limit on the freshly defined entry.
  {
    auto &S = CompCtx.top();
    Component::ExternInfo Probe;
    Probe.K = Component::ExternKind::TypeBound;
    Probe.Type = S.Types.back();
    EXPECTED_TRY(CompTypes.checkTypeLimits(Probe));
  }
  return {};
}

// core:importdesc inside moduletype declarations. A func or tag type index
// resolves in the own core type space of the moduletype.
Expect<void> Validator::validate(const AST::Component::CoreImportDesc &Desc,
                                 Component::CoreExternInfo &Out) noexcept {
  if (Desc.isFunc()) {
    const auto *Entry = CompCtx.top().getCoreType(Desc.getTypeIndex());
    if (Entry == nullptr) {
      spdlog::error(ErrCode::Value::ComponentTypeIndexOutOfBounds);
      spdlog::error("    Core type index {} out of bounds (size {})."sv,
                    Desc.getTypeIndex(), CompCtx.top().CoreTypes.size());
      return Unexpect(ErrCode::Value::ComponentTypeIndexOutOfBounds);
    }
    if (Entry->Func == nullptr || !Entry->Func->getCompositeType().isFunc()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    Core type index {} is not a function type."sv,
                    Desc.getTypeIndex());
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    Out.Kind = ExternalType::Function;
    Out.Func = Entry->Func;
  } else if (Desc.isTable()) {
    EXPECTED_TRY(validate(Desc.getTableType()));
    Out.Kind = ExternalType::Table;
    Out.Table = &Desc.getTableType();
  } else if (Desc.isMemory()) {
    EXPECTED_TRY(validate(Desc.getMemoryType()));
    Out.Kind = ExternalType::Memory;
    Out.Memory = &Desc.getMemoryType();
  } else if (Desc.isGlobal()) {
    EXPECTED_TRY(validate(Desc.getGlobalType()));
    Out.Kind = ExternalType::Global;
    Out.Global = &Desc.getGlobalType();
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
    Out.Kind = ExternalType::Tag;
    Out.Func = Entry->Func;
  }
  return {};
}

// moduletype ::= 0x50 md*:vec(<core:moduledecl>). Runs in a ModuleType scope
// whose core type space is local to the declaration body.
Expect<void>
Validator::validate(Span<const AST::Component::CoreModuleDecl> Decls,
                    Component::CoreShape &Out) noexcept {
  CompCtx.enterScope(Component::Scope::Kind::ModuleType);
  for (const auto &Decl : Decls) {
    if (Decl.isImport()) {
      const auto &Imp = Decl.getImport();
      Component::CoreExternInfo Ext;
      EXPECTED_TRY(validate(Imp.getImportDesc(), Ext));
      for (const auto &[ModName, Name, Prev] : Out.Imports) {
        if (ModName == Imp.getModuleName() && Name == Imp.getName()) {
          spdlog::error(ErrCode::Value::ComponentDuplicateImportName);
          spdlog::error("    Module type import '{}'.'{}' name conflict."sv,
                        ModName, Name);
          return Unexpect(ErrCode::Value::ComponentDuplicateImportName);
        }
      }
      Out.Imports.emplace_back(std::string(Imp.getModuleName()),
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
      Component::CoreExternInfo Ext;
      EXPECTED_TRY(validate(Exp.getImportDesc(), Ext));
      if (!Out.Exports.emplace(std::string(Exp.getName()), Ext).second) {
        spdlog::error(ErrCode::Value::DupExportName);
        spdlog::error("    Module type export '{}' name conflict."sv,
                      Exp.getName());
        return Unexpect(ErrCode::Value::DupExportName);
      }
    }
  }
  CompCtx.exitScope();
  return {};
}

// core:alias inside moduletype declarations. The grammar fixes it to an outer
// alias of the core type sort. The MVP also rejects an alias of a module type.
Expect<void>
Validator::validate(const AST::Component::CoreAlias &Alias) noexcept {
  const auto *Target = CompCtx.scopeUp(Alias.getComponentJump());
  if (Target == nullptr) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error("    Outer alias count {} exceeds enclosing scopes."sv,
                  Alias.getComponentJump());
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  const auto *Entry = Target->getCoreType(Alias.getIndex());
  if (Entry == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTypeIndexOutOfBounds);
    spdlog::error("    Aliased core type index {} out of bounds."sv,
                  Alias.getIndex());
    return Unexpect(ErrCode::Value::ComponentTypeIndexOutOfBounds);
  }
  if (Entry->Mod != nullptr) {
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error("    Module types cannot be aliased into module types."sv);
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  CompCtx.top().CoreTypes.push_back(*Entry);
  return {};
}

Expect<void> Validator::validate(const AST::Component::InstanceType &IT,
                                 Component::Shape &Out) noexcept {
  auto &S = CompCtx.enterScope(Component::Scope::Kind::InstanceType);
  Out.DeclScope = &S;
  for (const auto &Decl : IT.getDecl()) {
    const auto Before = Out.Exports.size();
    EXPECTED_TRY(validate(Decl, Out.Exports));
    if (Out.Exports.size() != Before && Decl.isExportDecl()) {
      Out.ExportOrder.emplace_back(Decl.getExport().getName());
    }
  }
  CompCtx.exitScope();
  return {};
}

Expect<void> Validator::validate(const AST::Component::ComponentType &CT,
                                 Component::Shape &Out) noexcept {
  auto &S = CompCtx.enterScope(Component::Scope::Kind::ComponentType);
  Out.DeclScope = &S;
  for (const auto &Decl : CT.getDecl()) {
    EXPECTED_TRY(validate(Decl, Out));
  }
  CompCtx.exitScope();
  return {};
}

Expect<void> Validator::validate(const AST::Component::InstanceDecl &Decl,
                                 std::map<std::string, Component::ExternInfo,
                                          std::less<>> &Exports) noexcept {
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
    Component::ExternInfo Info;
    EXPECTED_TRY(validate(ED.getExternDesc(), false, Info));
    if (Info.K == Component::ExternKind::InstanceType) {
      Info.Shape = CompCtx.freshenDeclaredResources(Info.Shape, false);
    }
    EXPECTED_TRY(Component::ExternName CN,
                 CompCtx.parseExternName(ED.getName(), false));
    EXPECTED_TRY(CompCtx.addUniqueName(CompCtx.top().ExportSide.Names,
                                       CompCtx.makeNameRecord(CN), false));
    CompCtx.defineExtern(Info);
    EXPECTED_TRY(CompCtx.checkNamedTypesRule(Info, false));
    EXPECTED_TRY(CompCtx.checkAnnotatedName(CN, Info, false));
    EXPECTED_TRY(CompCtx.checkNameAttributes(
        CN, ED.getImplements(), ED.getExternalIds(), ED.getVersionSuffixes(),
        Info.K == Component::ExternKind::InstanceType));
    CompCtx.recordResourceLabel(CN, Info, false);
    Exports.emplace(std::string(ED.getName()), Info);
    return {};
  }
  spdlog::error(ErrCode::Value::InvalidTypeReference);
  return Unexpect(ErrCode::Value::InvalidTypeReference);
}

Expect<void> Validator::validate(const AST::Component::ComponentDecl &Decl,
                                 Component::Shape &Out) noexcept {
  if (Decl.isImportDecl()) {
    const auto &ID = Decl.getImport();
    Component::ExternInfo Resolved;
    EXPECTED_TRY(validate(ID.getExternDesc(), true, Resolved));
    EXPECTED_TRY(auto Ext, CompCtx.defineImport(
                               ID.getName(), Resolved, ID.getImplements(),
                               ID.getExternalIds(), ID.getVersionSuffixes()));
    Out.Imports.emplace_back(std::string(ID.getName()), Ext);
    return {};
  }
  return validate(Decl.getInstance(), Out.Exports);
}

// externdesc resolution: bounds/kind checks plus entity typing. Sub-resource
// type bounds allocate a fresh abstract id in the current scope.
Expect<void> Validator::validate(const AST::Component::ExternDesc &Desc,
                                 bool IsImport,
                                 Component::ExternInfo &Out) noexcept {
  Out.K = Desc.getDescType();
  auto &S = CompCtx.top();
  switch (Out.K) {
  case AST::Component::ExternDesc::DescType::CoreType: {
    const auto *Entry = S.getCoreType(Desc.getTypeIndex());
    if (Entry == nullptr) {
      spdlog::error(ErrCode::Value::ComponentTypeIndexOutOfBounds);
      spdlog::error("    Core type index {} out of bounds."sv,
                    Desc.getTypeIndex());
      return Unexpect(ErrCode::Value::ComponentTypeIndexOutOfBounds);
    }
    if (Entry->Mod == nullptr) {
      const auto Code = IsImport ? ErrCode::Value::ComponentUnknownModule
                                 : ErrCode::Value::ComponentNotModuleType;
      spdlog::error(Code);
      spdlog::error("    Core type index {} is not a module type."sv,
                    Desc.getTypeIndex());
      return Unexpect(Code);
    }
    Out.CoreMod = Entry->Mod;
    return {};
  }
  case AST::Component::ExternDesc::DescType::FuncType: {
    const auto *Entry = S.getType(Desc.getTypeIndex());
    if (Entry == nullptr) {
      spdlog::error(ErrCode::Value::ComponentTypeIndexOutOfBounds);
      spdlog::error("    Type index {} out of bounds (size {})."sv,
                    Desc.getTypeIndex(), S.Types.size());
      return Unexpect(ErrCode::Value::ComponentTypeIndexOutOfBounds);
    }
    if (Entry->DT == nullptr || !Entry->DT->isFuncType()) {
      const auto Code = IsImport ? ErrCode::Value::ComponentUnknownFunctionType
                                 : ErrCode::Value::ComponentNotFunctionType;
      spdlog::error(Code);
      spdlog::error("    Type index {} is not a function type."sv,
                    Desc.getTypeIndex());
      return Unexpect(Code);
    }
    Out.Func = {&Entry->DT->getFuncType(), Entry->Home, Entry->Remap};
    return {};
  }
  case AST::Component::ExternDesc::DescType::ValueBound: {
    if (Desc.isEqType()) {
      const uint32_t Idx = Desc.getTypeIndex();
      if (Idx >= S.Values.size()) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error("    Value index {} out of bounds (size {})."sv, Idx,
                      S.Values.size());
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
      Out.Value = S.Values[Idx].Type;
      return {};
    }
    EXPECTED_TRY(validate(Desc.getValType()));
    Out.Value = {Desc.getValType(), &S, nullptr};
    return {};
  }
  case AST::Component::ExternDesc::DescType::TypeBound: {
    if (Desc.isEqType()) {
      const auto *Entry = S.getType(Desc.getTypeIndex());
      if (Entry == nullptr) {
        spdlog::error(ErrCode::Value::ComponentTypeIndexOutOfBounds);
        spdlog::error("    Type index {} out of bounds (size {})."sv,
                      Desc.getTypeIndex(), S.Types.size());
        return Unexpect(ErrCode::Value::ComponentTypeIndexOutOfBounds);
      }
      Out.Type = *Entry;
      // The created index carries a fresh naming identity.
      Out.Type.NameId = CompTypes.newNameId();
      return {};
    }
    // (sub resource): fresh abstract resource type.
    const uint32_t Id = CompTypes.addResource(nullptr, &S, IsImport);
    Out.Type = {nullptr,
                &S,
                nullptr,
                nullptr,
                nullptr,
                Id,
                CompTypes.getResource(Id).NameId};
    return {};
  }
  case AST::Component::ExternDesc::DescType::ComponentType: {
    const auto *Entry = S.getType(Desc.getTypeIndex());
    if (Entry == nullptr) {
      spdlog::error(ErrCode::Value::ComponentTypeIndexOutOfBounds);
      spdlog::error("    Type index {} out of bounds (size {})."sv,
                    Desc.getTypeIndex(), S.Types.size());
      return Unexpect(ErrCode::Value::ComponentTypeIndexOutOfBounds);
    }
    if (Entry->Comp == nullptr) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    Type index {} is not a component type."sv,
                    Desc.getTypeIndex());
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    Out.Shape = Entry->Comp;
    return {};
  }
  case AST::Component::ExternDesc::DescType::InstanceType: {
    const auto *Entry = S.getType(Desc.getTypeIndex());
    if (Entry == nullptr) {
      spdlog::error(ErrCode::Value::ComponentTypeIndexOutOfBounds);
      spdlog::error("    Type index {} out of bounds (size {})."sv,
                    Desc.getTypeIndex(), S.Types.size());
      return Unexpect(ErrCode::Value::ComponentTypeIndexOutOfBounds);
    }
    if (Entry->Inst == nullptr) {
      const auto Code = IsImport ? ErrCode::Value::ComponentUnknownInstanceType
                                 : ErrCode::Value::ComponentNotInstanceType;
      spdlog::error(Code);
      spdlog::error("    Type index {} is not an instance type."sv,
                    Desc.getTypeIndex());
      return Unexpect(Code);
    }
    Out.Shape = Entry->Inst;
    return {};
  }
  default:
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
}

} // namespace Validator
} // namespace WasmEdge
