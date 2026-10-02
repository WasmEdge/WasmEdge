// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <memory>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace WasmEdge {
namespace Executor {

// Instantiate core type section. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::instantiate(
    Runtime::Instance::ComponentInstance &CompInst,
    const AST::Component::CoreTypeSection &CoreTypeSec) {
  for (const auto &Ty : CoreTypeSec.getContent()) {
    CompInst.addCoreType(Ty);
  }
  return {};
}

// Instantiate type section. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
                               const AST::Component::TypeSection &TypeSec) {
  for (const auto &Ty : TypeSec.getContent()) {
    if (Ty.isResourceType()) {
      // A resource type defined here mints a fresh runtime identity, with
      // the destructor it names.
      Runtime::Instance::FunctionInstance *Dtor = nullptr;
      if (const auto DtorIdx = Ty.getResourceType().getDestructor();
          DtorIdx.has_value()) {
        EXPECTED_TRY(
            Dtor, CompInst.getCoreFunction(*DtorIdx).map_error([](auto E) {
              spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_ResourceType));
              return E;
            }));
      }
      CompInst.addResourceType(Ty, Dtor);
      continue;
    }
    CompInst.addType(Ty);
  }
  return {};
}

// Copy component. See "include/executor/component/executor.h".
void ComponentExecutor::copyComponent(
    const AST::Component::Component &Src,
    AST::Component::Component &Comp) noexcept {
  Comp.getMagic() = Src.getMagic();
  Comp.getVersion() = Src.getVersion();
  Comp.getLayer() = Src.getLayer();
  Comp.setIsValidated(Src.getIsValidated());
  auto &Secs = Comp.getSections();
  for (const auto &Sec : Src.getSections()) {
    std::visit(
        [this, &Secs](const auto &Section) {
          using T = std::decay_t<decltype(Section)>;
          // Only the sections that carry move-only definitions are rebuilt.
          if constexpr (std::is_same_v<T, AST::Component::CoreTypeSection> ||
                        std::is_same_v<T, AST::Component::ComponentSection> ||
                        std::is_same_v<T, AST::Component::TypeSection>) {
            T Copy;
            Copy.setContentSize(Section.getContentSize());
            Copy.setStartOffset(Section.getStartOffset());
            copySection(Section, Copy);
            Secs.emplace_back(std::move(Copy));
          } else {
            Secs.emplace_back(Section);
          }
        },
        Sec);
  }
}

// Copy core type section. See "include/executor/component/executor.h".
void ComponentExecutor::copySection(
    const AST::Component::CoreTypeSection &Src,
    AST::Component::CoreTypeSection &Sec) noexcept {
  auto &Content = Sec.getContent();
  for (const auto &SrcTy : Src.getContent()) {
    Content.emplace_back();
    copyType(SrcTy, Content.back());
  }
}

// Copy component section. See "include/executor/component/executor.h".
void ComponentExecutor::copySection(
    const AST::Component::ComponentSection &Src,
    AST::Component::ComponentSection &Sec) noexcept {
  Sec.getContent() = std::make_unique<AST::Component::Component>();
  copyComponent(Src.getContent(), *Sec.getContent());
}

// Copy type section. See "include/executor/component/executor.h".
void ComponentExecutor::copySection(const AST::Component::TypeSection &Src,
                                    AST::Component::TypeSection &Sec) noexcept {
  auto &Content = Sec.getContent();
  for (const auto &SrcTy : Src.getContent()) {
    Content.emplace_back();
    copyType(SrcTy, Content.back());
  }
}

// Copy core:deftype. See "include/executor/component/executor.h".
void ComponentExecutor::copyType(const AST::Component::CoreDefType &Src,
                                 AST::Component::CoreDefType &Ty) noexcept {
  if (Src.isModuleType()) {
    std::vector<AST::Component::CoreModuleDecl> Decls;
    for (const auto &SrcDecl : Src.getModuleType()) {
      Decls.emplace_back();
      copyDecl(SrcDecl, Decls.back());
    }
    Ty.setModuleType(std::move(Decls));
  } else {
    const auto SubTypes = Src.getSubTypes();
    Ty.setSubTypes(std::vector<AST::SubType>(SubTypes.begin(), SubTypes.end()));
  }
}

