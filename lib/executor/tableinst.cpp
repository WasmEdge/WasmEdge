#include "executor/tableinst.h"
#include <iterator>

namespace SSVM {

/// Setter of element type. See "include/executor/tableinst.h".
Executor::ErrCode TableInstance::setElemType(AST::ElemType &Elem) {
  Type = Elem;
  return Executor::ErrCode::Success;
}

/// Setter of table limit. See "include/executor/tableinst.h".
Executor::ErrCode TableInstance::setLimit(bool HasMax, unsigned int Max) {
  HasMaxSize = HasMax;
  MaxSize = Max;
  return Executor::ErrCode::Success;
}

/// Setter of initialization list. See "include/executor/tableinst.h".
Executor::ErrCode TableInstance::setInitList(unsigned int Offset,
                                             std::vector<unsigned int> &Addrs) {
  if (FuncElem.size() < Offset + Addrs.size())
    FuncElem.resize(Offset + Addrs.size());
  for (auto it = Addrs.begin(); it != Addrs.end(); it++)
    FuncElem[Offset + it - Addrs.begin()] = *it;
  return Executor::ErrCode::Success;
}

} // namespace SSVM