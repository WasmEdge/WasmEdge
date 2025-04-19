// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#include "common/errinfo.h"
#include "validator/context.h"
#include "validator/validator.h"

namespace WasmEdge {
namespace Validator {

using namespace std::literals;
using namespace AST::Component;

// Helper func for better logging
inline std::string toString(SortCase SC) {
  switch (SC) {
  case SortCase::Func:
    return "Func";
  case SortCase::Value:
    return "Value";
  case SortCase::Type:
    return "Type";
  case SortCase::Component:
    return "Component";
  case SortCase::Instance:
    return "Instance";
  default:
    return "Unknown SortCase";
  }
}

inline std::string toString(CoreSort CS) {
  switch (CS) {
  case CoreSort::Func:
    return "Func";
  case CoreSort::Table:
    return "Table";
  case CoreSort::Memory:
    return "Memory";
  case CoreSort::Global:
    return "Global";
  case CoreSort::Type:
    return "Type";
  case CoreSort::Module:
    return "Module";
  case CoreSort::Instance:
    return "Instance";
  default:
    return "Unknown CoreSort";
  }
}

inline std::string toString(IndexKind IK) {
  switch (IK) {
  case IndexKind::CoreType:
    return "CoreType";
  case IndexKind::FuncType:
    return "FuncType";
  case IndexKind::ComponentType:
    return "ComponentType";
  case IndexKind::InstanceType:
    return "InstanceType";
  default:
    return "Unknown IndexKind (" + std::to_string(static_cast<int>(IK)) + ")";
  }
}

struct ExternDescVisitor {
  ExternDescVisitor(Context &Ctx) : Ctx(Ctx) {}

  Expect<void> operator()(const DescTypeIndex &DTI) {
    switch (DTI.getKind()) {
    case IndexKind::CoreType:
      Ctx.incCoreIndexSize(CoreSort::Type);
      break;
    case IndexKind::FuncType:
      Ctx.incComponentIndexSize(SortCase::Func);
      break;
    case IndexKind::ComponentType:
      Ctx.incComponentIndexSize(SortCase::Component);
      break;
    case IndexKind::InstanceType:
      Ctx.incComponentIndexSize(SortCase::Instance);
      break;
    }
    return {};
  }
  Expect<void> operator()(const TypeBound &) {
    // TypeBound == std::optional<TypeIndex>
    Ctx.incComponentIndexSize(SortCase::Type);
    return {};
  }
  Expect<void> operator()(const ValueType &) {
    Ctx.incComponentIndexSize(SortCase::Type);
    return {};
  }

private:
  Context &Ctx;
};

struct CoreInstanceExprVisitor {
  CoreInstanceExprVisitor(Context &Ctx) : Ctx(Ctx) {}

  Expect<void> operator()(const CoreInstantiate &Inst) {
    const uint32_t ModuleIdx = Inst.getModuleIdx();
    if (ModuleIdx >= Ctx.getCoreModuleCount()) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error(
          "CoreInstanceSection: Module index {} exceeds available core modules {}"sv,
          ModuleIdx, Ctx.getCoreModuleCount());
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
    auto Args = Inst.getArgs();
    auto ImportsList = getImports(Inst);
    for (auto Import : ImportsList) {
      auto ArgIt = std::find_if(Args.begin(), Args.end(), [&](const auto &Arg) {
        return Arg.getName() == Import;
      });

      if (ArgIt == Args.end()) {
        spdlog::error(ErrCode::Value::MissingArgument);
        spdlog::error("Module[{}]: Missing argument for import '{}'"sv,
                      Inst.getModuleIdx(), Import);
        return Unexpect(ErrCode::Value::MissingArgument);
      }
    }
    return {};
  }

  Expect<void> operator()(const CoreInlineExports &) { return {}; }

private:
  Context &Ctx;

  std::vector<std::string> getImports(const CoreInstantiate &Inst) {
    const uint32_t Index = Inst.getModuleIdx();
    std::vector<std::string> ImportsList;

    const auto &Mod = Ctx.getCoreModule(Index);
    const auto &ImportDesc = Mod.getImportSection().getContent();

    for (const auto &Import : ImportDesc) {
      ImportsList.emplace_back(std::string(Import.getModuleName()));
    }
    return ImportsList;
  }
};

struct InstanceExprVisitor {
  InstanceExprVisitor(Context &Ctx) : Ctx(Ctx) {}

