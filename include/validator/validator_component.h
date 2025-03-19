// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#include "validator/validator.h"
#include "common/errinfo.h"


namespace WasmEdge {
namespace Validator {

using namespace AST::Component;

class Context {
public:
  void addComponent(const Component &C) noexcept { CompList.emplace_back(C); }

  const Component &getComponent(uint32_t Index) const noexcept {
    return CompList[Index];
  }

  size_t getComponentCount() const noexcept { return CompList.size(); }

private:
  std::vector<Component> CompList;
};

struct ExternDescVisitor {
  Expect<void> operator()(const DescTypeIndex &) { return {}; }
  Expect<void> operator()(const TypeBound &) {
    // TypeBound == std::optional<TypeIndex>
    return {};
  }
  Expect<void> operator()(const ValueType &) { return {}; }
};

struct InstanceExprVisitor {
  InstanceExprVisitor(Context &Ctx) : Ctx(Ctx) {}

  Expect<void> operator()(const Instantiate &Inst) {
    auto Args = Inst.getArgs();
    auto ImportMap = getImports(Inst.getComponentIdx());

    for (const auto &Arg : Args) {
      const auto &ArgName = Arg.getName();
      auto it = ImportMap.find(std::string(ArgName));

      if (it == ImportMap.end()) {
        spdlog::error(ErrCode::Value::UnknownArgument);
        spdlog::error(
            WasmEdge::ErrInfo::InfoRegistering(std::to_string(Inst.getComponentIdx())));
        spdlog::error("Component[{}]: Unknown argument '{}'",
                      Inst.getComponentIdx(), ArgName);
        return Unexpect(ErrCode::Value::UnknownArgument);
      }

      if (!matchImportAndArgTypes(it->second, Arg.getIndex().getSort())) {
        spdlog::error(ErrCode::Value::ArgTypeMismatch);
        spdlog::error("Component[{}]: Argument '{}' type mismatch",
                      Inst.getComponentIdx(), ArgName);
        return Unexpect(ErrCode::Value::ArgTypeMismatch);
    }

      ImportMap.erase(it);
    }

    // TODO: The indices in sortidx are validated according to their sort's
    // index spaces, which are built incrementally as each definition is
    // validated.
    return {};
  }
  Expect<void> operator()(const CompInlineExports &) {
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
      spdlog::error("Unreachable State: Index {} exceeds Component Count {}",
                    Index, Ctx.getComponentCount());
      spdlog::error(
          WasmEdge::ErrInfo::InfoBoundary(Index, 0, Ctx.getComponentCount()));

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

  bool matchImportAndArgTypes(const ExternDesc &ImportType,
                              const Sort &ArgSort) {
    SortCase ArgSortCase = std::get<SortCase>(ArgSort);

    if (std::holds_alternative<TypeBound>(ImportType)) {
      return ArgSortCase == SortCase::Instance;
    } else if (std::holds_alternative<ValueType>(ImportType)) {
      return ArgSortCase == SortCase::Value;
    } else if (std::holds_alternative<DescTypeIndex>(ImportType)) {
      return ArgSortCase == SortCase::Type;
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
  Expect<void> operator()(const AST::FunctionType &) { return {}; }
  Expect<void> operator()(const ModuleType &Mod) {
    for (const ModuleDecl &D : Mod.getContent()) {
      EXPECTED_TRY(std::visit(ModuleDeclVisitor{}, D));
    }
    return {};
  }
};

struct DefTypeVisitor {
  Expect<void> operator()(const DefValType &) {
    // TODO: Validation of valtype requires the typeidx to refer to a
    // defvaltype.

    // TODO: Validation of own and borrow requires the typeidx to refer to a
    // resource type.
    return {};
  }
  Expect<void> operator()(const FuncType &) {
    // TODO: Validation of functype rejects any transitive use of borrow in
    // a result type. Similarly, validation of components and component
    // types rejects any transitive use of borrow in an exported value type.
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
  Expect<void> operator()(const InstanceType &) { return {}; }
  Expect<void> operator()(const ResourceType &) { return {}; }

  void check(const InstanceDecl &) {
    // TODO: Validation of instancedecl (currently) only allows the type and
    // instance sorts in alias declarators.
  }

  void check(const ExternDesc &) {
    // TODO: Validation of externdesc requires the various typeidx type
    // constructors to match the preceding sort.
  }
};
struct CanonVisitor {
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
    return {};
  }
  Expect<void> operator()(const ResourceNew &) {
    // TODO: validation specifies
    // = $rt must refer to locally-defined (not imported) resource type
    // = $f is given type (func (param $rt.rep) (result i32)), where $rt.rep is
    // currently fixed to be i32.
    return {};
  }
  Expect<void> operator()(const ResourceDrop &) {
    // TODO: validation specifies
    // = $rt must refer to resource type
    // = $f is given type (func (param i32))
    return {};
  }
  Expect<void> operator()(const ResourceRep &) {
    // TODO: validation specifies
    // = $rt must refer to a locally-defined (not imported) resource type
    // = $f is given type (func (param i32) (result $rt.rep)), where $rt.rep is
    // currently fixed to be i32.
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
};

struct SectionVisitor {
  SectionVisitor(Validator &V, Context &Ctx) : V(V), Ctx(Ctx) {}

  Expect<void> operator()(const AST::CustomSection &) { return {}; }
  Expect<void> operator()(const AST::CoreModuleSection &Sec) {
    auto &Mod = Sec.getContent();
    V.validate(Mod);
    return {};
  }
  Expect<void> operator()(const ComponentSection &Sec) {
    auto &C = Sec.getContent();
    Ctx.addComponent(C);
    V.validate(C);
    return {};
  }
  Expect<void> operator()(const CoreInstanceSection &) {
    // NOTE: refers below InstanceSection, and copy the similar structure

    // TODO: Validation of core:instantiatearg initially only allows the
    // instance sort, but would be extended to accept other sorts as core wasm
    // is extended.
    return {};
  }
  Expect<void> operator()(const InstanceSection &Sec) {
    for (const InstanceExpr &E : Sec.getContent()) {
      InstanceExprVisitor Visitor(Ctx);
      EXPECTED_TRY(std::visit(Visitor, E));
    }
    return {};
  }
  Expect<void> operator()(const AliasSection &Sec) {
    for (const Alias &A : Sec.getContent()) {
      EXPECTED_TRY(std::visit(AliasTargetVisitor{A.getSort()}, A.getTarget()));
    }
    return {};
  }
  Expect<void> operator()(const CoreTypeSection &Sec) {
    // TODO: As described in the explainer, each module type is validated with
    // an initially-empty type index space.
    for (const CoreDefType &T : Sec.getContent()) {
      EXPECTED_TRY(std::visit(CoreDefTypeVisitor{}, T));
    }
    return {};
  }
  Expect<void> operator()(const TypeSection &Sec) {
    for (const DefType &T : Sec.getContent()) {
      EXPECTED_TRY(std::visit(DefTypeVisitor{}, T));
    }
    // TODO: Validation of resourcetype requires the destructor (if present) to
    // have type [i32] -> [].

    return {};
  }
  Expect<void> operator()(const CanonSection &Sec) {
    // TODO: Validation prevents duplicate or conflicting canonopt.
    for (const Canon &C : Sec.getContent()) {
      EXPECTED_TRY(std::visit(CanonVisitor{}, C));
    }

    return {};
  }
  Expect<void> operator()(const StartSection &) {
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

      EXPECTED_TRY(std::visit(ExternDescVisitor{}, I.getDesc()));
    }
    // TODO: Validation requires that all resource types transitively used in
    // the type of an export are introduced by a preceding importdecl or
    // exportdecl.

    return {};
  }
  Expect<void> operator()(const ExportSection &Sec) {
    for (const Export &E : Sec.getContent()) {
      if (E.getDesc().has_value()) {
        // TODO: Validation requires any exported sortidx to have a valid
        // externdesc (which disallows core sorts other than core module). When
        // the optional externdesc immediate is present, validation requires it
        // to be a supertype of the inferred externdesc of the sortidx.
        auto SI = E.getSortIndex();
        SI.getSort();
        SI.getSortIdx();
        EXPECTED_TRY(std::visit(ExternDescVisitor{}, *E.getDesc()));
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
