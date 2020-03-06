// SPDX-License-Identifier: Apache-2.0
#include "common/ast/section.h"

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
  unsigned int VecCnt = 0;
  /// Read vector count.
  if (auto Res = Mgr.readU32()) {
    VecCnt = *Res;
  } else {
    return Unexpect(Res);
  }
  for (int i = 0; i < VecCnt; i++) {
    if (auto Res = Mgr.readU32()) {
      Content.push_back(*Res);
    } else {
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

} // namespace AST
} // namespace SSVM
