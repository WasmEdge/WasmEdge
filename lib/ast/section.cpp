// SPDX-License-Identifier: Apache-2.0

#include "ast/section.h"

#include "aot/version.h"
#include "common/defines.h"

namespace WasmEdge {
namespace AST {

/// Load binary to construct Section node. See "include/ast/section.h".
Expect<void> Section::loadBinary(FileMgr &Mgr, const Configure &Conf) {
  if (auto Res = loadSize(Mgr)) {
    Mgr.setSectionSize(ContentSize);
    auto ResContent = loadContent(Mgr, Conf);
    Mgr.unsetSectionSize();
    if (!ResContent) {
      return Unexpect(ResContent);
    }
  } else {
    return Unexpect(Res);
  }
  return {};
}

/// Load content size. See "include/ast/section.h".
Expect<void> Section::loadSize(FileMgr &Mgr) {
  if (auto Res = Mgr.readU32()) {
    ContentSize = *Res;
  } else {
    return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
  }
  return {};
}

/// Load content of custom section. See "include/ast/section.h".
Expect<void> CustomSection::loadContent(FileMgr &Mgr, const Configure &) {
  /// Read name.
  auto ReadSize = Mgr.getOffset();
  if (auto Res = Mgr.readName()) {
    Name = *Res;
  } else {
    return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
  }
  ReadSize = Mgr.getOffset() - ReadSize;
  /// Read remain bytes.
  if (auto Res = Mgr.readBytes(ContentSize - ReadSize)) {
    Content.insert(Content.end(), (*Res).begin(), (*Res).end());
  } else {
    return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
  }
  return {};
}

/// Load vector of type section. See "include/ast/section.h".
Expect<void> TypeSection::loadContent(FileMgr &Mgr, const Configure &Conf) {
  return Section::loadToVector(Mgr, Conf, NodeAttr, Content);
}

/// Load vector of import section. See "include/ast/section.h".
Expect<void> ImportSection::loadContent(FileMgr &Mgr, const Configure &Conf) {
  return Section::loadToVector(Mgr, Conf, NodeAttr, Content);
}

/// Load vector of function section. See "include/ast/section.h".
Expect<void> FunctionSection::loadContent(FileMgr &Mgr, const Configure &) {
  auto StartOffset = Mgr.getOffset();
  uint32_t VecCnt = 0;
  /// Read vector count.
  if (auto Res = Mgr.readU32()) {
    VecCnt = *Res;
    /// A section may be splited into partitions in module.
    Content.reserve(Content.size() + VecCnt);
  } else {
    return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
  }
  for (uint32_t i = 0; i < VecCnt; ++i) {
    if (auto Res = Mgr.readU32()) {
      Content.push_back(*Res);
    } else {
      return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
    }
  }
  /// Check the read size match the section size.
  auto EndOffset = Mgr.getOffset();
  if (EndOffset - StartOffset != ContentSize) {
    return logLoadError(ErrCode::SectionSizeMismatch, EndOffset, NodeAttr);
  }
  return {};
}

/// Load vector of table section. See "include/ast/section.h".
Expect<void> TableSection::loadContent(FileMgr &Mgr, const Configure &Conf) {
  return Section::loadToVector(Mgr, Conf, NodeAttr, Content);
}

/// Load vector of memory section. See "include/ast/section.h".
Expect<void> MemorySection::loadContent(FileMgr &Mgr, const Configure &Conf) {
  return Section::loadToVector(Mgr, Conf, NodeAttr, Content);
}

/// Load vector of global section. See "include/ast/section.h".
Expect<void> GlobalSection::loadContent(FileMgr &Mgr, const Configure &Conf) {
  return Section::loadToVector(Mgr, Conf, NodeAttr, Content);
}

/// Load vector of export section. See "include/ast/section.h".
Expect<void> ExportSection::loadContent(FileMgr &Mgr, const Configure &Conf) {
  return Section::loadToVector(Mgr, Conf, NodeAttr, Content);
}

/// Load start function index. See "include/ast/section.h".
Expect<void> StartSection::loadContent(FileMgr &Mgr, const Configure &) {
  auto StartOffset = Mgr.getOffset();
  if (auto Res = Mgr.readU32()) {
    Content = *Res;
  } else {
    return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
  }
  /// Check the read size match the section size.
  auto EndOffset = Mgr.getOffset();
  if (EndOffset - StartOffset != ContentSize) {
    return logLoadError(ErrCode::SectionSizeMismatch, EndOffset, NodeAttr);
  }
  return {};
}

/// Load vector of element section. See "include/ast/section.h".
Expect<void> ElementSection::loadContent(FileMgr &Mgr, const Configure &Conf) {
  return Section::loadToVector(Mgr, Conf, NodeAttr, Content);
}

/// Load vector of code section. See "include/ast/section.h".
Expect<void> CodeSection::loadContent(FileMgr &Mgr, const Configure &Conf) {
  return Section::loadToVector(Mgr, Conf, NodeAttr, Content);
}

/// Load vector of data section. See "include/ast/section.h".
Expect<void> DataSection::loadContent(FileMgr &Mgr, const Configure &Conf) {
  return Section::loadToVector(Mgr, Conf, NodeAttr, Content);
}

/// Load content of data count section. See "include/ast/section.h".
Expect<void> DataCountSection::loadContent(FileMgr &Mgr, const Configure &) {
  auto StartOffset = Mgr.getOffset();
  /// Read u32 of data count.
  if (auto Res = Mgr.readU32()) {
    Content = *Res;
  } else {
    return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
  }
  /// Check the read size match the section size.
  auto EndOffset = Mgr.getOffset();
  if (EndOffset - StartOffset != ContentSize) {
    return logLoadError(ErrCode::SectionSizeMismatch, EndOffset, NodeAttr);
  }
  return {};
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
#endif
}

inline constexpr uint8_t HostArchType() noexcept {
#if defined(__x86_64__)
  return UINT8_C(1);
#elif defined(__aarch64__)
  return UINT8_C(2);
#endif
}

} // namespace

Expect<void> AOTSection::loadBinary(FileMgr &Mgr, const Configure &) {
  if (auto Res = Mgr.readU32(); !Res) {
    spdlog::error("AOT binary version read error:{}", Res.error());
    return Unexpect(Res);
  } else {
    Version = *Res;
  }
  if (Version != HostVersion()) {
    spdlog::error("AOT binary version unmatched.");
    return Unexpect(ErrCode::MalformedSection);
  }

  if (auto Res = Mgr.readByte(); !Res) {
    spdlog::error("AOT os type read error:{}", Res.error());
    return Unexpect(Res);
  } else {
    OSType = *Res;
  }
  if (OSType != HostOSType()) {
    spdlog::error("AOT OS type unmatched.");
    return Unexpect(ErrCode::MalformedSection);
  }

  if (auto Res = Mgr.readByte(); !Res) {
    spdlog::error("AOT arch type read error:{}", Res.error());
    return Unexpect(Res);
  } else {
    ArchType = *Res;
  }
  if (ArchType != HostArchType()) {
    spdlog::error("AOT arch type unmatched.");
    return Unexpect(ErrCode::MalformedSection);
  }

  if (auto Res = Mgr.readU64(); !Res) {
    spdlog::error("AOT version address read error:{}", Res.error());
    return Unexpect(Res);
  } else {
    VersionAddress = *Res;
  }
  if (auto Res = Mgr.readU64(); !Res) {
    spdlog::error("AOT intrinsics address read error:{}", Res.error());
    return Unexpect(Res);
  } else {
    IntrinsicsAddress = *Res;
  }
  if (auto Res = Mgr.readU64(); !Res) {
    spdlog::error("AOT types size read error:{}", Res.error());
    return Unexpect(Res);
  } else {
    TypesAddress.resize(*Res);
  }
  for (size_t I = 0; I < TypesAddress.size(); ++I) {
    if (auto Res = Mgr.readU64(); !Res) {
      spdlog::error("AOT type address read error:{}", Res.error());
      return Unexpect(Res);
    } else {
      TypesAddress[I] = *Res;
    }
  }
  if (auto Res = Mgr.readU64(); !Res) {
    spdlog::error("AOT code size read error:{}", Res.error());
    return Unexpect(Res);
  } else {
    CodesAddress.resize(*Res);
  }
  for (size_t I = 0; I < CodesAddress.size(); ++I) {
    if (auto Res = Mgr.readU64(); !Res) {
      spdlog::error("AOT code address read error:{}", Res.error());
      return Unexpect(Res);
    } else {
      CodesAddress[I] = *Res;
    }
  }

  if (auto Res = Mgr.readU32(); !Res) {
    spdlog::error("AOT section count read error:{}", Res.error());
    return Unexpect(Res);
  } else {
    Sections.resize(*Res);
  }

  for (auto &Section : Sections) {
    if (auto Res = Mgr.readByte(); !Res) {
      spdlog::error("AOT section type read error:{}", Res.error());
      return Unexpect(Res);
    } else {
      std::get<0>(Section) = *Res;
    }
    if (auto Res = Mgr.readU64(); !Res) {
      spdlog::error("AOT section offset read error:{}", Res.error());
      return Unexpect(Res);
    } else {
      std::get<1>(Section) = *Res;
    }
    if (auto Res = Mgr.readU64(); !Res) {
      spdlog::error("AOT section size read error:{}", Res.error());
      return Unexpect(Res);
    } else {
      std::get<2>(Section) = *Res;
    }
    uint32_t ContentSize;
    if (auto Res = Mgr.readU32(); !Res) {
      spdlog::error("AOT section data size read error:{}", Res.error());
      return Unexpect(Res);
    } else {
      ContentSize = *Res;
    }
    if (auto Res = Mgr.readBytes(ContentSize); !Res) {
      spdlog::error("AOT section data read error:{}", Res.error());
      return Unexpect(Res);
    } else {
      std::get<3>(Section) = std::move(*Res);
    }
  }

  return {};
}

} // namespace AST
} // namespace WasmEdge