  Expect<void> operator()(const Instantiate &Inst) {
    auto Args = Inst.getArgs();
    auto ImportMap = getImports(Inst.getComponentIdx());

    for (auto It = ImportMap.begin(); It != ImportMap.end(); ++It) {
      const auto &ImportName = It->first;
      const auto &ImportDesc = It->second;
      auto ArgIt = std::find_if(Args.begin(), Args.end(), [&](const auto &Arg) {
        return Arg.getName() == ImportName;
      });

      if (ArgIt == Args.end()) {
        spdlog::error(ErrCode::Value::MissingArgument);
        spdlog::error("Component[{}]: Missing argument for import '{}'"sv,
                      Inst.getComponentIdx(), ImportName);
        return Unexpect(ErrCode::Value::MissingArgument);
      }

      const auto &ArgSortIndex = ArgIt->getIndex();
      const auto &ArgSort = ArgSortIndex.getSort();
      const uint32_t ArgIdx = ArgSortIndex.getSortIdx();

      if (!matchImportAndArgTypes(ImportDesc, ArgSort)) {
        spdlog::error(ErrCode::Value::ArgTypeMismatch);
        spdlog::error("Component[{}]: Argument '{}' type mismatch"sv,
                      Inst.getComponentIdx(), ImportName);
        return Unexpect(ErrCode::Value::ArgTypeMismatch);
      }

      if (!isValidArgumentIndex(ArgSort, ArgIdx)) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error(
            "Component[{}]: Argument '{}' refers to invalid index {}"sv,
            Inst.getComponentIdx(), ArgIt->getName(), ArgIdx);
        return Unexpect(ErrCode::Value::InvalidIndex);
      }

      if (std::holds_alternative<SortCase>(ArgSort) &&
          std::get<SortCase>(ArgSort) == SortCase::Type &&
          std::holds_alternative<TypeBound>(ImportDesc)) {
        Ctx.substituteTypeImport(ImportName, ArgIdx);
      }
    }

    return {};
  }
  Expect<void> operator()(const CompInlineExports &Exports) {
    for (const auto &Export : Exports.getExports()) {
      auto ExportName = Export.getName();
      auto SortIdx = Export.getSortIdx();
      auto Sort = SortIdx.getSort();
      uint32_t Idx = SortIdx.getSortIdx();

      if (!isValidArgumentIndex(Sort, Idx)) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error(
            "Component: Inline export '{}' refers to invalid index {}"sv,
            ExportName, Idx);
        return Unexpect(ErrCode::Value::InvalidIndex);
      }

      if (std::holds_alternative<SortCase>(Sort) &&
          std::get<SortCase>(Sort) == SortCase::Type) {
        auto SubstitutedIdx = Ctx.getSubstitutedType(std::string(ExportName));
        if (SubstitutedIdx.has_value() && Idx != SubstitutedIdx.value()) {
          spdlog::error(ErrCode::Value::InvalidTypeReference);
          spdlog::error(
              "Component: Inline export '{}' type index {} does not match substituted type index {}"sv,
              ExportName, Idx, SubstitutedIdx.value());
          return Unexpect(ErrCode::Value::InvalidTypeReference);
        }
      }
    }
    // TODO: When validating instantiate, after each individual type-import
    // is supplied via with, the actual type supplied is immediately
    // substituted for all uses of the import, so that subsequent imports
    // and all exports are now specialized to the actual type.
    return {};
  }

private:
  Context &Ctx;

  std::unordered_map<std::string, ExternDesc> getImports(uint32_t Index) {
    std::unordered_map<std::string, ExternDesc> ImportMap;

    if (Index >= Ctx.getComponentCount()) {
      spdlog::error("Unreachable State: Index {} exceeds Component Count {}"sv,
                    Index, Ctx.getComponentCount());
      spdlog::error(WasmEdge::ErrInfo::InfoBoundary(
          Index, 0, static_cast<uint32_t>(Ctx.getComponentCount())));

      return ImportMap;
    }

    auto &Comp = Ctx.getComponent(Index);

    for (const auto &Sec : Comp.getSections()) {
      if (std::holds_alternative<ImportSection>(Sec)) {
        const auto &ImportSec = std::get<ImportSection>(Sec);

        for (const auto &Import : ImportSec.getContent()) {
          ImportMap[std::string(Import.getName())] = Import.getDesc();
        }
      }
    }

    return ImportMap;
  }

