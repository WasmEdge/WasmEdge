// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "loader/loader.h"

#include "aot/version.h"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <limits>
#include <string_view>
#include <system_error>
#include <utility>

namespace WasmEdge {
namespace Loader {

// Load data from file path. See "include/loader/loader.h".
Expect<std::vector<Byte>>
Loader::loadFile(const std::filesystem::path &FilePath) {
  std::error_code EC;
  size_t FileSize = std::filesystem::file_size(FilePath, EC);
  if (EC) {
    spdlog::error(ErrCode::Value::IllegalPath);
    spdlog::error(ErrInfo::InfoFile(FilePath));
    return Unexpect(ErrCode::Value::IllegalPath);
  }

  std::ifstream Fin(FilePath, std::ios::in | std::ios::binary);
  if (!Fin) {
    spdlog::error(ErrCode::Value::IllegalPath);
    spdlog::error(ErrInfo::InfoFile(FilePath));
    return Unexpect(ErrCode::Value::IllegalPath);
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
        spdlog::error(ErrCode::Value::UnexpectedEnd);
        spdlog::error(ErrInfo::InfoLoading(ReadCount));
        spdlog::error(ErrInfo::InfoFile(FilePath));
        return Unexpect(ErrCode::Value::UnexpectedEnd);
      } else {
        spdlog::error(ErrCode::Value::ReadError);
        spdlog::error(ErrInfo::InfoLoading(ReadCount));
        spdlog::error(ErrInfo::InfoFile(FilePath));
        return Unexpect(ErrCode::Value::ReadError);
      }
    }
    Index += static_cast<size_t>(BlockSize);
    FileSize -= static_cast<size_t>(BlockSize);
  }
  return Buf;
}

Expect<std::variant<AST::Component::Component, AST::Module>>
Loader::parseWasmUnit(const std::filesystem::path &FilePath) {
  std::lock_guard Lock(Mutex);
  // Set path and check the header.
  if (auto Res = FMgr.setPath(FilePath); !Res) {
    spdlog::error(Res.error());
    spdlog::error(ErrInfo::InfoFile(FilePath));
    return Unexpect(Res);
  }

  switch (FMgr.getHeaderType()) {
  // Filter out the Windows .dll, MacOS .dylib, or Linux .so AOT compiled
  // shared-library-WASM.
  case FileMgr::FileHeader::ELF:
  case FileMgr::FileHeader::DLL:
  case FileMgr::FileHeader::MachO_32:
  case FileMgr::FileHeader::MachO_64: {
    // AOT compiled shared-library-WASM cases. Use ldmgr to load the module.
    WASMType = InputType::SharedLibrary;
    FMgr.reset();
    if (auto Res = LMgr.setPath(FilePath); !Res) {
      spdlog::error(ErrInfo::InfoFile(FilePath));
      return Unexpect(Res);
    }
    if (auto Res = LMgr.getVersion()) {
      if (*Res != AOT::kBinaryVersion) {
        spdlog::error(ErrInfo::InfoMismatch(AOT::kBinaryVersion, *Res));
        spdlog::error(ErrInfo::InfoFile(FilePath));
        return Unexpect(ErrCode::Value::MalformedVersion);
      }
    } else {
      spdlog::error(ErrInfo::InfoFile(FilePath));
      return Unexpect(Res);
    }

    std::unique_ptr<AST::Module> Mod;
    if (auto Code = LMgr.getWasm()) {
      // Set the binary and load module.
      // Not to use parseModule() here to keep the `WASMType` value.
      if (auto Res = FMgr.setCode(*Code); !Res) {
        spdlog::error(ErrInfo::InfoFile(FilePath));
        return Unexpect(Res);
      }
      if (auto Res = loadUnit()) {
        if (std::holds_alternative<AST::Module>(*Res)) {
          Mod = std::make_unique<AST::Module>(std::get<AST::Module>(*Res));
        }
      } else {
        spdlog::error(ErrInfo::InfoFile(FilePath));
        return Unexpect(Res);
      }
    } else {
      spdlog::error(ErrInfo::InfoFile(FilePath));
      return Unexpect(Code);
    }
    if (!Conf.getRuntimeConfigure().isForceInterpreter()) {
      // If the configure is set to force interpreter mode, not to load the AOT
      // related data.
      if (auto Res = loadCompiled(*Mod.get()); unlikely(!Res)) {
        spdlog::error(ErrInfo::InfoFile(FilePath));
        return Unexpect(Res);
      }
    }
    return *Mod.get();
  }
  default:
    // Universal WASM, WASM, or other cases. Load and parse the module directly.
    WASMType = InputType::WASM;
    auto Unit = loadUnit();
    if (!Unit) {
      spdlog::error(ErrInfo::InfoFile(FilePath));
      return Unexpect(Unit);
    }
    switch (Unit->index()) {
    case 0: // component
      return Unit;
    case 1: // module
    default: {
      auto Mod = std::get<AST::Module>(*Unit);
      if (!Conf.getRuntimeConfigure().isForceInterpreter()) {
        // If the configure is set to force interpreter mode, not to set the
        // symbol.
        if (auto &Symbol = Mod.getSymbol()) {
          *Symbol = IntrinsicsTable;
        }
      }
      return Mod;
    }
    }
  }
}

