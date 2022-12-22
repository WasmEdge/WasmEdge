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

// Parse module from file path. See "include/loader/loader.h".
Expect<std::unique_ptr<AST::Module>>
Loader::parseModule(const std::filesystem::path &FilePath) {
  using namespace std::literals::string_view_literals;
  std::lock_guard Lock(Mutex);
  // Set path and check the header.
  if (auto Res = FMgr.setPath(FilePath); !Res) {
    spdlog::error(Res.error());
    spdlog::error(ErrInfo::InfoFile(FilePath));
    return Unexpect(Res);
  }

  switch (FMgr.getHeaderType()) {
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
      if (auto Res = loadModule()) {
        Mod = std::move(*Res);
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
    return Mod;
  }
  default:
    // Universal WASM, WASM, or other cases. Load and parse the module directly.
    WASMType = InputType::WASM;
    if (auto Res = loadModule()) {
      if (!Conf.getRuntimeConfigure().isForceInterpreter()) {
        // If the configure is set to force interpreter mode, not to set the
        // symbol.
        if (auto &Symbol = (*Res)->getSymbol()) {
          *Symbol = IntrinsicsTable;
        }
      }
      return std::move(*Res);
    } else {
      spdlog::error(ErrInfo::InfoFile(FilePath));
      return Unexpect(Res);
    }
  }
}

// Parse module from byte code. See "include/loader/loader.h".
Expect<std::unique_ptr<AST::Module>>
Loader::parseModule(Span<const uint8_t> Code) {
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
  return loadModule();
}

// Helper function of checking the valid value types.
Expect<ValType> Loader::checkValTypeProposals(ValType VType, uint64_t Off,
                                              ASTNodeAttr Node) const noexcept {
  if (VType == ValType::V128 && !Conf.hasProposal(Proposal::SIMD)) {
    return logNeedProposal(ErrCode::Value::MalformedValType, Proposal::SIMD,
                           Off, Node);
  }
  if ((VType == ValType::FuncRef &&
       !Conf.hasProposal(Proposal::ReferenceTypes) &&
       !Conf.hasProposal(Proposal::BulkMemoryOperations)) ||
      (VType == ValType::ExternRef &&
       !Conf.hasProposal(Proposal::ReferenceTypes))) {
    return logNeedProposal(ErrCode::Value::MalformedElemType,
                           Proposal::ReferenceTypes, Off, Node);
  }
  switch (VType) {
  case ValType::I32:
  case ValType::I64:
  case ValType::F32:
  case ValType::F64:
  case ValType::V128:
  case ValType::ExternRef:
  case ValType::FuncRef:
    return VType;
  default:
    return logLoadError(ErrCode::Value::MalformedValType, Off, Node);
  }
}

// Helper function of checking the valid reference types.
Expect<RefType> Loader::checkRefTypeProposals(RefType RType, uint64_t Off,
                                              ASTNodeAttr Node) const noexcept {
  switch (RType) {
  case RefType::ExternRef:
    if (!Conf.hasProposal(Proposal::ReferenceTypes)) {
      return logNeedProposal(ErrCode::Value::MalformedElemType,
                             Proposal::ReferenceTypes, Off, Node);
    }
    [[fallthrough]];
  case RefType::FuncRef:
    return RType;
  default:
    if (Conf.hasProposal(Proposal::ReferenceTypes)) {
      return logLoadError(ErrCode::Value::MalformedRefType, Off, Node);
    } else {
      return logLoadError(ErrCode::Value::MalformedElemType, Off, Node);
    }
  }
}

} // namespace Loader
} // namespace WasmEdge