  // https://github.com/WebAssembly/component-model/blob/main/design/mvp/Explainer.md
  bool matchImportAndArgTypes(const ExternDesc &ImportType,
                              const Sort &ArgSort) {
    if (std::holds_alternative<CoreSort>(ArgSort)) {
      CoreSort ArgCoreSort = std::get<CoreSort>(ArgSort);

      if (std::holds_alternative<DescTypeIndex>(ImportType)) {
        const auto &Desc = std::get<DescTypeIndex>(ImportType);
        IndexKind Kind = Desc.getKind();

        if ((Kind == IndexKind::CoreType && ArgCoreSort == CoreSort::Type) ||
            (Kind == IndexKind::FuncType && ArgCoreSort == CoreSort::Func) ||
            (Kind == IndexKind::ComponentType &&
             ArgCoreSort == CoreSort::Module) ||
            (Kind == IndexKind::InstanceType &&
             ArgCoreSort == CoreSort::Instance)) {
          return true;
        }
        spdlog::error("[Core Sort] Type mismatch: Expected '{}' but got '{}'"sv,
                      WasmEdge::Validator::toString(Kind),
                      WasmEdge::Validator::toString(ArgCoreSort));

        return false;
      } else if (std::holds_alternative<ValueType>(ImportType)) {
        if (ArgCoreSort == CoreSort::Func || ArgCoreSort == CoreSort::Table ||
            ArgCoreSort == CoreSort::Memory ||
            ArgCoreSort == CoreSort::Global) {
          return true;
        }
        spdlog::error(
            "[Core Sort] Type mismatch: Expected 'Func', 'Table', 'Memory', or 'Global' but got '{}'"sv,
            WasmEdge::Validator::toString(ArgCoreSort));
        return false;
      } else if (std::holds_alternative<TypeBound>(ImportType)) {
        if (ArgCoreSort == CoreSort::Module ||
            ArgCoreSort == CoreSort::Instance) {
          return true;
        }
        spdlog::error(
            "[Core Sort] Type mismatch: Expected 'Module' or 'Instance' but got '{}'"sv,
            WasmEdge::Validator::toString(ArgCoreSort));
        return false;
      }
    } else if (std::holds_alternative<SortCase>(ArgSort)) {
      SortCase ArgSortCase = std::get<SortCase>(ArgSort);

      if (std::holds_alternative<TypeBound>(ImportType)) {
        if (ArgSortCase == SortCase::Type) {
          return true;
        }
        spdlog::error(
            "[Sort Case] Type mismatch: Expected 'Type' but got '{}'"sv,
            WasmEdge::Validator::toString(ArgSortCase));
        return false;
      } else if (std::holds_alternative<ValueType>(ImportType)) {
        if (ArgSortCase == SortCase::Value) {
          return true;
        }
        spdlog::error(
            "[Sort Case] Type mismatch: Expected 'Value' but got '{}'"sv,
            WasmEdge::Validator::toString(ArgSortCase));
        return false;
      } else if (std::holds_alternative<DescTypeIndex>(ImportType)) {
        const auto &Desc = std::get<DescTypeIndex>(ImportType);
        IndexKind Kind = Desc.getKind();

        if ((Kind == IndexKind::ComponentType &&
             ArgSortCase == SortCase::Component) ||
            (Kind == IndexKind::InstanceType &&
             ArgSortCase == SortCase::Instance) ||
            (Kind == IndexKind::FuncType && ArgSortCase == SortCase::Func) ||
            (Kind == IndexKind::CoreType && ArgSortCase == SortCase::Type)) {
          return true;
        }

        spdlog::error("[Sort Case] Type mismatch: Expected '{}' but got '{}'"sv,
                      WasmEdge::Validator::toString(Kind),
                      WasmEdge::Validator::toString(ArgSortCase));
        return false;
      }
    }

    spdlog::error(
        "Unhandled type comparison: ImportType and ArgSort do not match any known case."sv);
    return false;
  }

  bool isValidArgumentIndex(const Sort &ArgSort, const uint32_t Idx) {
    if (std::holds_alternative<SortCase>(ArgSort)) {
      auto ArgSortCase = std::get<SortCase>(ArgSort);
      if (Ctx.getComponentIndexSize(ArgSortCase) > Idx) {
        spdlog::debug("{} >= {}", Ctx.getComponentIndexSize(ArgSortCase), Idx);
        return true;
      }
    } else {
      auto ArgSortCase = std::get<CoreSort>(ArgSort);
      if (Ctx.getCoreIndexSize(ArgSortCase) > Idx) {
        spdlog::debug("{} >= {}", Ctx.getCoreIndexSize(ArgSortCase), Idx);
        return true;
      }
    }
    return false;
  }
};