// Copy deftype. See "include/executor/component/executor.h".
void ComponentExecutor::copyType(const AST::Component::DefType &Src,
                                 AST::Component::DefType &Ty) noexcept {
  if (Src.isFuncType()) {
    Ty.setFuncType(AST::Component::FuncType(Src.getFuncType()));
  } else if (Src.isComponentType()) {
    AST::Component::ComponentType CompTy;
    copyType(Src.getComponentType(), CompTy);
    Ty.setComponentType(std::move(CompTy));
  } else if (Src.isInstanceType()) {
    AST::Component::InstanceType InstTy;
    copyType(Src.getInstanceType(), InstTy);
    Ty.setInstanceType(std::move(InstTy));
  } else if (Src.isResourceType()) {
    Ty.setResourceType(AST::Component::ResourceType(Src.getResourceType()));
  } else {
    Ty.setDefValType(AST::Component::DefValType(Src.getDefValType()));
  }
}

// Copy componenttype. See "include/executor/component/executor.h".
void ComponentExecutor::copyType(const AST::Component::ComponentType &Src,
                                 AST::Component::ComponentType &Ty) noexcept {
  std::vector<AST::Component::ComponentDecl> Decls;
  for (const auto &SrcDecl : Src.getDecl()) {
    Decls.emplace_back();
    copyDecl(SrcDecl, Decls.back());
  }
  Ty.setDecl(std::move(Decls));
}

// Copy instancetype. See "include/executor/component/executor.h".
void ComponentExecutor::copyType(const AST::Component::InstanceType &Src,
                                 AST::Component::InstanceType &Ty) noexcept {
  std::vector<AST::Component::InstanceDecl> Decls;
  for (const auto &SrcDecl : Src.getDecl()) {
    Decls.emplace_back();
    copyDecl(SrcDecl, Decls.back());
  }
  Ty.setDecl(std::move(Decls));
}

// Copy core:moduledecl. See "include/executor/component/executor.h".
void ComponentExecutor::copyDecl(
    const AST::Component::CoreModuleDecl &Src,
    AST::Component::CoreModuleDecl &Decl) noexcept {
  if (Src.isImport()) {
    Decl.setImport(AST::Component::CoreImportDecl(Src.getImport()));
  } else if (Src.isType()) {
    auto CoreTy = std::make_unique<AST::Component::CoreDefType>();
    copyType(*Src.getType(), *CoreTy);
    Decl.setType(std::move(CoreTy));
  } else if (Src.isAlias()) {
    Decl.setAlias(AST::Component::CoreAlias(Src.getAlias()));
  } else {
    Decl.setExport(AST::Component::CoreExportDecl(Src.getExport()));
  }
}

// Copy instancedecl. See "include/executor/component/executor.h".
void ComponentExecutor::copyDecl(const AST::Component::InstanceDecl &Src,
                                 AST::Component::InstanceDecl &Decl) noexcept {
  if (Src.isCoreType()) {
    auto CoreTy = std::make_unique<AST::Component::CoreDefType>();
    copyType(*Src.getCoreType(), *CoreTy);
    Decl.setCoreType(std::move(CoreTy));
  } else if (Src.isType()) {
    auto Ty = std::make_unique<AST::Component::DefType>();
    copyType(*Src.getType(), *Ty);
    Decl.setType(std::move(Ty));
  } else if (Src.isAlias()) {
    Decl.setAlias(AST::Component::Alias(Src.getAlias()));
  } else {
    Decl.setExport(AST::Component::ExportDecl(Src.getExport()));
  }
}

// Copy componentdecl. See "include/executor/component/executor.h".
void ComponentExecutor::copyDecl(const AST::Component::ComponentDecl &Src,
                                 AST::Component::ComponentDecl &Decl) noexcept {
  if (Src.isImportDecl()) {
    Decl.setImport(AST::Component::ImportDecl(Src.getImport()));
  } else {
    AST::Component::InstanceDecl InstDecl;
    copyDecl(Src.getInstance(), InstDecl);
    Decl.setInstance(std::move(InstDecl));
  }
}

} // namespace Executor
} // namespace WasmEdge
