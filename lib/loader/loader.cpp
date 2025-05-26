// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

#include "aot/version.h"

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
    std::shared_ptr<SharedLibrary> Library = std::make_shared<SharedLibrary>();
    EXPECTED_TRY(Library->load(FilePath).map_error(ReportError));
    EXPECTED_TRY(auto Version, Library->getVersion().map_error(ReportError));
    if (Version != AOT::kBinaryVersion) {
      spdlog::error(ErrInfo::InfoMismatch(AOT::kBinaryVersion, Version));
      spdlog::error(ErrInfo::InfoFile(FilePath));
      return Unexpect(ErrCode::Value::MalformedVersion);
    }

    EXPECTED_TRY(auto Code, Library->getWasm().map_error(ReportError));
    // Set the binary and load module.
    // Not to use parseModule() here to keep the `WASMType` value.
    EXPECTED_TRY(FMgr.setCode(Code).map_error(ReportError));
    EXPECTED_TRY(auto Unit, loadUnit().map_error(ReportError));
    if (auto Ptr = std::get_if<std::unique_ptr<AST::Module>>(&Unit);
        likely(!!Ptr)) {
      if (!Conf.getRuntimeConfigure().isForceInterpreter()) {
        // If the configure is set to force interpreter mode, not to load the
        // AOT related data.
        EXPECTED_TRY(loadExecutable(**Ptr, Library).map_error(ReportError));
      }
    } else {
      spdlog::error("Component Module is not supported in AOT."sv);
      spdlog::error(ErrInfo::InfoFile(FilePath));
      return Unexpect(ErrCode::Value::IllegalGrammar);
    }
    return Unit;
  }
  default: {
    // Universal WASM, WASM, or other cases. Load and parse the module directly.
    WASMType = InputType::WASM;
    EXPECTED_TRY(auto Unit, loadUnit().map_error(ReportError));
    if (auto Ptr = std::get_if<std::unique_ptr<AST::Module>>(&Unit);
        likely(!!Ptr)) {
      if (!Conf.getRuntimeConfigure().isForceInterpreter()) {
        // If the configure is set to force interpreter mode, not to set the
        // symbol.
        if (auto &Symbol = (*Ptr)->getSymbol()) {
          *Symbol = IntrinsicsTable;
        }
      }
    }
    return Unit;
  }
  }
}

// Parse module or component from byte code. See "include/loader/loader.h".
Expect<std::variant<std::unique_ptr<AST::Component::Component>,
                    std::unique_ptr<AST::Module>>>
Loader::parseWasmUnit(Span<const uint8_t> Code) {
  std::lock_guard Lock(Mutex);
  EXPECTED_TRY(FMgr.setCode(Code));
  switch (FMgr.getHeaderType()) {
  // Filter out the Windows .dll, MacOS .dylib, or Linux .so AOT compiled
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
    if (!Conf.getRuntimeConfigure().isForceInterpreter()) {
      EXPECTED_TRY(loadModuleAOT(Mod->getAOTSection()));
    }
    // Seek to the position after the binary header.
    FMgr.seek(8);
    EXPECTED_TRY(loadModule(*Mod));

    // Load library from AOT Section for the universal WASM case.
    // For the force interpreter mode, skip this.
    if (!Conf.getRuntimeConfigure().isForceInterpreter() &&
        WASMType == InputType::UniversalWASM) {
      EXPECTED_TRY(loadUniversalWASM(*Mod));
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
