#include "loader/section.h"

namespace AST {

/// Load binary to construct Section node. See "include/loader/section.h".
bool Section::loadBinary(FileMgr &Mgr) {
  if (!loadSize(Mgr))
    return false;
  return loadContent(Mgr);
}

/// Load content size.
bool Section::loadSize(FileMgr &Mgr) { return Mgr.readU32(ContentSize); }

/// Template function of reading vector. See "include/loader/section.h".
template <typename T>
bool Section::loadVector(FileMgr &Mgr, std::vector<std::unique_ptr<T>> &Vec) {
  /// Read vector size.
  unsigned int VecCnt = 0;
  if (!Mgr.readU32(VecCnt))
    return false;
  /// Sequently create AST node T and read data.
  for (int i = 0; i < VecCnt; i++) {
    auto NewContent = std::make_unique<T>();
    if (!NewContent->loadBinary(Mgr))
      return false;
    Vec.push_back(std::move(NewContent));
  }
  return true;
}

/// Load content of custom section.
bool CustomSection::loadContent(FileMgr &Mgr) {
  /// Read all raw bytes.
  return Mgr.readSize(Content, ContentSize);
}

/// Load vector of type section.
bool TypeSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of import section.
bool ImportSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of function section.
bool FunctionSection::loadContent(FileMgr &Mgr) {
  unsigned int VecCnt = 0;
  unsigned int Idx = 0;
  /// Read vector count.
  if (!Mgr.readU32(VecCnt))
    return false;
  /// Read function indices.
  for (int i = 0; i < VecCnt; i++) {
    if (!Mgr.readU32(Idx))
      return false;
    Content.push_back(Idx);
  }
  return true;
}

/// Load vector of table section.
bool TableSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of memory section.
bool MemorySection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of global section.
bool GlobalSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of export section.
bool ExportSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load start function index.
bool StartSection::loadContent(FileMgr &Mgr) { return Mgr.readU32(Content); }

/// Load vector of element section.
bool ElementSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of code section.
bool CodeSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of data section.
bool DataSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

} // namespace AST