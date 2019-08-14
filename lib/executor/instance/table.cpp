#include "executor/instance/table.h"
#include <iterator>

namespace SSVM {
namespace Executor {
namespace Instance {

/// Setter of element type. See "include/executor/instance/table.h".
ErrCode TableInstance::setElemType(AST::ElemType &Elem) {
  Type = Elem;
  return ErrCode::Success;
}

/// Setter of table limit. See "include/executor/instance/table.h".
ErrCode TableInstance::setLimit(bool HasMax, unsigned int Max) {
  HasMaxSize = HasMax;
  MaxSize = Max;
  return ErrCode::Success;
}

/// Setter of initialization list. See "include/executor/instance/table.h".
ErrCode TableInstance::setInitList(unsigned int Offset,
                                   std::vector<unsigned int> &Addrs) {
  if (FuncElem.size() < Offset + Addrs.size())
    FuncElem.resize(Offset + Addrs.size());
  for (auto it = Addrs.begin(); it != Addrs.end(); it++)
    FuncElem[Offset + it - Addrs.begin()] = *it;
  return ErrCode::Success;
}

} // namespace Instance
} // namespace Executor
} // namespace SSVM
