// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "aot/version.h"
#include "common/defines.h"
#include "loader/loader.h"

#include <tuple>
#include <utility>

using namespace std::literals;

namespace WasmEdge {
namespace Loader {

// Load content of custom section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::CustomSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() -> Expect<void> {
    auto ReportError = [this](auto E) {
      return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Sec_Custom);
    };

    // Read name.
    auto StartOffset = FMgr.getOffset();
    EXPECTED_TRY(std::string Name, FMgr.readName().map_error(ReportError));
    Sec.setName(Name);
    auto ReadSize = FMgr.getOffset() - StartOffset;

    // Read remain bytes. Check is overread or not first.
    if (unlikely(Sec.getContentSize() < ReadSize)) {
      return logLoadError(ErrCode::Value::UnexpectedEnd, FMgr.getLastOffset(),
                          ASTNodeAttr::Sec_Custom);
    }
    EXPECTED_TRY(
        std::vector<uint8_t> Bytes,
        FMgr.readBytes(Sec.getContentSize() - ReadSize).map_error(ReportError));
    Sec.getContent().insert(Sec.getContent().end(), Bytes.begin(), Bytes.end());
    return {};
  });
}

// Load vector of type section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::TypeSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() -> Expect<void> {
    auto ReportError = [this](auto E) {
      return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Sec_Type);
    };

    // Read the recursive type vector size.
    EXPECTED_TRY(uint32_t VecCnt, loadVecCnt().map_error(ReportError));
    // Read the recursive types.
    Sec.getContent().clear();
    uint32_t SubTypeCnt = 0;
    for (uint32_t I = 0; I < VecCnt; I++) {
      EXPECTED_TRY(uint8_t CodeByte, FMgr.peekByte().map_error(ReportError));

      TypeCode Code = static_cast<TypeCode>(CodeByte);
      if (!Conf.hasProposal(Proposal::GC) && Code != TypeCode::Func) {
        return logNeedProposal(ErrCode::Value::IntegerTooLong, Proposal::GC,
                               FMgr.getOffset(), ASTNodeAttr::Sec_Type);
      }
      if (Code == TypeCode::Rec) {
        // Case: 0x4E vec(subtype).
        FMgr.readByte();
        EXPECTED_TRY(uint32_t RecVecCnt, loadVecCnt().map_error(ReportError));
        for (uint32_t J = 0; J < RecVecCnt; ++J) {
          Sec.getContent().emplace_back();
          EXPECTED_TRY(loadType(Sec.getContent().back()).map_error([](auto E) {
            spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Type));
            return E;
          }));
          Sec.getContent().back().setRecursiveInfo(J, RecVecCnt);
          Sec.getContent().back().setTypeIndex(SubTypeCnt);
          SubTypeCnt++;
        }
      } else {
        // Case: subtype.
        Sec.getContent().emplace_back();
        Sec.getContent().back().setTypeIndex(SubTypeCnt);
        SubTypeCnt++;
        EXPECTED_TRY(loadType(Sec.getContent().back()).map_error([](auto E) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Type));
          return E;
        }));
      }
    }
    return {};
  });
}

// Load vector of import section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::ImportSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::ImportDesc &ImpDesc) { return loadDesc(ImpDesc); });
  });
}

// Load vector of function section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::FunctionSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](uint32_t &FuncIdx) -> Expect<void> {
          EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error([this](auto E) {
            return logLoadError(E, FMgr.getLastOffset(),
                                ASTNodeAttr::Sec_Function);
          }));
          FuncIdx = Idx;
          return {};
        });
  });
}

// Load vector of table section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::TableSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::TableSegment &TabSeg) { return loadSegment(TabSeg); });
  });
}

// Load vector of memory section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::MemorySection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::MemoryType &MemType) { return loadType(MemType); });
  });
}

// Load vector of global section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::GlobalSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(Sec, [this](AST::GlobalSegment &GlobSeg) {
      return loadSegment(GlobSeg);
    });
  });
}

// Load vector of export section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::ExportSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::ExportDesc &ExpDesc) { return loadDesc(ExpDesc); });
  });
}

// Load start function index. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::StartSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() -> Expect<void> {
    // Read u32 of start function index.
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error([this](auto E) {
      return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Sec_Start);
    }));
    Sec.setContent(Idx);
    return {};
  });
}

