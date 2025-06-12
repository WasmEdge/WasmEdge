// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

// Load component core:module section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::Component::CoreModuleSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() -> Expect<void> {
    auto ReportError = [](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_CoreMod));
      return E;
    };
    auto ExpectedSize = Sec.getContentSize();
    auto StartOffset = FMgr.getOffset();

    EXPECTED_TRY(auto Preamble, Loader::loadPreamble().map_error(ReportError));
    auto &[WasmMagic, Ver] = Preamble;
    if (unlikely(Ver != ModuleVersion)) {
      return logLoadError(ErrCode::Value::MalformedVersion,
                          FMgr.getLastOffset(), ASTNodeAttr::Comp_Sec_CoreMod);
    }
    AST::Module &CoreMod = Sec.getContent();
    CoreMod.getMagic() = WasmMagic;
    CoreMod.getVersion() = Ver;

    auto Offset = FMgr.getOffset();
    if (unlikely(ExpectedSize < Offset - StartOffset)) {
      return logLoadError(ErrCode::Value::UnexpectedEnd, FMgr.getLastOffset(),
                          ASTNodeAttr::Comp_Sec_CoreMod);
    }

    EXPECTED_TRY(loadModule(CoreMod, ExpectedSize - (Offset - StartOffset))
                     .map_error(ReportError));
    return {};
  });
}

// Load component core:instance section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::Component::CoreInstanceSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::Component::CoreInstance &Instance) {
          return loadCoreInstance(Instance);
        });
  });
}

// Load component core:type section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::Component::CoreTypeSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::Component::CoreDefType &Ty) { return loadType(Ty); });
  });
}

// Load component nested-component section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::Component::ComponentSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() -> Expect<void> {
    auto ReportError = [](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Component));
      return E;
    };
    auto ExpectedSize = Sec.getContentSize();
    auto StartOffset = FMgr.getOffset();

    EXPECTED_TRY(auto Preamble, loadPreamble().map_error(ReportError));
    auto &[WasmMagic, Ver] = Preamble;
    if (unlikely(Ver != ComponentVersion)) {
      return logLoadError(ErrCode::Value::MalformedVersion,
                          FMgr.getLastOffset(),
                          ASTNodeAttr::Comp_Sec_Component);
    }
    auto NestedComp = std::make_unique<AST::Component::Component>();
    NestedComp->getMagic() = WasmMagic;
    NestedComp->getVersion() = {Ver[0], Ver[1]};
    NestedComp->getLayer() = {Ver[2], Ver[3]};

    auto Offset = FMgr.getOffset();
    if (unlikely(ExpectedSize < Offset - StartOffset)) {
      return logLoadError(ErrCode::Value::UnexpectedEnd, FMgr.getLastOffset(),
                          ASTNodeAttr::Component);
    }

    EXPECTED_TRY(
        loadComponent(*NestedComp, ExpectedSize - (Offset - StartOffset))
            .map_error(ReportError));
    Sec.getContent() = std::move(NestedComp);
    return {};
  });
}

// Load component instance section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::Component::InstanceSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(Sec,
                                 [this](AST::Component::Instance &Instance) {
                                   return loadInstance(Instance);
                                 });
  });
}

// Load component alias section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::Component::AliasSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::Component::Alias &Alias) { return loadAlias(Alias); });
  });
}

// Load component type section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::Component::TypeSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::Component::DefType &Ty) { return loadType(Ty); });
  });
}

// Load component cannon section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::Component::CanonSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::Component::Canonical &C) { return loadCanonical(C); });
  });
}

// Load component start section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::Component::StartSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() -> Expect<void> {
    return loadStart(Sec.getContent()).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Start));
      return E;
    });
  });
}

// Load component import section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::Component::ImportSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::Component::Import &C) { return loadImport(C); });
  });
}

// Load component export section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::Component::ExportSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::Component::Export &C) { return loadExport(C); });
  });
}

// TODO: COMPONENT - Load component value section.

} // namespace Loader
} // namespace WasmEdge
