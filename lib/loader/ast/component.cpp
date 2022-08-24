// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "loader/loader.h"

#include <cstdint>
#include <utility>

namespace WasmEdge {
namespace Loader {

Expect<std::unique_ptr<AST::Component>> Loader::loadComponent() {
  auto Comp = std::make_unique<AST::Component>();
  // component ::= <preamble> s*:<section>*
  // preamble ::= <magic> <version> <layer>
  // <magic>
  if (auto Res = FMgr.readBytes(4)) {
    std::vector<Byte> WasmMagic = {0x00, 0x61, 0x73, 0x6D};
    if (*Res != WasmMagic) {
      return logLoadError(ErrCode::Value::MalformedMagic, FMgr.getLastOffset(),
                          ASTNodeAttr::Component);
    }
    Comp->getMagic() = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
  // <version>
  if (auto Res = FMgr.readBytes(2)) {
    std::vector<Byte> Version = {0x0a, 0x00};
    if (*Res != Version) {
      return logLoadError(ErrCode::Value::MalformedVersion,
                          FMgr.getLastOffset(), ASTNodeAttr::Component);
    }
    Comp->getVersion() = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
  // <layer>
  if (auto Res = FMgr.readBytes(2)) {
    std::vector<Byte> Layer = {0x01, 0x00};
    if (*Res != Layer) {
      return logLoadError(ErrCode::Value::MalformedVersion,
                          FMgr.getLastOffset(), ASTNodeAttr::Component);
    }
    Comp->getLayer() = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }

  // section   ::=    section_0(<core:custom>)         => ϵ
  //             | m*:section_1(<core:module>)         => [core-prefix(m)]
  //             | i*:section_2(vec(<core:instance>))  => core-prefix(i)*
  //             | a*:section_3(vec(<core:alias>))     => core-prefix(a)*
  //             | t*:section_4(vec(<core:type>))      => core-prefix(t)*
  //             | c: section_5(<component>)           => [c]
  //             | i*:section_6(vec(<instance>))       => i*
  //             | a*:section_7(vec(<alias>))          => a*
  //             | t*:section_8(vec(<type>))           => t*
  //             | c*:section_9(vec(<canon>))          => c*
  //             | s: section_10(<start>)              => [s]
  //             | i*:section_11(vec(<import>))        => i*
  //             | e*:section_12(vec(<export>))        => e*
  while (true) {
    uint8_t NewSectionId = 0x00;
    // If not read section ID, seems the end of file and break.
    if (auto Res = FMgr.readByte(); Res.has_value()) {
      NewSectionId = *Res;
    } else {
      if (Res.error() == ErrCode::Value::UnexpectedEnd) {
        break;
      }
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Component);
    }

    // NOTE: section is not unique in component model
    switch (NewSectionId) {
    case 0x00:
      // section_0(<core:custom>)            => ϵ
      //
      // this is an error because component model ensure this section is omitted
      // for now:
      // https://github.com/WebAssembly/component-model/commit/d334e4db4a1cef4902871555cf4283e4114fefa5
      return logLoadError(ErrCode::Value::MalformedSection,
                          FMgr.getLastOffset(), ASTNodeAttr::Component);
    case 0x01:
      // m*:section_1(<core:module>)         => [core-prefix(m)]
      if (auto Res = FMgr.readByte(); !Res) {
        return logLoadError(Res.error(), FMgr.getLastOffset(),
                            ASTNodeAttr::Component);
      }
      if (auto Mod = loadModule()) {
        Comp->getModuleSection().getContent().push_back(std::move(*Mod));
      } else {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Mod);
      }
      break;
    case 0x02:
      // i*:section_2(vec(<core:instance>))  => core-prefix(i)*
      if (auto Res = loadSection(Comp->getCoreInstanceSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      break;
    case 0x03:
      // a*:section_3(vec(<core:alias>))     => core-prefix(a)*
      if (auto Res = loadSection(Comp->getCoreAliasSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      break;
    case 0x04:
      // t*:section_4(vec(<core:type>))      => core-prefix(t)*
      if (auto Res = loadSection(Comp->getCoreTypeSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      break;
    case 0x05:
      // c: section_5(<component>)           => [c]
      if (auto C = loadComponent()) {
        Comp->getComponentSection().getContent().push_back(std::move(*C));
      } else {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(C);
      }
      break;
    case 0x06:
      // i*:section_6(vec(<instance>))       => i*
      if (auto Res = loadSection(Comp->getInstanceSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      break;
    case 0x07:
      // a*:section_7(vec(<alias>))          => a*
      if (auto Res = loadSection(Comp->getAliasSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      break;
    case 0x08:
      // t*:section_8(vec(<type>))           => t*
      if (auto Res = loadSection(Comp->getTypeSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      break;
    case 0x09:
      // c*:section_9(vec(<canon>))          => c*
      if (auto Res = loadSection(Comp->getCanonSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      break;
    case 0x0A: {
      // s: section_10(<start>)              => [s]
      AST::Start S;
      if (auto Res = loadStart(S)) {
        Comp->getStartSection().getContent().push_back(S);
      } else {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      break;
    }
    case 0x0B:
      // i*:section_11(vec(<import>))        => i*
      //
      // import definitions
      if (auto Res = loadSection(Comp->getImportSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      break;
    case 0x0C:
      // e*:section_12(vec(<export>))        => e*
      //
      // export definitions
      if (auto Res = loadSection(Comp->getExportSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      break;
    default:
      return logLoadError(ErrCode::Value::MalformedSection,
                          FMgr.getLastOffset(), ASTNodeAttr::Component);
    }
  }

  return Comp;
}

} // namespace Loader
} // namespace WasmEdge
