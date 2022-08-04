// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "loader/loader.h"

#include <cstdint>
#include <utility>

namespace WasmEdge {
namespace Loader {
Expect<std::unique_ptr<AST::Component>> Loader::loadComponent() {
  auto Comp = std::make_unique<AST::Component>();
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
  return Comp;
}

} // namespace Loader
} // namespace WasmEdge