struct AliasTargetVisitor {
  AliasTargetVisitor(const Sort &S) : S{S} {}

  Expect<void> operator()(const AliasTargetExport &) {
    // TODO: For export aliases, i is validated to refer to an instance in
    // the instance index space that exports n with the specified sort.
    return {};
  }
  Expect<void> operator()(const AliasTargetOuter &) {
    // TODO: For outer aliases, ct is validated to be less or equal than the
    // number of enclosing components and i is validated to be a valid index
    // in the sort index space of the ith enclosing component (counting
    // outward, starting with 0 referring to the current component).

    // TODO: For outer aliases, validation restricts the sort to one of
    // type, module or component and additionally requires that the
    // outer-aliased type is not a resource type (which is generative).

    return {};
  }

  const Sort &S;
};

struct ModuleDeclVisitor {
  // TODO: Validation of core:moduledecl rejects core:moduletype
  // definitions and outer aliases of core:moduletype definitions inside
  // type declarators. Thus, as an invariant, when validating a
  // core:moduletype, the core type index space will not contain any
  // core module types.
  Expect<void> operator()(const AST::ImportDesc &) { return {}; }
  Expect<void> operator()(const std::shared_ptr<CoreType> &) { return {}; }
  Expect<void> operator()(const Alias &) { return {}; }
  Expect<void> operator()(const CoreExportDecl &) { return {}; }
};
struct CoreDefTypeVisitor {
  CoreDefTypeVisitor(Context &Ctx) : Ctx(Ctx) {}

  Expect<void> operator()(const AST::FunctionType &) {
    Ctx.incCoreIndexSize(CoreSort::Type);
    return {};
  }
  Expect<void> operator()(const ModuleType &Mod) {
    for (const ModuleDecl &D : Mod.getContent()) {
      EXPECTED_TRY(std::visit(ModuleDeclVisitor{}, D));
    }
    return {};
  }

private:
  Context &Ctx;
};

struct DefTypeVisitor {
  DefTypeVisitor(Context &Ctx) : Ctx(Ctx) {}

  Expect<void> operator()(const DefValType &) {
    // TODO: Validation of valtype requires the typeidx to refer to a
    // defvaltype.

    // TODO: Validation of own and borrow requires the typeidx to refer to a
    // resource type.
    Ctx.incComponentIndexSize(SortCase::Type);
    return {};
  }
  Expect<void> operator()(const FuncType &) {
    // TODO: Validation of functype rejects any transitive use of borrow in
    // a result type. Similarly, validation of components and component
    // types rejects any transitive use of borrow in an exported value type.
    Ctx.incComponentIndexSize(SortCase::Type);
    return {};
  }
  Expect<void> operator()(const ComponentType &CT) {
    for (auto &Decl : CT.getContent()) {
      if (std::holds_alternative<ImportDecl>(Decl)) {
        auto &I = std::get<ImportDecl>(Decl);
        check(I.getExternDesc());
      } else if (std::holds_alternative<InstanceDecl>(Decl)) {
        check(std::get<InstanceDecl>(Decl));
      }
      Ctx.incComponentIndexSize(SortCase::Type);
    }
    // TODO: Validation rejects resourcetype type definitions inside
    // componenttype and instancettype. Thus, handle types inside a
    // componenttype can only refer to resource types that are imported or
    // exported.

    // TODO: As described in the explainer, each component and instance type
    // is validated with an initially-empty type index space. Outer aliases
    // can be used to pull in type definitions from containing components.

    return {};
  }
  Expect<void> operator()(const InstanceType &) {
    Ctx.incComponentIndexSize(SortCase::Type);
    return {};
  }
  Expect<void> operator()(const ResourceType &) {
    Ctx.incComponentIndexSize(SortCase::Type);
    return {};
  }

  void check(const InstanceDecl &) {
    // TODO: Validation of instancedecl (currently) only allows the type and
    // instance sorts in alias declarators.
  }

  void check(const ExternDesc &) {
    // TODO: Validation of externdesc requires the various typeidx type
    // constructors to match the preceding sort.
  }

private:
  Context &Ctx;
};
struct CanonVisitor {
  CanonVisitor(Context &Ctx) : Ctx(Ctx) {}

