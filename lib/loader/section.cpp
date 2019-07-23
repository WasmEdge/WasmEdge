#include "loader/section.h"

namespace AST {

/// Load binary to construct Section node. See "include/loader/section.h".
Loader::ErrCode Section::loadBinary(FileMgr &Mgr) {
  Loader::ErrCode Status = loadSize(Mgr);
  if (Status != Loader::ErrCode::Success)
    return Status;
  return loadContent(Mgr);
}

/// Load content size.
Loader::ErrCode Section::loadSize(FileMgr &Mgr) {
  return Mgr.readU32(ContentSize);
}

/// Template function of reading vector. See "include/loader/section.h".
template <typename T>
Loader::ErrCode Section::loadVector(FileMgr &Mgr,
                                    std::vector<std::unique_ptr<T>> &Vec) {
  unsigned int VecCnt = 0;
  Loader::ErrCode Status = Loader::ErrCode::Success;

  /// Read vector size.
  if ((Status = Mgr.readU32(VecCnt)) != Loader::ErrCode::Success)
    return Status;

  /// Sequently create AST node T and read data.
  for (int i = 0; i < VecCnt; i++) {
    auto NewContent = std::make_unique<T>();
    if ((Status = NewContent->loadBinary(Mgr)) != Loader::ErrCode::Success)
      return Status;
    Vec.push_back(std::move(NewContent));
  }
  return Status;
}

/// Load content of custom section.
Loader::ErrCode CustomSection::loadContent(FileMgr &Mgr) {
  /// Read all raw bytes.
  return Mgr.readBytes(Content, ContentSize);
}

/// Load vector of type section.
Loader::ErrCode TypeSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of import section.
Loader::ErrCode ImportSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of function section.
Loader::ErrCode FunctionSection::loadContent(FileMgr &Mgr) {
  unsigned int VecCnt = 0;
  unsigned int Idx = 0;
  Loader::ErrCode Status = Loader::ErrCode::Success;

  /// Read vector count.
  if ((Status = Mgr.readU32(VecCnt)) != Loader::ErrCode::Success)
    return Status;

  /// Read function indices.
  for (int i = 0; i < VecCnt; i++) {
    if ((Status = Mgr.readU32(Idx)) != Loader::ErrCode::Success)
      return Status;
    Content.push_back(Idx);
  }
  return Status;
}

/// Load vector of table section.
Loader::ErrCode TableSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of memory section.
Loader::ErrCode MemorySection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of global section.
Loader::ErrCode GlobalSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of export section.
Loader::ErrCode ExportSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load start function index.
Loader::ErrCode StartSection::loadContent(FileMgr &Mgr) {
  return Mgr.readU32(Content);
}

/// Load vector of element section.
Loader::ErrCode ElementSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of code section.
Loader::ErrCode CodeSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of data section.
Loader::ErrCode DataSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

} // namespace AST