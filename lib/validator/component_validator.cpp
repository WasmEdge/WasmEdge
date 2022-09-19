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
  return {};
}

// Validate core type section.
Expect<void> Validator::validate(__attribute__((__unused__))
                                 const AST::CoreTypeSection &CoreTypeSec) {
  // TODO
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
  return {};
}

// Validate alias section.
Expect<void> Validator::validate(__attribute__((__unused__))
                                 const AST::AliasSection &AliasSec) {
  // TODO
  return {};
}

// Validate type section.
Expect<void> Validator::validate(__attribute__((__unused__))
                                 const AST::ComponentTypeSection &TypeSec) {
  // TODO
  return {};
}

// Validate canon section.
Expect<void> Validator::validate(__attribute__((__unused__))
                                 const AST::ComponentCanonSection &CanonSec) {
  // TODO
  return {};
}

// Validate start section.
Expect<void> Validator::validate(__attribute__((__unused__))
                                 const AST::ComponentStartSection &StartSec) {
  // TODO
  return {};
}

// Validate import section.
Expect<void> Validator::validate(__attribute__((__unused__))
                                 const AST::ComponentImportSection &ImportSec) {
  // TODO
  return {};
}

// Validate export section.
Expect<void> Validator::validate(__attribute__((__unused__))
                                 const AST::ComponentExportSection &ExportSec) {
  // TODO
  return {};
}

} // namespace Validator
} // namespace WasmEdge