  Expect<void> operator()(const Lift &) {
    // TODO: validation specifies
    // = $callee must have type flatten_functype($opts, $ft, 'lift')
    // = $f is given type $ft
    // = a memory is present if required by lifting and is a subtype of
    //       (memory 1)
    // = a realloc is present if required by lifting and has type
    //       (func (param i32 i32 i32 i32) (result i32))
    // = if a post-return is present, it has type
    //       (func (param flatten_functype({}, $ft, 'lift').results))
    Ctx.incComponentIndexSize(SortCase::Func);
    return {};
  }
  Expect<void> operator()(const Lower &) {
    // TODO: where $callee has type $ft, validation specifies
    // = $f is given type flatten_functype($opts, $ft, 'lower')
    // = a memory is present if required by lifting and is a subtype of
    //     (memory 1)
    // = a realloc is present if required by lifting and has type
    //     (func (param i32 i32 i32 i32) (result i32))
    // = there is no post-return in $opts
    Ctx.incCoreIndexSize(CoreSort::Func);
    return {};
  }
  Expect<void> operator()(const ResourceNew &) {
    // TODO: validation specifies
    // = $rt must refer to locally-defined (not imported) resource type
    // = $f is given type (func (param $rt.rep) (result i32)), where $rt.rep is
    // currently fixed to be i32.
    Ctx.incCoreIndexSize(CoreSort::Func);
    return {};
  }
  Expect<void> operator()(const ResourceDrop &) {
    // TODO: validation specifies
    // = $rt must refer to resource type
    // = $f is given type (func (param i32))
    Ctx.incCoreIndexSize(CoreSort::Func);
    return {};
  }
  Expect<void> operator()(const ResourceRep &) {
    // TODO: validation specifies
    // = $rt must refer to a locally-defined (not imported) resource type
    // = $f is given type (func (param i32) (result $rt.rep)), where $rt.rep is
    // currently fixed to be i32.
    Ctx.incCoreIndexSize(CoreSort::Func);
    return {};
  }
  // TODO: The following are not yet exists in WasmEdge, so just list entries
  // 1. canon task.backpressure
  // 2. canon task.start
  // 3. canon task.return
  // 4. canon task.wait
  // 5. canon task.poll
  // 6. canon task.yield
  // 7. canon thread.spawn
  // 8. canon thread.hw_concurrency
private:
  Context &Ctx;
};

struct SectionVisitor {
  SectionVisitor(Validator &V, Context &Ctx) : V(V), Ctx(Ctx) {}

