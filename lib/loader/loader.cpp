// SPDX-License-Identifier: Apache-2.0
#include "loader/loader.h"
#include "aot/version.h"
#include "common/filesystem.h"
#include "common/log.h"

#include <fstream>
#include <string_view>

#if WASMEDGE_OS_LINUX
#define EXTENSION ".so"sv
#elif WASMEDGE_OS_MACOS
#define EXTENSION ".dylib"sv
#elif WASMEDGE_OS_WINDOWS
#define EXTENSION ".dll"sv
#endif

namespace WasmEdge {
namespace Loader {

/// Load data from file path. See "include/loader/loader.h".
Expect<std::vector<Byte>>
Loader::loadFile(const std::filesystem::path &FilePath) {
  std::error_code EC;
  size_t FileSize = std::filesystem::file_size(FilePath, EC);
  if (EC) {
    spdlog::error(ErrCode::IllegalPath);
    spdlog::error(ErrInfo::InfoFile(FilePath));
    return Unexpect(ErrCode::IllegalPath);
  }

  std::ifstream Fin(FilePath, std::ios::in | std::ios::binary);
  if (!Fin) {
    spdlog::error(ErrCode::IllegalPath);
    spdlog::error(ErrInfo::InfoFile(FilePath));
    return Unexpect(ErrCode::IllegalPath);
  }

  std::vector<Byte> Buf(FileSize);
  size_t Index = 0;
  while (FileSize > 0) {
    const uint32_t BlockSize = static_cast<uint32_t>(
        std::min<size_t>(FileSize, std::numeric_limits<uint32_t>::max()));
    Fin.read(reinterpret_cast<char *>(Buf.data()) + Index, BlockSize);
    const uint32_t ReadCount = static_cast<uint32_t>(Fin.gcount());
    if (ReadCount != BlockSize) {
      if (Fin.eof()) {
        spdlog::error(ErrCode::UnexpectedEnd);
        spdlog::error(ErrInfo::InfoLoading(ReadCount));
        spdlog::error(ErrInfo::InfoFile(FilePath));
        return Unexpect(ErrCode::UnexpectedEnd);
      } else {
        spdlog::error(ErrCode::ReadError);
        spdlog::error(ErrInfo::InfoLoading(ReadCount));
        spdlog::error(ErrInfo::InfoFile(FilePath));
        return Unexpect(ErrCode::ReadError);
      }
    }
    Index += static_cast<size_t>(BlockSize);
    FileSize -= static_cast<size_t>(BlockSize);
  }
  return Buf;
}

/// Parse module from file path. See "include/loader/loader.h".
Expect<std::unique_ptr<AST::Module>>
Loader::parseModule(const std::filesystem::path &FilePath) {
  using namespace std::literals::string_view_literals;
  if (FilePath.extension() == EXTENSION) {
    if (auto Res = LMgr.setPath(FilePath); !Res) {
      spdlog::error(Res.error());
      spdlog::error(ErrInfo::InfoFile(FilePath));
      return Unexpect(Res);
    }
    if (auto Res = LMgr.getVersion()) {
      if (*Res != AOT::kBinaryVersion) {
        spdlog::error(ErrInfo::InfoMismatch(AOT::kBinaryVersion, *Res));
        return Unexpect(ErrCode::MalformedVersion);
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
