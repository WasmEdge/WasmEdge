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
      return logLoadError(ErrCode::MalformedMagic, FMgr.getLastOffset(),
                          ASTNodeAttr::Component);
    }
    Comp->getMagic() = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
  // <version>
  if (auto Res = FMgr.readBytes(4)) {
    std::vector<Byte> Version = {0x0a, 0x00};
    if (*Res != Version) {
      return logLoadError(ErrCode::MalformedVersion, FMgr.getLastOffset(),
                          ASTNodeAttr::Component);
    }
    Comp->getVersion() = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
  // <layer>
  if (auto Res = FMgr.readBytes(4)) {
    std::vector<Byte> Layer = {0x01, 0x00};
    if (*Res != Layer) {
      return logLoadError(ErrCode::MalformedVersion, FMgr.getLastOffset(),
                          ASTNodeAttr::Component);
    }
    Comp->getLayer() = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
  // section   ::=    section_0(<core:custom>)         => ϵ
  //            | m*:section_1(<core:module>)         => [core-prefix(m)]
  //            | i*:section_2(vec(<core:instance>))  => core-prefix(i)*
  //            | a*:section_3(vec(<core:alias>))     => core-prefix(a)*
  //            | t*:section_4(vec(<core:type>))      => core-prefix(t)*
  //            | c: section_5(<component>)           => [c]
  //            | i*:section_6(vec(<instance>))       => i*
  //            | a*:section_7(vec(<alias>))          => a*
  //            | t*:section_8(vec(<type>))           => t*
  //            | c*:section_9(vec(<canon>))          => c*
  //            | s: section_10(<start>)              => [s]
  //            | i*:section_11(vec(<import>))        => i*
  //            | e*:section_12(vec(<export>))        => e*
  return Comp;
}

} // namespace Loader
} // namespace WasmEdge
