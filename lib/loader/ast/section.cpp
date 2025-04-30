// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

#include "aot/version.h"
#include "common/defines.h"
#include "spdlog/spdlog.h"
#include <cstdint>
#include <tuple>
#include <utility>

using namespace std::literals;

namespace WasmEdge {
namespace Loader {

// Load content of custom section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::CustomSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() -> Expect<void> {
    // Read name.
    auto StartOffset = FMgr.getOffset();
    if (auto Res = FMgr.readName()) {
      Sec.setName(*Res);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Sec_Custom);
    }
    auto ReadSize = FMgr.getOffset() - StartOffset;
    // Read remain bytes. Check is overread or not first.
    if (Sec.getContentSize() < ReadSize) {
      return logLoadError(ErrCode::Value::UnexpectedEnd, FMgr.getLastOffset(),
                          ASTNodeAttr::Sec_Custom);
    }
    if (auto Res = FMgr.readBytes(Sec.getContentSize() - ReadSize)) {
      Sec.getContent().insert(Sec.getContent().end(), (*Res).begin(),
                              (*Res).end());
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Sec_Custom);
    }
    return {};
  });
}

// Load vector of type section. See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::TypeSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() -> Expect<void> {
    // Read the recursive type vector size.
    uint32_t VecCnt = 0;
    if (auto Res = loadVecCnt()) {
      VecCnt = *Res;
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Sec_Type);
    }
    // Read the recursive types.
    Sec.getContent().clear();
    uint32_t SubTypeCnt = 0;
    for (uint32_t I = 0; I < VecCnt; I++) {
      if (auto CodeByte = FMgr.peekByte()) {
        TypeCode Code = static_cast<TypeCode>(*CodeByte);
        if (Code == TypeCode::Rec) {
          // Case: 0x4E vec(subtype).
          FMgr.readByte();
          uint32_t RecVecCnt = 0;
          if (auto Res = loadVecCnt()) {
            RecVecCnt = *Res;
          } else {
            return logLoadError(Res.error(), FMgr.getLastOffset(),
                                ASTNodeAttr::Sec_Type);
          }
          for (uint32_t J = 0; J < RecVecCnt; ++J) {
            Sec.getContent().emplace_back();
            EXPECTED_TRY(
                loadType(Sec.getContent().back()).map_error([](auto E) {
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
      } else {
        return logLoadError(CodeByte.error(), FMgr.getLastOffset(),
                            ASTNodeAttr::Sec_Type);
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
    return loadSectionContentVec(Sec, [this](uint32_t &FuncIdx) {
      return FMgr.readU32()
          .map_error([this](auto E) {
            spdlog::error(E);
            spdlog::error(ErrInfo::InfoLoading(FMgr.getLastOffset()));
            return E;
          })
          .and_then([&FuncIdx](auto Idx) {
            FuncIdx = Idx;
            return Expect<void>{};
          });
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
    if (auto Res = FMgr.readU32()) {
      Sec.setContent(*Res);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Sec_Start);
    }
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
    if (auto Res = FMgr.readU32()) {
      Sec.setContent(*Res);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Sec_DataCount);
    }
    return {};
  });
}

Expect<void> Loader::loadSection(AST::TagSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::TagType &TgType) { return loadType(TgType); });
  });
}

Expect<void> Loader::loadSection(AST::Component::ComponentSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() -> Expect<void> {
    auto ReportError = [](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
      return E;
    };
    auto ExpectedSize = Sec.getContentSize();
    auto StartOffset = FMgr.getOffset();

    EXPECTED_TRY(auto Preamble, Loader::loadPreamble().map_error(ReportError));
    auto &[WasmMagic, Ver] = Preamble;
    if (unlikely(Ver != ComponentVersion)) {
      return logLoadError(ErrCode::Value::MalformedVersion,
                          FMgr.getLastOffset(), ASTNodeAttr::Component);
    }
    auto NestedComp = std::make_shared<AST::Component::Component>();
    NestedComp->getMagic() = WasmMagic;
    NestedComp->getVersion() = {Ver[0], Ver[1]};
    NestedComp->getLayer() = {Ver[2], Ver[3]};

    auto Offset = FMgr.getOffset();

    if (unlikely(ExpectedSize < Offset - StartOffset)) {
      return logLoadError(ErrCode::Value::UnexpectedEnd, FMgr.getLastOffset(),
                          ASTNodeAttr::Component);
    }

    EXPECTED_TRY(
        loadComponent(*NestedComp, ExpectedSize - (Offset - StartOffset))
            .map_error(ReportError));

    Sec.getContent() = std::move(NestedComp);
    return {};
  });
}

Expect<void> Loader::loadSection(AST::CoreModuleSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() -> Expect<void> {
    auto ReportError = [](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      return E;
    };
    auto ExpectedSize = Sec.getContentSize();
    auto StartOffset = FMgr.getOffset();

    EXPECTED_TRY(auto Preamble, Loader::loadPreamble().map_error(ReportError));
    auto &[WasmMagic, Ver] = Preamble;
    if (unlikely(Ver != ModuleVersion)) {
      return logLoadError(ErrCode::Value::MalformedVersion,
                          FMgr.getLastOffset(), ASTNodeAttr::Module);
    }
    AST::Module CoreMod;
    CoreMod.getMagic() = WasmMagic;
    CoreMod.getVersion() = Ver;

    auto Offset = FMgr.getOffset();

    if (unlikely(ExpectedSize < Offset - StartOffset)) {
      return logLoadError(ErrCode::Value::UnexpectedEnd, FMgr.getLastOffset(),
                          ASTNodeAttr::Module);
    }

    EXPECTED_TRY(
        loadModuleInBound(CoreMod, ExpectedSize - (Offset - StartOffset))
            .map_error(ReportError));

    Sec.getContent() = std::move(CoreMod);
    return {};
  });
}

// Load vector of component alias section.
// See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::Component::AliasSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::Component::Alias &Alias) { return loadAlias(Alias); });
  });
}

// Load vector of component core:instance section.
// See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::Component::CoreInstanceSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::Component::CoreInstanceExpr &InstanceExpr) {
          return loadCoreInstance(InstanceExpr);
        });
  });
}

// Load vector of core type section.
// See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::Component::CoreTypeSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::Component::CoreDefType &Ty) { return loadType(Ty); });
  });
}

// Load vector of component type section.
// See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::Component::TypeSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::Component::DefType &Ty) { return loadType(Ty); });
  });
}

Expect<void> Loader::loadSection(AST::Component::StartSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() -> Expect<void> {
    return loadStart(Sec.getContent());
  });
}

Expect<void> Loader::loadSection(AST::Component::CanonSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::Component::Canon &C) { return loadCanonical(C); });
  });
}

Expect<void> Loader::loadSection(AST::Component::ImportSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::Component::Import &C) { return loadImport(C); });
  });
}
Expect<void> Loader::loadSection(AST::Component::ExportSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::Component::Export &C) { return loadExport(C); });
  });
}

// Load vector of component instance section.
// See "include/loader/loader.h".
Expect<void> Loader::loadSection(AST::Component::InstanceSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::Component::InstanceExpr &InstanceExpr) {
          return loadInstance(InstanceExpr);
        });
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