  Expect<void> operator()(const AST::CustomSection &) {
    spdlog::debug("Custom Section"sv);
    return {};
  }
  Expect<void> operator()(const AST::CoreModuleSection &Sec) {
    spdlog::debug("CoreModule Section"sv);
    auto &Mod = Sec.getContent();
    V.validate(Mod);
    Ctx.addCoreModule(Mod);
    Ctx.incCoreIndexSize(CoreSort::Module);
    return {};
  }
  Expect<void> operator()(const ComponentSection &Sec) {
    spdlog::debug("Component Section"sv);
    auto &C = Sec.getContent();
    ComponentContextGuard guard(C, Ctx);
    V.validate(C);
    Ctx.incComponentIndexSize(SortCase::Component);
    return {};
  }
  Expect<void> operator()(const CoreInstanceSection &Sec) {
    spdlog::debug("CoreInstance Section"sv);
    for (const CoreInstanceExpr &E : Sec.getContent()) {
      CoreInstanceExprVisitor Visitor{Ctx};
      EXPECTED_TRY(std::visit(Visitor, E));
      Ctx.incCoreIndexSize(CoreSort::Instance);
    }
    // NOTE: refers below InstanceSection, and copy the similar structure

    // TODO: Validation of core:instantiatearg initially only allows the
    // instance sort, but would be extended to accept other sorts as core wasm
    // is extended.
    return {};
  }
  Expect<void> operator()(const InstanceSection &Sec) {
    spdlog::debug("Instance Section"sv);
    for (const InstanceExpr &E : Sec.getContent()) {
      InstanceExprVisitor Visitor{Ctx};
      EXPECTED_TRY(std::visit(Visitor, E));
      Ctx.incComponentIndexSize(SortCase::Instance);
    }
    return {};
  }
  Expect<void> operator()(const AliasSection &Sec) {
    spdlog::debug("Alias Section"sv);
    for (const Alias &A : Sec.getContent()) {
      EXPECTED_TRY(std::visit(AliasTargetVisitor{A.getSort()}, A.getTarget()));
      auto Sort = A.getSort();
      if (std::holds_alternative<SortCase>(Sort)) {
        Ctx.incComponentIndexSize(std::get<SortCase>(Sort));
      } else if (std::holds_alternative<CoreSort>(Sort)) {
        Ctx.incCoreIndexSize(std::get<CoreSort>(Sort));
      }
    }
    return {};
  }
  Expect<void> operator()(const CoreTypeSection &Sec) {
    spdlog::debug("CoreType Section"sv);
    // TODO: As described in the explainer, each module type is validated with
    // an initially-empty type index space.
    for (const CoreDefType &T : Sec.getContent()) {
      CoreDefTypeVisitor Visitor{Ctx};
      EXPECTED_TRY(std::visit(Visitor, T));
    }
    return {};
  }
  Expect<void> operator()(const TypeSection &Sec) {
    spdlog::debug("Type Section"sv);
    for (const DefType &T : Sec.getContent()) {
      DefTypeVisitor Visitor{Ctx};
      EXPECTED_TRY(std::visit(Visitor, T));
    }
    // TODO: Validation of resourcetype requires the destructor (if present) to
    // have type [i32] -> [].

    return {};
  }
  Expect<void> operator()(const CanonSection &Sec) {
    spdlog::debug("Canon Section"sv);
    // TODO: Validation prevents duplicate or conflicting canonopt.
    for (const Canon &C : Sec.getContent()) {
      CanonVisitor Visitor{Ctx};
      EXPECTED_TRY(std::visit(Visitor, C));
    }

    return {};
  }
  Expect<void> operator()(const StartSection &) {
    spdlog::debug("Start Section"sv);
    // API:
    // const Start &S = Sec.getContent();
    // S.getFunctionIndex();
    // S.getArguments();
    // S.getResult();

    // TODO: Validation requires f have functype with param arity and types
    // matching arg* and result arity r.

    // TODO: Validation appends the result types of f to the value index space
    // (making them available for reference by subsequent definitions).

    // TODO: In addition to the type-compatibility checks mentioned above, the
    // validation rules for value definitions additionally require that each
    // value is consumed exactly once. Thus, during validation, each value has
    // an associated "consumed" boolean flag. When a value is first added to
    // the value index space (via import, instance, alias or start), the flag
    // is clear. When a value is used (via export, instantiate or start), the
    // flag is set. After validating the last definition of a component,
    // validation requires all values' flags are set.

    return {};
  }
  Expect<void> operator()(const ImportSection &Sec) {
    spdlog::debug("Import Section"sv);
    // NOTE: This section share the validation rules with export section, one
    // must share the implementation as much as possible.
    for (const Import &I : Sec.getContent()) {
      // TODO: Validation requires that annotated plainnames only occur on func
      // imports or exports and that the first label of a [constructor],
      // [method] or [static] matches the plainname of a preceding resource
      // import or export, respectively, in the same scope (component, component
      // type or instance type).

      // TODO: Validation of [constructor] names requires that the func returns
      // a (result (own $R)), where $R is the resource labeled r.

      // TODO: Validation of [method] names requires the first parameter of the
      // function to be (param "self" (borrow $R)), where $R is the resource
      // labeled r.

      // TODO: Validation of [method] and [static] names ensures that all field
      // names are disjoint.
      I.getName();

      ExternDescVisitor Visitor{Ctx};
      EXPECTED_TRY(std::visit(Visitor, I.getDesc()));
    }
    // TODO: Validation requires that all resource types transitively used in
    // the type of an export are introduced by a preceding importdecl or
    // exportdecl.

    return {};
  }
  Expect<void> operator()(const ExportSection &Sec) {
    spdlog::debug("Export Section"sv);
    for (const Export &E : Sec.getContent()) {
      auto SI = E.getSortIndex();
      auto Sort = SI.getSort();
      SI.getSortIdx();
      if (E.getDesc().has_value()) {
        // TODO: Validation requires any exported sortidx to have a valid
        // externdesc (which disallows core sorts other than core module). When
        // the optional externdesc immediate is present, validation requires it
        // to be a supertype of the inferred externdesc of the sortidx.
        ExternDescVisitor Visitor{Ctx};
        EXPECTED_TRY(std::visit(Visitor, *E.getDesc()));
      } else if (std::holds_alternative<SortCase>(Sort)) {
        auto SC = std::get<SortCase>(Sort);
        Ctx.incComponentIndexSize(SC);
      }
    }

    return {};
  }

private:
  Validator &V;
  Context &Ctx;
};

} // namespace Validator
} // namespace WasmEdge
