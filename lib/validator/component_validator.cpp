// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "validator/validator.h"

#include "ast/component.h"
#include "common/errinfo.h"
#include "common/log.h"

#include <variant>
#include <vector>

namespace WasmEdge {
namespace Validator {

Expect<void> Validator::validate(const AST::Component &Comp) {
  Checker.reset(true);

  // Validate core type section.
  if (auto Res = validate(Comp.getCoreTypeSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::CompSec_CoreType));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return Unexpect(Res);
  }

  // Validate core module sections.
  for (auto &Mod : Comp.getModuleSection().getContent()) {
    if (auto Res = validate(*Mod.get()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::CompSec_Module));
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
      return Unexpect(Res);
    }
  }

  // Validate core instance section.
  if (auto Res = validate(Comp.getCoreInstanceSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::CompSec_CoreInstance));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return Unexpect(Res);
  }

  // Validate core type section.
  if (auto Res = validate(Comp.getCoreTypeSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::CompSec_CoreType));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return Unexpect(Res);
  }

  // Validate component section.
  if (auto Res = validate(Comp.getComponentSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::CompSec_Component));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return Unexpect(Res);
  }

  // Validate instance section.
  if (auto Res = validate(Comp.getInstanceSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::CompSec_Instance));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return Unexpect(Res);
  }

  // Validate alias section.
  if (auto Res = validate(Comp.getAliasSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::CompSec_Alias));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return Unexpect(Res);
  }

  // Validate type section.
  if (auto Res = validate(Comp.getTypeSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::CompSec_Type));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return Unexpect(Res);
  }

  // Validate canon section.
  if (auto Res = validate(Comp.getCanonSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::CompSec_Canon));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return Unexpect(Res);
  }

  // Validate start section.
  if (auto Res = validate(Comp.getStartSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::CompSec_Start));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return Unexpect(Res);
  }

  // Validate import section.
  if (auto Res = validate(Comp.getImportSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::CompSec_Import));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return Unexpect(Res);
  }

  // Validate export section.
  if (auto Res = validate(Comp.getExportSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::CompSec_Export));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return Unexpect(Res);
  }

  return {};
}

// Validate core instance section.
Expect<void>
Validator::validate(__attribute__((__unused__))
                    const AST::CoreInstanceSection &CoreInstanceSec) {
  // TODO
  // https://github.com/WebAssembly/component-model/blob/11604e2ae7dd7389c926784995264487591559f6/design/mvp/Binary.md
  // Validation of core:instantiatearg initially only allows the instance sort,
  // but would be extended to accept other sorts as core wasm is extended.

  return {};
}

// Validate core type section.
Expect<void> Validator::validate(const AST::CoreTypeSection &CoreTypeSec) {
  for (auto &CoreType : CoreTypeSec.getContent()) {
    // Register type definitions into FormChecker.
    if (auto FuncType = std::get_if<AST::CoreDefType::FuncType>(&CoreType)) {
      Checker.addType(*FuncType);
    } else if (auto ModuleType =
                   std::get_if<AST::CoreDefType::ModuleType>(&CoreType)) {
      // https://github.com/WebAssembly/component-model/blob/11604e2ae7dd7389c926784995264487591559f6/design/mvp/Binary.md
      // Validation of core:moduledecl (currently) rejects core:moduletype
      // definitions inside type declarators (i.e., nested core module types).
      for (auto ModuleDecl : ModuleType->getModuleDecls()) {
        if (auto InnerCoreType =
                std::get_if<AST::CoreType>(&ModuleDecl.getContent())) {
          if (std::get_if<AST::CoreDefType::ModuleType>(InnerCoreType)) {
            spdlog::error(ErrCode::Value::NestedCoreModuleTypes);
            return Unexpect(ErrCode::Value::NestedCoreModuleTypes);
          }
        }
      }
    }
  }

  // TODO
  // https://github.com/WebAssembly/component-model/blob/11604e2ae7dd7389c926784995264487591559f6/design/mvp/Binary.md
  // As described in the explainer, each module type is validated with an
  // initially-empty type index space.

  // https://github.com/WebAssembly/component-model/blob/11604e2ae7dd7389c926784995264487591559f6/design/mvp/Binary.md
  // alias declarators currently only allow outer type aliases but would add
  // export aliases when core wasm adds type exports.

  // https://github.com/WebAssembly/component-model/blob/11604e2ae7dd7389c926784995264487591559f6/design/mvp/Binary.md
  // Validation of outer aliases cannot see beyond the enclosing core type index
  // space. Since core modules and core module types cannot nest in the MVP,
  // this means that the maximum ct in an MVP alias declarator is 1.

  // https://github.com/WebAssembly/component-model/blob/11604e2ae7dd7389c926784995264487591559f6/design/mvp/Explainer.md
  // Also, in the MVP, validation will reject nested core:moduletype, since,
  // before module-linking, core modules cannot themselves import or export
  // other core modules.

  return {};
}

