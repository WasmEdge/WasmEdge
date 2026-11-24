// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "loader/loader.h"

#include "aot/version.h"
#ifdef WASMEDGE_BUILD_WAT
#include "wat/parser.h"
#endif

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <limits>
#include <memory>
#include <system_error>
#include <utility>
#include <variant>

using namespace std::literals;

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

// Parse module or component from file path. See "include/loader/loader.h".
Expect<std::variant<std::unique_ptr<AST::Component::Component>,
                    std::unique_ptr<AST::Module>>>
Loader::parseWasmUnit(const std::filesystem::path &FilePath) {
  std::lock_guard Lock(Mutex);

  // Set path and check the header.
  EXPECTED_TRY(FMgr.setPath(FilePath).map_error([&FilePath](auto E) {
    spdlog::error(E);
    spdlog::error(ErrInfo::InfoFile(FilePath));
    return E;
  }));

  auto ReportError = [&FilePath](auto E) {
    spdlog::error(ErrInfo::InfoFile(FilePath));
    return E;
  };

  // Find WAT text in the mapped bytes. A full read only for this test is not
  // necessary, because maybeWAT reads only the leading whitespace and comments.
  // The text format loader is experimental, so it is on only by request.
  // Without it, the text goes to the binary loader.
  if (isWatInput(FMgr.getSpan())) {
    // Reset the file manager after the conversion, because the parser reads
    // the mapped file. A later parse then starts from a clean state.
    auto Res = parseWatUnit(FMgr.getSpan()).map_error(ReportError);
    FMgr.reset();
    EXPECTED_TRY(auto Mod, std::move(Res));
    return Mod;
  }

  switch (FMgr.getHeaderType()) {
  // Filter out the Windows .dll, macOS .dylib, or Linux .so AOT compiled
  // shared-library-WASM.
  case FileMgr::FileHeader::ELF:
  case FileMgr::FileHeader::DLL:
  case FileMgr::FileHeader::MachO_32:
  case FileMgr::FileHeader::MachO_64: {
    if (Conf.getRuntimeConfigure().getRunMode() != RunMode::AOT) {
      spdlog::error("Might an invalid wasm file"sv);
      spdlog::error(ErrCode::Value::MalformedMagic);
      spdlog::error(
          "    The AOT compiled WASM shared library is only loadable in the "
          "AOT run mode. Please use a normal WASM file, or set the run mode "
          "to AOT."sv);
      spdlog::error(ErrInfo::InfoFile(FilePath));
      return Unexpect(ErrCode::Value::MalformedMagic);
    }

    WASMType = InputType::SharedLibrary;
    FMgr.reset();
    std::shared_ptr<SharedLibrary> Library = std::make_shared<SharedLibrary>();
    EXPECTED_TRY(Library->load(FilePath).map_error(ReportError));
    EXPECTED_TRY(auto Version, Library->getVersion().map_error(ReportError));
    if (Version != AOT::kBinaryVersion) {
      spdlog::error(ErrInfo::InfoMismatch(AOT::kBinaryVersion, Version));
      spdlog::error(ErrInfo::InfoFile(FilePath));
      return Unexpect(ErrCode::Value::MalformedVersion);
    }

    EXPECTED_TRY(auto Code, Library->getWasm().map_error(ReportError));

    // Keep the library alive and load executable symbols.
    EXPECTED_TRY(FMgr.setCode(Code).map_error(ReportError));
    EXPECTED_TRY(auto Unit, loadUnit().map_error(ReportError));
    if (auto Ptr = std::get_if<std::unique_ptr<AST::Module>>(&Unit);
        likely(!!Ptr)) {
      EXPECTED_TRY(loadExecutable(**Ptr, Library).map_error(ReportError));
    } else {
      spdlog::error("Component Module is not supported in AOT."sv);
      spdlog::error(ErrInfo::InfoFile(FilePath));
      return Unexpect(ErrCode::Value::IllegalGrammar);
    }
    return Unit;
  }
  default: {
    // Universal WASM, WASM, or other cases. Load and parse the module directly.
    // The intrinsics-table pointer (if any executable is loaded) is patched
    // inside loadExecutable, so no symbol manipulation is needed here.
    WASMType = InputType::WASM;
    return loadUnit().map_error(ReportError);
  }
  }
}

