// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

#include "aot/version.h"
#include "common/defines.h"
#include <cstdint>
#include <tuple>
#include <utility>

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
            if (auto Res = loadType(Sec.getContent().back()); unlikely(!Res)) {
              spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Type));
              return Unexpect(Res);
            }
            Sec.getContent().back().setRecursiveInfo(J, RecVecCnt);
            Sec.getContent().back().setTypeIndex(SubTypeCnt);
            SubTypeCnt++;
          }
        } else {
          // Case: subtype.
          Sec.getContent().emplace_back();
          Sec.getContent().back().setTypeIndex(SubTypeCnt);
          SubTypeCnt++;
          if (auto Res = loadType(Sec.getContent().back()); unlikely(!Res)) {
            spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Type));
            return Unexpect(Res);
          }
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
  return loadSectionContent(Sec, [this, &Sec]() -> Expect<void> {
    return loadSectionContentVec(
        Sec, [this](uint32_t &FuncIdx) -> Expect<void> {
          if (auto Res = FMgr.readU32()) {
            FuncIdx = *Res;
          } else {
            spdlog::error(Res.error());
            spdlog::error(ErrInfo::InfoLoading(FMgr.getLastOffset()));
            return Unexpect(Res);
          }
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
  auto ResPreamble = Loader::loadPreamble();
  if (!ResPreamble) {
    return Unexpect(ResPreamble);
  }
  auto WasmMagic = ResPreamble->first;
  auto Ver = ResPreamble->second;
  if (unlikely(Ver != ComponentVersion)) {
    return logLoadError(ErrCode::Value::MalformedVersion, FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
  auto NestedComp = std::make_shared<AST::Component::Component>();
  NestedComp->getMagic() = WasmMagic;
  NestedComp->getVersion() = {Ver[0], Ver[1]};
  NestedComp->getLayer() = {Ver[2], Ver[3]};
  if (auto Res = loadComponent(*NestedComp); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return Unexpect(Res);
  }
  Sec.getContent() = NestedComp;
  return {};
}

Expect<void> Loader::loadSection(AST::CoreModuleSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() -> Expect<void> {
    auto ExpectedSize = Sec.getContentSize();
    auto StartOffset = FMgr.getOffset();
    auto ResPreamble = Loader::loadPreamble();
    if (!ResPreamble) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      return Unexpect(ResPreamble);
    }
    auto WasmMagic = ResPreamble->first;
    auto Ver = ResPreamble->second;
    if (unlikely(Ver != ModuleVersion)) {
      return logLoadError(ErrCode::Value::MalformedVersion,
                          FMgr.getLastOffset(), ASTNodeAttr::Module);
    }
    AST::Module CoreMod;
    CoreMod.getMagic() = WasmMagic;
    CoreMod.getVersion() = Ver;

    auto Offset = FMgr.getOffset();
    ExpectedSize -= (Offset - StartOffset);

    if (auto Res = loadModuleInBound(CoreMod, ExpectedSize); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      return Unexpect(Res);
    }
    Sec.getContent() = CoreMod;
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
  if (auto Res = VecMgr.readU32(); unlikely(!Res)) {
    spdlog::info(Res.error());
    spdlog::info("    AOT binary version read error:{}", Res.error());
    return Unexpect(Res);
  } else {
    Sec.setVersion(*Res);
  }
  if (unlikely(Sec.getVersion() != HostVersion())) {
    spdlog::info(ErrCode::Value::MalformedSection);
    spdlog::info("    AOT binary version unmatched.");
    return Unexpect(ErrCode::Value::MalformedSection);
  }

  if (auto Res = VecMgr.readByte(); unlikely(!Res)) {
    spdlog::info(Res.error());
    spdlog::info("    AOT os type read error:{}", Res.error());
    return Unexpect(Res);
  } else {
    Sec.setOSType(*Res);
  }
  if (unlikely(Sec.getOSType() != HostOSType())) {
    spdlog::info(ErrCode::Value::MalformedSection);
    spdlog::info("    AOT OS type unmatched.");
    return Unexpect(ErrCode::Value::MalformedSection);
  }

  if (auto Res = VecMgr.readByte(); unlikely(!Res)) {
    spdlog::info(Res.error());
    spdlog::info("    AOT arch type read error:{}", Res.error());
    return Unexpect(Res);
  } else {
    Sec.setArchType(*Res);
  }
  if (unlikely(Sec.getArchType() != HostArchType())) {
    spdlog::info(ErrCode::Value::MalformedSection);
    spdlog::info("    AOT arch type unmatched.");
    return Unexpect(ErrCode::Value::MalformedSection);
  }

  if (auto Res = VecMgr.readU64(); unlikely(!Res)) {
    spdlog::info(Res.error());
    spdlog::info("    AOT version address read error:{}", Res.error());
    return Unexpect(Res);
  } else {
    Sec.setVersionAddress(*Res);
  }
  if (auto Res = VecMgr.readU64(); unlikely(!Res)) {
    spdlog::info(Res.error());
    spdlog::info("    AOT intrinsics address read error:{}", Res.error());
    return Unexpect(Res);
  } else {
    Sec.setIntrinsicsAddress(*Res);
  }
  if (auto Res = VecMgr.readU64(); unlikely(!Res)) {
    spdlog::info(Res.error());
    spdlog::info("    AOT types size read error:{}", Res.error());
    return Unexpect(Res);
  } else {
    const uint64_t Size = *Res;
    if (Size > VecMgr.getRemainSize()) {
      spdlog::info(ErrCode::Value::IntegerTooLong);
      spdlog::info("    AOT types size too large");
      return Unexpect(ErrCode::Value::IntegerTooLong);
    }
    Sec.getTypesAddress().resize(Size);
  }
  for (size_t I = 0; I < Sec.getTypesAddress().size(); ++I) {
    if (auto Res = VecMgr.readU64(); unlikely(!Res)) {
      spdlog::info(Res.error());
      spdlog::info("    AOT type address read error:{}", Res.error());
      return Unexpect(Res);
    } else {
      Sec.getTypesAddress()[I] = *Res;
    }
  }
  if (auto Res = VecMgr.readU64(); unlikely(!Res)) {
    spdlog::info(Res.error());
    spdlog::info("    AOT code size read error:{}", Res.error());
    return Unexpect(Res);
  } else {
    const uint64_t Size = *Res;
    if (Size > VecMgr.getRemainSize()) {
      spdlog::info(ErrCode::Value::IntegerTooLong);
      spdlog::info("    AOT code size too large");
      return Unexpect(ErrCode::Value::IntegerTooLong);
    }
    Sec.getCodesAddress().resize(Size);
  }
  for (size_t I = 0; I < Sec.getCodesAddress().size(); ++I) {
    if (auto Res = VecMgr.readU64(); unlikely(!Res)) {
      spdlog::info(Res.error());
      spdlog::info("    AOT code address read error:{}", Res.error());
      return Unexpect(Res);
    } else {
      const uint64_t Address = *Res;
      Sec.getCodesAddress()[I] = Address;
    }
  }

  if (auto Res = VecMgr.readU32(); unlikely(!Res)) {
    spdlog::info(Res.error());
    spdlog::info("    AOT section count read error:{}", Res.error());
    return Unexpect(Res);
  } else {
    const uint32_t Size = *Res;
    if (Size > VecMgr.getRemainSize()) {
      spdlog::info(ErrCode::Value::IntegerTooLong);
      spdlog::info("    AOT section count too large");
      return Unexpect(ErrCode::Value::IntegerTooLong);
    }
    Sec.getSections().resize(Size);
  }

  for (auto &Section : Sec.getSections()) {
    if (auto Res = VecMgr.readByte(); unlikely(!Res)) {
      spdlog::info(Res.error());
      spdlog::info("    AOT section type read error:{}", Res.error());
      return Unexpect(Res);
    } else {
      std::get<0>(Section) = *Res;
    }
    if (auto Res = VecMgr.readU64(); unlikely(!Res)) {
      spdlog::info(Res.error());
      spdlog::info("    AOT section offset read error:{}", Res.error());
      return Unexpect(Res);
    } else {
      std::get<1>(Section) = *Res;
    }
    if (auto Res = VecMgr.readU64(); unlikely(!Res)) {
      spdlog::info(Res.error());
      spdlog::info("    AOT section size read error:{}", Res.error());
      return Unexpect(Res);
    } else {
      std::get<2>(Section) = *Res;
    }
    uint32_t ContentSize;
    if (auto Res = VecMgr.readU32(); unlikely(!Res)) {
      spdlog::info(Res.error());
      spdlog::info("    AOT section data size read error:{}", Res.error());
      return Unexpect(Res);
    } else {
      ContentSize = *Res;
      if (ContentSize > VecMgr.getRemainSize()) {
        spdlog::info(ErrCode::Value::IntegerTooLong);
        spdlog::info("    AOT section data size is too large");
        return Unexpect(ErrCode::Value::IntegerTooLong);
      }
      if (std::get<2>(Section) < ContentSize) {
        spdlog::info(ErrCode::Value::IntegerTooLong);
        spdlog::info("    AOT section data size is larger then section size");
        return Unexpect(ErrCode::Value::IntegerTooLong);
      }
    }
    if (auto Res = VecMgr.readBytes(ContentSize); unlikely(!Res)) {
      spdlog::info(Res.error());
      spdlog::info("    AOT section data read error:{}", Res.error());
      return Unexpect(Res);
    } else {
      std::get<3>(Section) = std::move(*Res);
    }
  }
  return {};
}

} // namespace Loader
} // namespace WasmEdge