// Validate component section.
Expect<void> Validator::validate(__attribute__((__unused__))
                                 const AST::ComponentSection &ComponentSec) {
  // TODO
  return {};
}

// Validate instance section.
Expect<void> Validator::validate(__attribute__((__unused__))
                                 const AST::InstanceSection &InstanceSec) {
  // TODO
  // https://github.com/WebAssembly/component-model/blob/11604e2ae7dd7389c926784995264487591559f6/design/mvp/Binary.md
  // The indices in sortidx are validated according to their sort's index
  // spaces, which are built incrementally as each definition is validated.

  return {};
}

// Validate alias section.
Expect<void> Validator::validate(__attribute__((__unused__))
                                 const AST::AliasSection &AliasSec) {
  // TODO
  // https://github.com/WebAssembly/component-model/blob/11604e2ae7dd7389c926784995264487591559f6/design/mvp/Binary.md
  // For export aliases, i is validated to refer to an instance in the instance
  // index space that exports n with the specified sort.

  // For outer aliases, ct is validated to be less or equal than the number of
  // enclosing components and i is validated to be a valid index in the sort
  // index space of the ith enclosing component (counting outward, starting with
  // 0 referring to the current component).

  // For outer aliases, validation restricts the sort to one of type, module or
  // component.

  // https://github.com/WebAssembly/component-model/blob/11604e2ae7dd7389c926784995264487591559f6/design/mvp/Explainer.md
  // In the case of export aliases, validation ensures name is an export in the
  // target instance and has a matching sort.

  return {};
}

// Validate type section.
Expect<void> Validator::validate(__attribute__((__unused__))
                                 const AST::ComponentTypeSection &TypeSec) {
  // TODO
  // https://github.com/WebAssembly/component-model/blob/11604e2ae7dd7389c926784995264487591559f6/design/mvp/Binary.md
  // Validation of valtype requires the typeidx to refer to a defvaltype.

  // Validation of instancedecl (currently) only allows outer type alias
  // declarators.

  // As described in the explainer, each component and instance type is
  // validated with an initially-empty type index space. Outer aliases can be
  // used to pull in type definitions from containing components.

  // Validation of externdesc requires the various typeidx type constructors to
  // match the preceding sort.

  // Validation of function parameter and result names, record field names,
  // variant case names, flag names, and enum case names requires that the name
  // be unique for the func, record, variant, flags, or enum type definition.

  // Validation of the optional refines clause of a variant case requires that
  // the case index is less than the current case's index (and therefore cases
  // are acyclic).

  // https://github.com/WebAssembly/component-model/blob/11604e2ae7dd7389c926784995264487591559f6/design/mvp/Explainer.md
  // In contrast to core:functype, the parameters and results of functype can
  // have associated names which validation requires to be unique.

  return {};
}

