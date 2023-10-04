// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "loader/loader.h"

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Loader {

Expect<std::variant<AST::Component, AST::Module>> Loader::loadUnit() {
  // component ::= <preamble> s*:<section>* => (component flatten(s*))
  // preamble  ::= <magic> <version> <layer>
  // magic     ::= 0x00 0x61 0x73 0x6D
  // version   ::= 0x0a 0x00
  // layer     ::= 0x01 0x00

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
  std::vector<Byte> ModuleVersion = {0x01, 0x00, 0x00, 0x00};
  std::vector<Byte> ComponentVersion = {0x0a, 0x00, 0x01, 0x00};
  if (*Ver == ModuleVersion) {
    auto Mod = std::make_unique<AST::Module>();
    Mod->getMagic() = WasmMagic;
    Mod->getVersion() = ModuleVersion;
    return *Mod;
  } else if (*Ver == ComponentVersion) {
    auto Comp = std::make_unique<AST::Component>();
    Comp->getMagic() = WasmMagic;
    Comp->getVersion() = {0x0a, 0x00};
    Comp->getLayer() = {0x01, 0x00};
    return *Comp;
  } else {
    return logLoadError(ErrCode::Value::MalformedVersion, FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
}

} // namespace Loader
} // namespace WasmEdge
