#include "loader/section.h"

namespace AST {

/// Load binary to construct Section node. See "include/loader/section.h".
Base::ErrCode Section::loadBinary(FileMgr &Mgr) {
  Base::ErrCode Status = loadSize(Mgr);
  if (Status != Base::ErrCode::Success)
    return Status;
  return loadContent(Mgr);
}

/// Load content size.
Base::ErrCode Section::loadSize(FileMgr &Mgr) {
  return static_cast<Base::ErrCode>(Mgr.readU32(ContentSize));
}

/// Template function of reading vector. See "include/loader/section.h".
template <typename T>
Base::ErrCode Section::loadVector(FileMgr &Mgr,
                                  std::vector<std::unique_ptr<T>> &Vec) {
  unsigned int VecCnt = 0;
  Base::ErrCode Status = Base::ErrCode::Success;

  /// Read vector size.
  if ((Status = static_cast<Base::ErrCode>(Mgr.readU32(VecCnt))) !=
      Base::ErrCode::Success)
    return Status;

  /// Sequently create AST node T and read data.
  for (int i = 0; i < VecCnt; i++) {
    auto NewContent = std::make_unique<T>();
    if ((Status = NewContent->loadBinary(Mgr)) != Base::ErrCode::Success)
      return Status;
    Vec.push_back(std::move(NewContent));
  }
  return Status;
}

/// Load content of custom section.
Base::ErrCode CustomSection::loadContent(FileMgr &Mgr) {
  /// Read all raw bytes.
  return static_cast<Base::ErrCode>(Mgr.readBytes(Content, ContentSize));
}

/// Load vector of type section.
Base::ErrCode TypeSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of import section.
Base::ErrCode ImportSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of function section.
Base::ErrCode FunctionSection::loadContent(FileMgr &Mgr) {
  unsigned int VecCnt = 0;
  unsigned int Idx = 0;
  Base::ErrCode Status = Base::ErrCode::Success;

  /// Read vector count.
  if ((Status = static_cast<Base::ErrCode>(Mgr.readU32(VecCnt))) !=
      Base::ErrCode::Success)
    return Status;

  /// Read function indices.
  for (int i = 0; i < VecCnt; i++) {
    if ((Status = static_cast<Base::ErrCode>(Mgr.readU32(Idx))) !=
        Base::ErrCode::Success)
      return Status;
    Content.push_back(Idx);
  }
  return Status;
}

/// Load vector of table section.
Base::ErrCode TableSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of memory section.
Base::ErrCode MemorySection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of global section.
Base::ErrCode GlobalSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of export section.
Base::ErrCode ExportSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load start function index.
Base::ErrCode StartSection::loadContent(FileMgr &Mgr) {
  return static_cast<Base::ErrCode>(Mgr.readU32(Content));
}

/// Load vector of element section.
Base::ErrCode ElementSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of code section.
Base::ErrCode CodeSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

/// Load vector of data section.
Base::ErrCode DataSection::loadContent(FileMgr &Mgr) {
  return Section::loadVector(Mgr, Content);
}

} // namespace AST