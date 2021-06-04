// SPDX-License-Identifier: Apache-2.0
#include "loader/loader.h"
#include "aot/version.h"
#include "common/filesystem.h"
#include "common/log.h"

#include <fstream>
#include <string_view>

namespace WasmEdge {
namespace Loader {

/// Load data from file path. See "include/loader/loader.h".
Expect<std::vector<Byte>>
Loader::loadFile(const std::filesystem::path &FilePath) {
  std::ifstream Fin(FilePath, std::ios::in | std::ios::binary);
  if (!Fin) {
    spdlog::error(ErrCode::InvalidPath);
    spdlog::error(ErrInfo::InfoFile(FilePath));
    return Unexpect(ErrCode::InvalidPath);
  }

  Fin.seekg(0, std::ios::end);
  const size_t Size = Fin.tellg();
  Fin.seekg(0, std::ios::beg);

  std::vector<Byte> Buf(Size);
  Fin.read(reinterpret_cast<char *>(Buf.data()), Size);
  if (static_cast<size_t>(Fin.gcount()) != Size) {
    if (Fin.eof()) {
      spdlog::error(ErrCode::UnexpectedEnd);
      spdlog::error(ErrInfo::InfoLoading(Fin.gcount()));
      spdlog::error(ErrInfo::InfoFile(FilePath));
      return Unexpect(ErrCode::UnexpectedEnd);
    } else {
      spdlog::error(ErrCode::ReadError);
      spdlog::error(ErrInfo::InfoLoading(Fin.gcount()));
      spdlog::error(ErrInfo::InfoFile(FilePath));
      return Unexpect(ErrCode::ReadError);
    }
  }
  return Buf;
}

/// Parse module from file path. See "include/loader/loader.h".
Expect<std::unique_ptr<AST::Module>>
Loader::parseModule(const std::filesystem::path &FilePath) {
  using namespace std::literals::string_view_literals;
  if (FilePath.extension() == ".so"sv) {
    if (auto Res = LMgr.setPath(FilePath); !Res) {
      spdlog::error(Res.error());
      spdlog::error(ErrInfo::InfoFile(FilePath));
      return Unexpect(Res);
    }
    if (auto Res = LMgr.getVersion()) {
      if (*Res != AOT::kBinaryVersion) {
        spdlog::error(ErrInfo::InfoMismatch(AOT::kBinaryVersion, *Res));
        return Unexpect(ErrCode::InvalidVersion);
      }
    } else {
      spdlog::error(ErrInfo::InfoFile(FilePath));
      return Unexpect(Res);
    }

    std::unique_ptr<AST::Module> Mod;
    if (auto Code = LMgr.getWasm()) {
      if (auto Res = parseModule(*Code)) {
        Mod = std::move(*Res);
      } else {
        spdlog::error(ErrInfo::InfoFile(FilePath));
        return Unexpect(Res);
      }
    } else {
      spdlog::error(ErrInfo::InfoFile(FilePath));
      return Unexpect(Code);
    }
    if (auto Res = Mod->loadCompiled(LMgr)) {
      return Mod;
    } else {
      spdlog::error(ErrInfo::InfoFile(FilePath));
      return Unexpect(Res);
    }
  } else {
    auto Mod = std::make_unique<AST::Module>();
    if (auto Res = FMgr.setPath(FilePath); !Res) {
      spdlog::error(Res.error());
      spdlog::error(ErrInfo::InfoFile(FilePath));
      return Unexpect(Res);
    }
    if (auto Res = Mod->loadBinary(FMgr, Conf)) {
      return Mod;
    } else {
      spdlog::error(ErrInfo::InfoFile(FilePath));
      return Unexpect(Res);
    }
  }
}

/// Parse module from byte code. See "include/loader/loader.h".
Expect<std::unique_ptr<AST::Module>>
Loader::parseModule(Span<const uint8_t> Code) {
  auto Mod = std::make_unique<AST::Module>();
  if (auto Res = FMgr.setCode(Code); !Res) {
    return Unexpect(Res);
  }
  if (auto Res = Mod->loadBinary(FMgr, Conf)) {
    return Mod;
  } else {
    return Unexpect(Res);
  }
}

} // namespace Loader
} // namespace WasmEdge
