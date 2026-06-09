// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

#include "aot/version.h"
#include "wat/parser.h"

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

  // Set path and check the header. setPath mmaps the file and getHeaderType
  // only peeks the leading bytes, so AOT shared libraries (dlopen'd from the
  // path) and binary WASM (parsed from the mmap) are handled without reading
  // the whole artifact into memory. Only a WAT text file -- which has no binary
  // magic and must be parsed from a contiguous buffer -- is read in full below.
  EXPECTED_TRY(FMgr.setPath(FilePath).map_error([&FilePath](auto E) {
    spdlog::error(E);
    spdlog::error(ErrInfo::InfoFile(FilePath));
    return E;
  }));

  auto ReportError = [&FilePath](auto E) {
    spdlog::error(ErrInfo::InfoFile(FilePath));
    return E;
  };

  switch (FMgr.getHeaderType()) {
  // Filter out the Windows .dll, macOS .dylib, or Linux .so AOT compiled
  // shared-library-WASM.
  case FileMgr::FileHeader::ELF:
  case FileMgr::FileHeader::DLL:
  case FileMgr::FileHeader::MachO_32:
  case FileMgr::FileHeader::MachO_64: {
    // Open the shared library long enough to validate its WasmEdge AOT
    // version and extract the embedded WASM bytes. Whether the native
    // handle stays alive depends on the configured RunMode.
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

    if (Conf.getRuntimeConfigure().getRunMode() != RunMode::AOT) {
      // Non-AOT mode: drop the native handle now and re-parse the embedded
      // WASM bytes as plain WASM. No AOT function symbol from the shared
      // library is resolved or called.
      Library.reset();
      return parseWasmUnit(Code);
    }

    // AOT mode: keep the library alive and load executable symbols.
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
  case FileMgr::FileHeader::Wasm: {
    // Binary or universal WASM: parse directly from the mmap'd file. The
    // intrinsics-table pointer (if any executable is loaded) is patched inside
    // loadExecutable, so no symbol manipulation is needed here.
    WASMType = InputType::WASM;
    return loadUnit().map_error(ReportError);
  }
  default:
    break;
  }

  // Unknown header: the file may be WAT text, which can begin with a UTF-8 BOM,
  // whitespace, or a comment -- none of which match a binary magic. Read it in
  // full (WAT must be parsed from a contiguous buffer) and route via maybeWAT;
  // anything that is not WAT falls back to a binary load so malformed inputs
  // still produce the usual error.
  FMgr.reset();
  EXPECTED_TRY(auto Bytes, loadFile(FilePath).map_error(ReportError));
  if (Conf.isEnableWAT() && WAT::maybeWAT(Bytes)) {
    std::string_view Source(reinterpret_cast<const char *>(Bytes.data()),
                            Bytes.size());
    EXPECTED_TRY(auto Mod,
                 WAT::parseWat(Source, Conf).map_error([&FilePath](auto E) {
                   spdlog::error(E);
                   spdlog::error(ErrInfo::InfoFile(FilePath));
                   return E;
                 }));
    // The parsed result is a WASM module; mark the Loader state accordingly
    // so subsequent calls on the same instance do not see a stale WASMType
    // left over from a previous shared-library or universal-wasm parse.
    WASMType = InputType::WASM;
    return std::make_unique<AST::Module>(std::move(Mod));
  }
  EXPECTED_TRY(FMgr.setCode(std::move(Bytes)).map_error(ReportError));
  WASMType = InputType::WASM;
  return loadUnit().map_error(ReportError);
}

// Parse module or component from byte code. See "include/loader/loader.h".
Expect<std::variant<std::unique_ptr<AST::Component::Component>,
                    std::unique_ptr<AST::Module>>>
Loader::parseWasmUnit(Span<const uint8_t> Code) {
  std::lock_guard Lock(Mutex);

  // Check if the buffer contains WAT text.
  if (Conf.isEnableWAT() && WAT::maybeWAT(Code)) {
    std::string_view Source(reinterpret_cast<const char *>(Code.data()),
                            Code.size());
    EXPECTED_TRY(auto Mod, WAT::parseWat(Source, Conf));
    // Drop any file handle/mmap held by FMgr from a prior file-path load and
    // record that the resulting module is plain WASM, matching the file-path
    // WAT fast-path in parseWasmUnit(filesystem::path).
    FMgr.reset();
    WASMType = InputType::WASM;
    return std::make_unique<AST::Module>(std::move(Mod));
  }

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
        "from memory. Please use the universal WASM binary or pure WASM, or "
        "load the AOT compiled WASM shared library from file."sv);
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

// Helper function to set the function type for tag.
void Loader::setTagFunctionType(AST::TagSection &TagSec,
                                AST::ImportSection &ImportSec,
                                AST::TypeSection &TypeSec) {
  auto &TypeVec = TypeSec.getContent();
  for (auto &TgType : TagSec.getContent()) {
    auto TypeIdx = TgType.getTypeIdx();
    // Invalid type index would be checked during validation.
    if (TypeIdx < TypeVec.size()) {
      TgType.setDefType(&TypeVec[TypeIdx]);
    }
  }
  for (auto &Desc : ImportSec.getContent()) {
    if (Desc.getExternalType() == ExternalType::Tag) {
      auto &TgType = Desc.getExternalTagType();
      auto TypeIdx = TgType.getTypeIdx();
      // Invalid type index would be checked during validation.
      if (TypeIdx < TypeVec.size()) {
        TgType.setDefType(&TypeVec[TypeIdx]);
      }
    }
  }
}

} // namespace Loader
} // namespace WasmEdge