// Validate canon section.
Expect<void> Validator::validate(__attribute__((__unused__))
                                 const AST::ComponentCanonSection &CanonSec) {
  // TODO
  // https://github.com/WebAssembly/component-model/blob/11604e2ae7dd7389c926784995264487591559f6/design/mvp/Binary.md
  // Validation prevents duplicate or conflicting canonopt.

  // Validation of canon lift requires f to have type flatten(ft) (defined by
  // the Canonical ABI). The function being defined is given type ft.

  // Validation of canon lower requires f to be a component function. The
  // function being defined is given core function type flatten(ft) where ft is
  // the functype of f.

  // If the lifting/lowering operations implied by lift or lower require access
  // to memory or realloc, then validation requires these options to be present.
  // If present, realloc must have core type (func (param i32 i32 i32 i32)
  // (result i32)).

  // The post-return option is only valid for canon lift and it is always
  // optional; if present, it must have core type (func (param ...)) where the
  // number and types of the parameters must match the results of the core
  // function being lifted and itself have no result values.

  // https://github.com/WebAssembly/component-model/blob/11604e2ae7dd7389c926784995264487591559f6/design/mvp/Explainer.md
  // While the production externdesc accepts any sort, the validation rules for
  // canon lift would only allow the func sort. In the future, other sorts may
  // be added (viz., types), hence the explicit sort.

  // It is a validation error to include more than one string-encoding option.

  // The (memory ...) option specifies the memory that the Canonical ABI will
  // use to load and store values. If the Canonical ABI needs to load or store,
  // validation requires this option to be present (there is no default).

  // The (realloc ...) option specifies a core function that is validated to
  // have the following core function type:
  // (func (param $originalPtr i32)
  //       (param $originalSize i32)
  //       (param $alignment i32)
  //       (param $newSize i32)
  //       (result i32))

  // If the Canonical ABI needs realloc, validation requires this option to be
  // present (there is no default).

  // The (post-return ...) option may only be present in canon lift and
  // specifies a core function to be called with the original return values
  // after they have finished being read, allowing memory to be deallocated and
  // destructors called. This immediate is always optional but, if present, is
  // validated to have parameters matching the callee's return type and empty
  // results.

  return {};
}

// Validate start section.
Expect<void> Validator::validate(__attribute__((__unused__))
                                 const AST::ComponentStartSection &StartSec) {
  // TODO
  // https://github.com/WebAssembly/component-model/blob/11604e2ae7dd7389c926784995264487591559f6/design/mvp/Binary.md
  // Validation requires f have functype with param arity and types matching
  // arg* and result arity r.

  // Validation appends the result types of f to the value index space (making
  // them available for reference by subsequent definitions).

  // https://github.com/WebAssembly/component-model/blob/11604e2ae7dd7389c926784995264487591559f6/design/mvp/Explainer.md
  // The arity and types of the two value lists are validated to match the
  // signature of funcidx.

  return {};
}

// Validate import section.
Expect<void> Validator::validate(__attribute__((__unused__))
                                 const AST::ComponentImportSection &ImportSec) {
  // TODO
  // https://github.com/WebAssembly/component-model/blob/11604e2ae7dd7389c926784995264487591559f6/design/mvp/Binary.md
  // Validation requires all import and export names are unique.

  // Validation requires any exported sortidx to have a valid externdesc (which
  // disallows core sorts other than core module).

  return {};
}

// Validate export section.
Expect<void> Validator::validate(__attribute__((__unused__))
                                 const AST::ComponentExportSection &ExportSec) {
  // TODO
  // https://github.com/WebAssembly/component-model/blob/11604e2ae7dd7389c926784995264487591559f6/design/mvp/Binary.md
  // Validation requires all import and export names are unique.

  // Validation requires any exported sortidx to have a valid externdesc (which
  // disallows core sorts other than core module).

  return {};
}

} // namespace Validator
} // namespace WasmEdge