// Load vector of element section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::ElementSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(Sec, [this](AST::ElementSegment &ElemSeg) {
      return loadSegment(ElemSeg);
    });
  });
}

// Load vector of code section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::CodeSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(Sec, [this](AST::CodeSegment &CodeSeg) {
      return loadSegment(CodeSeg);
    });
  });
}

// Load vector of data section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::DataSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(Sec, [this](AST::DataSegment &DataSeg) {
      return loadSegment(DataSeg);
    });
  });
}

// Load content of data count section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::DataCountSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() -> Expect<void> {
    // Read u32 of data count.
    EXPECTED_TRY(uint32_t Cnt, FMgr.readU32().map_error([this](auto E) {
      return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Sec_DataCount);
    }));
    Sec.setContent(Cnt);
    return {};
  });
}

// Load content of tag section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::TagSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::TagType &TgType) { return loadType(TgType); });
  });
}

namespace {

inline constexpr uint32_t HostVersion() noexcept {
  return WasmEdge::AOT::kBinaryVersion;
}

inline constexpr uint8_t HostOSType() noexcept {
#if WASMEDGE_OS_LINUX
  return UINT8_C(1);
#elif WASMEDGE_OS_MACOS
  return UINT8_C(2);
#elif WASMEDGE_OS_WINDOWS
  return UINT8_C(3);
#else
  // Means WasmEdge is not yet supported on this OS.
  return UINT8_C(-1);
#endif
}

inline constexpr uint8_t HostArchType() noexcept {
#if defined(__x86_64__) || defined(_M_X64)
  return UINT8_C(1);
#elif defined(__aarch64__)
  return UINT8_C(2);
#elif defined(__riscv) && __riscv_xlen == 64
  return UINT8_C(3);
#elif defined(__arm__) && __ARM_ARCH == 7
  return UINT8_C(4);
#elif defined(__s390x__)
  return UINT8_C(5);
#else
  // Means universal wasm binary is not yet supported on this arch.
  return UINT8_C(-1);
#endif
}

} // namespace

