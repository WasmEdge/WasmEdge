// SPDX-License-Identifier: Apache-2.0
#include "loader/loader.h"

namespace SSVM {
namespace Loader {

/// Parse module from file path. See "include/loader/loader.h".
Expect<std::unique_ptr<AST::Module>>
Loader::parseModule(const std::string &FilePath) {
  auto Mod = std::make_unique<AST::Module>();
  if (auto Res = FSMgr.setPath(FilePath); !Res) {
    return Unexpect(Res);
  }
  if (auto Res = Mod->loadBinary(FSMgr)) {
    return std::move(Mod);
  } else {
    return Unexpect(Res);
  }
}

/// Parse module from byte code. See "include/loader/loader.h".
Expect<std::unique_ptr<AST::Module>>
Loader::parseModule(const std::vector<uint8_t> &Code) {
  auto Mod = std::make_unique<AST::Module>();
  if (auto Res = FVMgr.setCode(Code); !Res) {
    return Unexpect(Res);
  }
  if (auto Res = Mod->loadBinary(FVMgr)) {
    return std::move(Mod);
  } else {
    return Unexpect(Res);
  }
}

} // namespace Loader
} // namespace SSVM
