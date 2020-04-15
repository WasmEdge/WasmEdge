// SPDX-License-Identifier: Apache-2.0
#include "loader/loader.h"
#include "common/version.h"
#include <string_view>

namespace SSVM {
namespace Loader {

namespace {
static bool endsWith(const std::string &String, const std::string_view Suffix) {
  const std::string_view View(String.data(), String.size());
  return View.size() >= Suffix.size() &&
         View.compare(View.size() - Suffix.size(), std::string_view::npos,
                      Suffix) == 0;
}
} // namespace


/// Load data from file path. See "include/loader/loader.h".
Expect<Bytes> Loader::loadFile(const std::string &FilePath) {
  std::ifstream Fin(FilePath, std::ios::in | std::ios::binary);
  if (!Fin) {
    return Unexpect(ErrCode::InvalidPath);
  }

  Fin.seekg(0, std::ios::end);
  const size_t Size = Fin.tellg();
  Fin.seekg(0, std::ios::beg);

  Bytes Buf(Size);
  Fin.read(reinterpret_cast<char *>(Buf.data()), Size);
  if (Fin.gcount() != Size) {
    if (Fin.eof()) {
      return Unexpect(ErrCode::EndOfFile);
    } else {
      return Unexpect(ErrCode::ReadError);
    }
  }
  return std::move(Buf);
}

/// Parse module from file path. See "include/loader/loader.h".
Expect<std::unique_ptr<AST::Module>>
Loader::parseModule(const std::string &FilePath) {
  using namespace std::literals::string_view_literals;
  if (endsWith(FilePath, ".so"sv)) {
    if (auto Res = LMgr.setPath(FilePath); !Res) {
      return Unexpect(Res);
    }
    if (auto Res = LMgr.getVersion()) {
      if (*Res != kVersion) {
        return Unexpect(ErrCode::InvalidVersion);
      }
    } else {
      return Unexpect(Res);
    }

    std::unique_ptr<AST::Module> Mod;
    if (auto Code = LMgr.getWasm()) {
      if (auto Res = parseModule(*Code)) {
        Mod = std::move(*Res);
      } else {
        return Unexpect(Res);
      }
    } else {
      return Unexpect(Code);
    }
    if (auto Res = Mod->loadCompiled(LMgr)) {
      Mod->setCtor(
          reinterpret_cast<AST::Module::Ctor>(LMgr.getRawSymbol("ctor")));
      return Mod;
    } else {
      return Unexpect(Res);
    }
  } else {
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