// If there is any loader error occurs in the loadSection, then fallback
// to the interpreter mode with info level log.
Expect<void> Loader::loadSection(FileMgr &VecMgr, AST::AOTSection &Sec) {
  EXPECTED_TRY(auto Version, VecMgr.readU32().map_error([](auto E) {
    spdlog::error(E);
    spdlog::error("    AOT binary version read error:{}"sv, E);
    return E;
  }));
  Sec.setVersion(Version);
  if (unlikely(Sec.getVersion() != HostVersion())) {
    spdlog::error(ErrCode::Value::MalformedSection);
    spdlog::error("    AOT binary version unmatched."sv);
    return Unexpect(ErrCode::Value::MalformedSection);
  }

  EXPECTED_TRY(auto OSType, VecMgr.readByte().map_error([](auto E) {
    spdlog::error(E);
    spdlog::error("    AOT os type read error:{}"sv, E);
    return E;
  }));
  Sec.setOSType(OSType);
  if (unlikely(Sec.getOSType() != HostOSType())) {
    spdlog::error(ErrCode::Value::MalformedSection);
    spdlog::error("    AOT OS type unmatched."sv);
    return Unexpect(ErrCode::Value::MalformedSection);
  }

  EXPECTED_TRY(auto ArchType, VecMgr.readByte().map_error([](auto E) {
    spdlog::error(E);
    spdlog::error("    AOT arch type read error:{}"sv, E);
    return E;
  }));
  Sec.setArchType(ArchType);
  if (unlikely(Sec.getArchType() != HostArchType())) {
    spdlog::error(ErrCode::Value::MalformedSection);
    spdlog::error("    AOT arch type unmatched."sv);
    return Unexpect(ErrCode::Value::MalformedSection);
  }

  EXPECTED_TRY(auto VersionAddress, VecMgr.readU64().map_error([](auto E) {
    spdlog::error(E);
    spdlog::error("    AOT version address read error:{}"sv, E);
    return E;
  }));
  Sec.setVersionAddress(VersionAddress);

  EXPECTED_TRY(auto IntrinsicsAddress, VecMgr.readU64().map_error([](auto E) {
    spdlog::error(E);
    spdlog::error("    AOT intrinsics address read error:{}"sv, E);
    return E;
  }));
  Sec.setIntrinsicsAddress(IntrinsicsAddress);

  EXPECTED_TRY(auto TypesSize, VecMgr.readU64().map_error([](auto E) {
    spdlog::error(E);
    spdlog::error("    AOT types size read error:{}"sv, E);
    return E;
  }));
  if (TypesSize > VecMgr.getRemainSize()) {
    spdlog::error(ErrCode::Value::IntegerTooLong);
    spdlog::error("    AOT types size too large"sv);
    return Unexpect(ErrCode::Value::IntegerTooLong);
  }
  Sec.getTypesAddress().resize(TypesSize);

  for (size_t I = 0; I < Sec.getTypesAddress().size(); ++I) {
    EXPECTED_TRY(auto TypesAddress, VecMgr.readU64().map_error([](auto E) {
      spdlog::error(E);
      spdlog::error("    AOT type address read error:{}"sv, E);
      return E;
    }));
    Sec.getTypesAddress()[I] = TypesAddress;
  }

  EXPECTED_TRY(auto CodesSize, VecMgr.readU64().map_error([](auto E) {
    spdlog::error(E);
    spdlog::error("    AOT code size read error:{}"sv, E);
    return E;
  }));
  if (CodesSize > VecMgr.getRemainSize()) {
    spdlog::error(ErrCode::Value::IntegerTooLong);
    spdlog::error("    AOT code size too large"sv);
    return Unexpect(ErrCode::Value::IntegerTooLong);
  }
  Sec.getCodesAddress().resize(CodesSize);

  for (size_t I = 0; I < Sec.getCodesAddress().size(); ++I) {
    EXPECTED_TRY(auto CodesAddress, VecMgr.readU64().map_error([](auto E) {
      spdlog::error(E);
      spdlog::error("    AOT code address read error:{}"sv, E);
      return E;
    }));
    Sec.getCodesAddress()[I] = CodesAddress;
  }

  EXPECTED_TRY(auto SectionsSize, VecMgr.readU32().map_error([](auto E) {
    spdlog::error(E);
    spdlog::error("    AOT section count read error:{}"sv, E);
    return E;
  }));
  if (SectionsSize > VecMgr.getRemainSize()) {
    spdlog::error(ErrCode::Value::IntegerTooLong);
    spdlog::error("    AOT section count too large"sv);
    return Unexpect(ErrCode::Value::IntegerTooLong);
  }
  Sec.getSections().resize(SectionsSize);

  for (auto &Section : Sec.getSections()) {
    EXPECTED_TRY(std::get<0>(Section), VecMgr.readByte().map_error([](auto E) {
      spdlog::error(E);
      spdlog::error("    AOT section type read error:{}"sv, E);
      return E;
    }));
    EXPECTED_TRY(std::get<1>(Section), VecMgr.readU64().map_error([](auto E) {
      spdlog::error(E);
      spdlog::error("    AOT section offset read error:{}"sv, E);
      return E;
    }));
    EXPECTED_TRY(std::get<2>(Section), VecMgr.readU64().map_error([](auto E) {
      spdlog::error(E);
      spdlog::error("    AOT section size read error:{}"sv, E);
      return E;
    }));
    EXPECTED_TRY(uint32_t Size, VecMgr.readU32().map_error([](auto E) {
      spdlog::error(E);
      spdlog::error("    AOT section data size read error:{}"sv, E);
      return E;
    }));
    if (Size > VecMgr.getRemainSize()) {
      spdlog::error(ErrCode::Value::IntegerTooLong);
      spdlog::error("    AOT section data size is too large"sv);
      return Unexpect(ErrCode::Value::IntegerTooLong);
    }
    if (std::get<2>(Section) < Size) {
      spdlog::error(ErrCode::Value::IntegerTooLong);
      spdlog::error("    AOT section data size is larger then section size"sv);
      return Unexpect(ErrCode::Value::IntegerTooLong);
    }
    EXPECTED_TRY(auto Data, VecMgr.readBytes(Size).map_error([](auto E) {
      spdlog::error(E);
      spdlog::error("    AOT section data read error:{}"sv, E);
      return E;
    }));
    std::get<3>(Section) = std::move(Data);
  }
  return {};
}

} // namespace Loader
} // namespace WasmEdge
