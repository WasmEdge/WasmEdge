// SPDX-License-Identifier: Apache-2.0
#include "loader/loader.h"
#include "common/version.h"
#include "support/filesystem.h"
#include "support/log.h"

#include <string_view>

namespace SSVM {
namespace Loader {

namespace {
static bool endsWith(std::string_view String, std::string_view Suffix) {
  std::string_view View(String.data(), String.size());
  return View.size() >= Suffix.size() &&
         View.compare(View.size() - Suffix.size(), std::string_view::npos,
                      Suffix) == 0;
}
} // namespace

/// Load data from file path. See "include/loader/loader.h".
Expect<std::vector<Byte>> Loader::loadFile(std::string_view FilePath) {
  std::ifstream Fin(std::filesystem::u8path(FilePath),
                    std::ios::in | std::ios::binary);
  if (!Fin) {
    Log::loggingError(ErrCode::InvalidPath);
    return Unexpect(ErrCode::InvalidPath);
  }

  Fin.seekg(0, std::ios::end);
  const size_t Size = Fin.tellg();
  Fin.seekg(0, std::ios::beg);

  std::vector<Byte> Buf(Size);
  Fin.read(reinterpret_cast<char *>(Buf.data()), Size);
  if (Fin.gcount() != Size) {
    if (Fin.eof()) {
      Log::loggingError(ErrCode::EndOfFile);
      return Unexpect(ErrCode::EndOfFile);
    } else {
      Log::loggingError(ErrCode::ReadError);
      return Unexpect(ErrCode::ReadError);
    }
  }
  return std::move(Buf);
}

/// Parse module from file path. See "include/loader/loader.h".
Expect<std::unique_ptr<AST::Module>>
Loader::parseModule(std::string_view FilePath) {
  using namespace std::literals::string_view_literals;
  if (endsWith(FilePath, ".so"sv)) {
    if (auto Res = LMgr.setPath(FilePath); !Res) {
      Log::loggingError(Res.error());
      return Unexpect(Res);
    }
    if (auto Res = LMgr.getVersion()) {
      if (*Res != kVersion) {
        Log::loggingError(ErrCode::InvalidVersion);
        return Unexpect(ErrCode::InvalidVersion);
      }
    } else {
      Log::loggingError(Res.error());
      return Unexpect(Res);
    }

    std::unique_ptr<AST::Module> Mod;
    if (auto Code = LMgr.getWasm()) {
      if (auto Res = parseModule(*Code)) {
        Mod = std::move(*Res);
      } else {
        Log::loggingError(Res.error());
        return Unexpect(Res);
      }
    } else {
      Log::loggingError(Code.error());
      return Unexpect(Code);
    }
    if (auto Res = Mod->loadCompiled(LMgr)) {
      Mod->setCtor(
          reinterpret_cast<AST::Module::Ctor>(LMgr.getRawSymbol("ctor")));
      return Mod;
    } else {
      Log::loggingError(Res.error());
      return Unexpect(Res);
    }
  } else {
    auto Mod = std::make_unique<AST::Module>();
    if (auto Res = FSMgr.setPath(FilePath); !Res) {
      Log::loggingError(Res.error());
      return Unexpect(Res);
    }
    if (auto Res = Mod->loadBinary(FSMgr)) {
      return std::move(Mod);
    } else {
      Log::loggingError(Res.error());
      return Unexpect(Res);
    }
  }
}

/// Parse module from byte code. See "include/loader/loader.h".
Expect<std::unique_ptr<AST::Module>>
Loader::parseModule(Span<const uint8_t> Code) {
  auto Mod = std::make_unique<AST::Module>();
  if (auto Res = FVMgr.setCode(Code); !Res) {
    Log::loggingError(Res.error());
    return Unexpect(Res);
  }
  if (auto Res = Mod->loadBinary(FVMgr)) {
    return std::move(Mod);
  } else {
    Log::loggingError(Res.error());
    return Unexpect(Res);
  }
}

} // namespace Loader
} // namespace SSVM