// Find if Code is WAT text that the loader reads. The text format loader is
// experimental, so it is on only by request.
// A build without the WAT loader reads only the binary format, also with the
// EnableWAT flag.
bool Loader::isWatInput([[maybe_unused]] Span<const uint8_t> Code) const {
#ifdef WASMEDGE_BUILD_WAT
  return EnableWAT && WAT::maybeWAT(Code);
#else
  return false;
#endif
}

// Convert the WAT text Code into a module.
Expect<std::unique_ptr<AST::Module>>
Loader::parseWatUnit([[maybe_unused]] Span<const uint8_t> Code) {
#ifdef WASMEDGE_BUILD_WAT
  // The text is a WASM module, so the input type is WASM.
  WASMType = InputType::WASM;
  EXPECTED_TRY(auto Mod, WAT::parseWat(Code, Conf).map_error([](auto E) {
    spdlog::error(E);
    return E;
  }));
  return std::make_unique<AST::Module>(std::move(Mod));
#else
  return Unexpect(ErrCode::Value::MalformedMagic);
#endif
}

// Parse module or component from byte code. See "include/loader/loader.h".
Expect<std::variant<std::unique_ptr<AST::Component::Component>,
                    std::unique_ptr<AST::Module>>>
Loader::parseWasmUnit(Span<const uint8_t> Code) {
  std::lock_guard Lock(Mutex);

  // Find if the buffer holds WAT text. The text format loader is
  // experimental, so it is on only by request.
  if (isWatInput(Code)) {
    // Drop the state of an earlier parse, which can still hold a mapped
    // file.
    FMgr.reset();
    EXPECTED_TRY(auto Mod, parseWatUnit(Code));
    return Mod;
  }
  return parseBinaryUnit(Code);
}

// Parse a binary module or component from byte code. See
// "include/loader/loader.h".
Expect<std::variant<std::unique_ptr<AST::Component::Component>,
                    std::unique_ptr<AST::Module>>>