Expect<std::variant<AST::Component::Component, AST::Module>>
Loader::parseWasmUnit(Span<const uint8_t> Code) {
  std::lock_guard Lock(Mutex);
  if (auto Res = FMgr.setCode(Code); !Res) {
    return Unexpect(Res);
  }
  switch (FMgr.getHeaderType()) {
  // Filter out the Windows .dll, MacOS .dylib, or Linux .so AOT compiled
  // shared-library-WASM.
  case FileMgr::FileHeader::ELF:
  case FileMgr::FileHeader::DLL:
  case FileMgr::FileHeader::MachO_32:
  case FileMgr::FileHeader::MachO_64:
    spdlog::error(ErrCode::Value::MalformedMagic);
    spdlog::error(
        "    The AOT compiled WASM shared library is not supported for loading "
        "from memory. Please use the universal WASM binary or pure WASM, or "
        "load the AOT compiled WASM shared library from file.");
    return Unexpect(ErrCode::Value::MalformedMagic);
  default:
    break;
  }
  // For malformed header checking, handle in the module loading.
  WASMType = InputType::WASM;
  return loadUnit();
}

// Parse module from file path. See "include/loader/loader.h".
Expect<std::unique_ptr<AST::Module>>
Loader::parseModule(const std::filesystem::path &FilePath) {
  if (auto R = parseWasmUnit(FilePath)) {
    if (std::holds_alternative<AST::Module>(*R)) {
      auto Res = std::make_unique<AST::Module>(std::get<AST::Module>(*R));
      return Res;
    } else {
      return Unexpect(ErrCode::Value::MalformedVersion);
    }
  } else {
    return Unexpect(R);
  }
}

// Parse module from byte code. See "include/loader/loader.h".
Expect<std::unique_ptr<AST::Module>>
Loader::parseModule(Span<const uint8_t> Code) {
  if (auto R = parseWasmUnit(Code)) {
    if (std::holds_alternative<AST::Module>(*R)) {
      auto Res = std::make_unique<AST::Module>(std::get<AST::Module>(*R));
      return Res;
    } else {
      return Unexpect(ErrCode::Value::MalformedVersion);
    }
  } else {
    return Unexpect(R);
  }
}

// Serialize module into byte code. See "include/loader/loader.h".
Expect<std::vector<Byte>> Loader::serializeModule(const AST::Module &Mod) {
  return Ser.serializeModule(Mod);
}

} // namespace Loader
} // namespace WasmEdge
