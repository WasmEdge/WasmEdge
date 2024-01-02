// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "loader/loader.h"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Loader {

Expect<std::pair<std::vector<Byte>, std::vector<Byte>>> Loader::loadPreamble() {
  // component ::= <preamble> s*:<section>* => (component flatten(s*))
  // preamble  ::= <magic> <version> <layer>
  // magic     ::= 0x00 0x61 0x73 0x6D
  // version   ::= 0x0a 0x00
  // layer     ::= 0x01 0x00

  // The combination of version and layer is corresponding to the version of
  // core wasm.
  // The core module has same magic but the different version:
  // 0x01 0x00 0x00 0x00
  auto Magic = FMgr.readBytes(4);
  if (!Magic) {
    return logLoadError(Magic.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
  std::vector<Byte> WasmMagic = {0x00, 0x61, 0x73, 0x6D};
  if (*Magic != WasmMagic) {
    return logLoadError(ErrCode::Value::MalformedMagic, FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
  auto Ver = FMgr.readBytes(4);
  if (!Ver) {
    return logLoadError(Ver.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
  return std::make_pair(*Magic, *Ver);
}

Expect<std::variant<AST::Component::Component, AST::Module>>
Loader::loadUnit() {
  auto ResPreamble = Loader::loadPreamble();
  if (!ResPreamble) {
    return Unexpect(ResPreamble);
  }
  auto WasmMagic = ResPreamble->first;
  auto Ver = ResPreamble->second;
  if (Ver == ModuleVersion) {
    auto Mod = std::make_unique<AST::Module>();
    Mod->getMagic() = WasmMagic;
    Mod->getVersion() = Ver;
    if (!Conf.getRuntimeConfigure().isForceInterpreter()) {

      if (auto Res = loadModuleAOT(Mod->getAOTSection()); !Res) {
        return Unexpect(Res);
      }
    }
    // Seek to the position after the binary header.
    FMgr.seek(8);
    if (auto Res = loadModule(*Mod); !Res) {
      return Unexpect(Res);
    }

    // Load library from AOT Section for the universal WASM case.
    // For the force interpreter mode, skip this.
    if (!Conf.getRuntimeConfigure().isForceInterpreter() &&
        WASMType == InputType::UniversalWASM) {
      if (auto Res = loadUniversalWASM(*Mod); !Res) {
        return Unexpect(Res);
      }
    }
    return *Mod;
  } else if (Ver == ComponentVersion) {
    if (!Conf.hasProposal(Proposal::Component)) {
      return logNeedProposal(ErrCode::Value::IllegalOpCode, Proposal::Component,
                             FMgr.getLastOffset(), ASTNodeAttr::Component);
    }
    spdlog::warn("component model is an experimental proposal");
    auto Comp = std::make_unique<AST::Component::Component>();
    Comp->getMagic() = WasmMagic;
    Comp->getVersion() = {Ver[0], Ver[1]};
    Comp->getLayer() = {Ver[2], Ver[3]};
    if (auto Res = loadComponent(*Comp); !Res) {
      return Unexpect(Res);
    }
    return *Comp;
  } else {
    return logLoadError(ErrCode::Value::MalformedVersion, FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
}

Expect<void> Loader::loadComponent(AST::Component::Component &Comp) {
  while (auto ResSecId = FMgr.readByte()) {
    if (!ResSecId) {
      return logLoadError(ResSecId.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Component);
    }
    // keep going only if we have new section ID
    uint8_t NewSectionId = *ResSecId;

    switch (NewSectionId) {
    case 0x00:
      Comp.getCustomSections().emplace_back();
      if (auto Res = loadSection(Comp.getCustomSections().back()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      break;
    case 0x01: {
      auto ResPreamble = Loader::loadPreamble();
      if (!ResPreamble) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(ResPreamble);
      }
      auto WasmMagic = ResPreamble->first;
      auto Ver = ResPreamble->second;
      if (unlikely(Ver != ModuleVersion)) {
        return logLoadError(ErrCode::Value::MalformedVersion,
                            FMgr.getLastOffset(), ASTNodeAttr::Component);
      }
      AST::Module CoreMod;
      CoreMod.getMagic() = WasmMagic;
      CoreMod.getVersion() = Ver;
      if (auto Res = loadModule(CoreMod); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      Comp.getCoreModuleSection().getContent().push_back(CoreMod);
      break;
    }
    case 0x02:
      if (auto Res = loadSection(Comp.getCoreInstanceSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      break;
    case 0x03:
      if (auto Res = loadSection(Comp.getCoreTypeSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      break;
    case 0x04: {
      auto ResPreamble = Loader::loadPreamble();
      if (!ResPreamble) {
        return Unexpect(ResPreamble);
      }
      auto WasmMagic = ResPreamble->first;
      auto Ver = ResPreamble->second;
      if (unlikely(Ver != ComponentVersion)) {
        return logLoadError(ErrCode::Value::MalformedVersion,
                            FMgr.getLastOffset(), ASTNodeAttr::Component);
      }
      auto NestedComp = std::make_shared<AST::Component::Component>();
      NestedComp->getMagic() = WasmMagic;
      NestedComp->getVersion() = {Ver[0], Ver[1]};
      NestedComp->getLayer() = {Ver[2], Ver[3]};
      if (auto Res = loadComponent(*NestedComp); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      Comp.getComponentSection().getContent().push_back(NestedComp);
      break;
    }
    case 0x05:
      if (auto Res = loadSection(Comp.getCoreInstanceSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      break;
    case 0x06:
      if (auto Res = loadSection(Comp.getCoreTypeSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      break;
    case 0x07:
      if (auto Res = loadSection(Comp.getTypeSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      break;
    case 0x08:
      if (auto Res = loadSection(Comp.getCanonSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      break;
    case 0x09: {
      AST::Component::Start S;
      if (auto Res = loadStart(S); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      Comp.getStartSection().getContent().push_back(S);
      break;
    }
    case 0x0A:
      if (auto Res = loadSection(Comp.getImportSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      break;
    case 0x0B:
      if (auto Res = loadSection(Comp.getExportSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      break;
    default:
      return logLoadError(ErrCode::Value::MalformedSection,
                          FMgr.getLastOffset(), ASTNodeAttr::Component);
    }
  }

  return {};
}

} // namespace Loader
} // namespace WasmEdge