Loader::parseBinaryUnit(Span<const uint8_t> Code) {
  EXPECTED_TRY(FMgr.setCode(Code));
  switch (FMgr.getHeaderType()) {
  // Filter out the Windows .dll, macOS .dylib, or Linux .so AOT compiled
  // shared-library-WASM.
  case FileMgr::FileHeader::ELF:
  case FileMgr::FileHeader::DLL:
  case FileMgr::FileHeader::MachO_32:
  case FileMgr::FileHeader::MachO_64:
    spdlog::error("Might an invalid wasm file"sv);
    spdlog::error(ErrCode::Value::MalformedMagic);
    spdlog::error(
        "    The AOT compiled WASM shared library is not supported for loading "
        "from memory. Please use a normal WASM file, or load the AOT compiled "
        "WASM shared library from file in the AOT run mode."sv);
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
  EXPECTED_TRY(auto ComponentOrModule, parseWasmUnit(FilePath));
  if (auto M = std::get_if<std::unique_ptr<AST::Module>>(&ComponentOrModule)) {
    return std::move(*M);
  }
  return Unexpect(ErrCode::Value::MalformedVersion);
}

// Parse module from byte code. See "include/loader/loader.h".
Expect<std::unique_ptr<AST::Module>>
Loader::parseModule(Span<const uint8_t> Code) {
  EXPECTED_TRY(auto ComponentOrModule, parseWasmUnit(Code));
  if (auto M = std::get_if<std::unique_ptr<AST::Module>>(&ComponentOrModule)) {
    return std::move(*M);
  }
  return Unexpect(ErrCode::Value::MalformedVersion);
}

// Load module or component unit. See "include/loader/loader.h".
Expect<std::variant<std::unique_ptr<AST::Component::Component>,
                    std::unique_ptr<AST::Module>>>
Loader::loadUnit() {
  EXPECTED_TRY(auto Preamble, loadPreamble());
  auto &[WasmMagic, Ver] = Preamble;
  if (Ver == ModuleVersion) {
    auto Mod = std::make_unique<AST::Module>();
    Mod->getMagic() = WasmMagic;
    Mod->getVersion() = Ver;
    if (Conf.getRuntimeConfigure().getRunMode() == RunMode::AOT) {
      EXPECTED_TRY(loadModuleAOT(Mod->getAOTSection()));
    }
    // Seek to the position after the binary header.
    FMgr.seek(8);
    EXPECTED_TRY(loadModule(*Mod));

    // Load library from AOT Section for the universal WASM case only when the
    // configured run mode is AOT.
    if (Conf.getRuntimeConfigure().getRunMode() == RunMode::AOT) {
      if (WASMType == InputType::UniversalWASM) {
        EXPECTED_TRY(loadUniversalWASM(*Mod));
      } else if (WASMType == InputType::WASM) {
        // AOT requested on a plain .wasm with no AOT artifact.
        spdlog::warn("AOT was requested but the input has no AOT artifact, "
                     "falling back to interpreter."sv);
      }
    }
    return Mod;
  } else if (Ver == ComponentVersion) {
    if (!Conf.hasProposal(Proposal::Component)) {
      return logNeedProposal(ErrCode::Value::MalformedVersion,
                             Proposal::Component, FMgr.getLastOffset(),
                             ASTNodeAttr::Component);
    }
    spdlog::warn("component model is an experimental proposal"sv);
    auto Comp = std::make_unique<AST::Component::Component>();
    Comp->getMagic() = WasmMagic;
    Comp->getVersion() = {Ver[0], Ver[1]};
    Comp->getLayer() = {Ver[2], Ver[3]};
    EXPECTED_TRY(loadComponent(*Comp));
    return Comp;
  } else {
    return logLoadError(ErrCode::Value::MalformedVersion, FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
}

// Serialize module into byte code. See "include/loader/loader.h".
Expect<std::vector<Byte>> Loader::serializeModule(const AST::Module &Mod) {
  return Ser.serializeModule(Mod);
}

// Parse an input of the AOT compiler. See "include/loader/loader.h".
Expect<Loader::CompileInput>
Loader::parseCompileInput(Span<const uint8_t> Code) {
  CompileInput In;
  In.Code = Code;
  In.IsWat = isWatInput(Code);
  std::lock_guard Lock(Mutex);
  if (In.IsWat) {
    // Drop the state of an earlier parse, which can still hold a mapped
    // file.
    FMgr.reset();
    EXPECTED_TRY(In.Module, parseWatUnit(Code));
    return In;
  }
  // The format is known, so the binary parse does not test it again.
  EXPECTED_TRY(auto Unit, parseBinaryUnit(Code));
  auto *Mod = std::get_if<std::unique_ptr<AST::Module>>(&Unit);
  if (Mod == nullptr) {
    return Unexpect(ErrCode::Value::MalformedVersion);
  }
  In.Module = std::move(*Mod);
  return In;
}

// Give the binary of an input of the AOT compiler. See
// "include/loader/loader.h".
Expect<Span<const Byte>> Loader::compileBinary(CompileInput &In) {
  if (!In.IsWat) {
    return In.Code;
  }
  if (In.Serialized.empty()) {
    EXPECTED_TRY(In.Serialized, Ser.serializeModule(*In.Module));
  }
  return Span<const Byte>(In.Serialized);
}

} // namespace Loader
} // namespace WasmEdge
