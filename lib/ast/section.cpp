// SPDX-License-Identifier: Apache-2.0
#include "ast/section.h"
#include "common/log.h"

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
    return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
  }
  return {};
}

/// Load content of custom section. See "include/ast/section.h".
Expect<void> CustomSection::loadContent(FileMgr &Mgr, const Configure &Conf) {
  /// Read name.
  auto ReadSize = Mgr.getOffset();
  if (auto Res = Mgr.readName()) {
    Content = std::vector<uint8_t>((*Res).begin(), (*Res).end());
  } else {
    return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
  }
  ReadSize = Mgr.getOffset() - ReadSize;
  /// Read remain bytes.
  if (auto Res = Mgr.readBytes(ContentSize - ReadSize)) {
    Content.insert(Content.end(), (*Res).begin(), (*Res).end());
  } else {
    return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
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
Expect<void> FunctionSection::loadContent(FileMgr &Mgr, const Configure &Conf) {
  auto StartOffset = Mgr.getOffset();
  uint32_t VecCnt = 0;
  /// Read vector count.
  if (auto Res = Mgr.readU32()) {
    VecCnt = *Res;
    /// A section may be splited into partitions in module.
    Content.reserve(Content.size() + VecCnt);
  } else {
    return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
  }
  for (uint32_t i = 0; i < VecCnt; ++i) {
    if (auto Res = Mgr.readU32()) {
      Content.push_back(*Res);
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
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
Expect<void> StartSection::loadContent(FileMgr &Mgr, const Configure &Conf) {
  auto StartOffset = Mgr.getOffset();
  if (auto Res = Mgr.readU32()) {
    Content = *Res;
  } else {
    return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
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
Expect<void> DataCountSection::loadContent(FileMgr &Mgr,
                                           const Configure &Conf) {
  auto StartOffset = Mgr.getOffset();
  /// Read u32 of data count.
  if (auto Res = Mgr.readU32()) {
    Content = *Res;
  } else {
    return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
  }
  /// Check the read size match the section size.
  auto EndOffset = Mgr.getOffset();
  if (EndOffset - StartOffset != ContentSize) {
    return logLoadError(ErrCode::SectionSizeMismatch, EndOffset, NodeAttr);
  }
  return {};
}

} // namespace AST
} // namespace WasmEdge
