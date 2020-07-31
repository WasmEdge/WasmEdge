// SPDX-License-Identifier: Apache-2.0
#include "common/ast/section.h"
#include "support/log.h"

namespace SSVM {
namespace AST {

/// Load binary to construct Section node. See "include/ast/section.h".
Expect<void> Section::loadBinary(FileMgr &Mgr) {
  if (auto Res = loadSize(Mgr)) {
    return loadContent(Mgr);
  } else {
    return Unexpect(Res);
  }
}

/// Load content size. See "include/ast/section.h".
Expect<void> Section::loadSize(FileMgr &Mgr) {
  if (auto Res = Mgr.readU32()) {
    ContentSize = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }
  return {};
}

/// Load content of custom section. See "include/ast/section.h".
Expect<void> CustomSection::loadContent(FileMgr &Mgr) {
  /// Read all raw bytes.
  if (auto Res = Mgr.readBytes(ContentSize)) {
    Content = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }
  return {};
}

/// Load vector of type section. See "include/ast/section.h".
Expect<void> TypeSection::loadContent(FileMgr &Mgr) {
  return Section::loadToVector(Mgr, Content);
}

/// Load vector of import section. See "include/ast/section.h".
Expect<void> ImportSection::loadContent(FileMgr &Mgr) {
  return Section::loadToVector(Mgr, Content);
}

/// Load vector of function section. See "include/ast/section.h".
Expect<void> FunctionSection::loadContent(FileMgr &Mgr) {
  uint32_t VecCnt = 0;
  /// Read vector count.
  if (auto Res = Mgr.readU32()) {
    VecCnt = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }
  for (uint32_t i = 0; i < VecCnt; ++i) {
    if (auto Res = Mgr.readU32()) {
      Content.push_back(*Res);
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Res);
    }
  }
  return {};
}

/// Load vector of table section. See "include/ast/section.h".
Expect<void> TableSection::loadContent(FileMgr &Mgr) {
  return Section::loadToVector(Mgr, Content);
}

/// Load vector of memory section. See "include/ast/section.h".
Expect<void> MemorySection::loadContent(FileMgr &Mgr) {
  return Section::loadToVector(Mgr, Content);
}

/// Load vector of global section. See "include/ast/section.h".
Expect<void> GlobalSection::loadContent(FileMgr &Mgr) {
  return Section::loadToVector(Mgr, Content);
}

/// Load vector of export section. See "include/ast/section.h".
Expect<void> ExportSection::loadContent(FileMgr &Mgr) {
  return Section::loadToVector(Mgr, Content);
}

/// Load start function index. See "include/ast/section.h".
Expect<void> StartSection::loadContent(FileMgr &Mgr) {
  if (auto Res = Mgr.readU32()) {
    Content = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }
  return {};
}

/// Load vector of element section. See "include/ast/section.h".
Expect<void> ElementSection::loadContent(FileMgr &Mgr) {
  return Section::loadToVector(Mgr, Content);
}

/// Load vector of code section. See "include/ast/section.h".
Expect<void> CodeSection::loadContent(FileMgr &Mgr) {
  return Section::loadToVector(Mgr, Content);
}

/// Load vector of data section. See "include/ast/section.h".
Expect<void> DataSection::loadContent(FileMgr &Mgr) {
  return Section::loadToVector(Mgr, Content);
}

/// Load content of data count section. See "include/ast/section.h".
Expect<void> DataCountSection::loadContent(FileMgr &Mgr) {
  /// Read u32 of data count.
  if (auto Res = Mgr.readU32()) {
    Content = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }
  return {};
}

} // namespace AST
} // namespace SSVM
