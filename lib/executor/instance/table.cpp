// SPDX-License-Identifier: Apache-2.0
#include "executor/instance/table.h"
#include <iterator>

namespace SSVM {
namespace Executor {
namespace Instance {

/// Setter of element type. See "include/executor/instance/table.h".
ErrCode TableInstance::setElemType(ElemType &Elem) {
  Type = Elem;
  return ErrCode::Success;
}

/// Setter of table limit. See "include/executor/instance/table.h".
ErrCode TableInstance::setLimit(unsigned int Min, bool HasMax,
                                unsigned int Max) {
  HasMaxSize = HasMax;
  MinSize = Min;
  MaxSize = Max;
  if (FuncElem.size() < MinSize) {
    FuncElem.resize(MinSize);
  }
  return ErrCode::Success;
}

/// Setter of initialization list. See "include/executor/instance/table.h".
ErrCode TableInstance::setInitList(unsigned int Offset,
                                   std::vector<unsigned int> &Addrs) {
  if (HasMaxSize && Offset + Addrs.size() > MaxSize) {
    return ErrCode::TableSizeExceeded;
  }
  if (FuncElem.size() < Offset + Addrs.size()) {
    unsigned int OriginSize = FuncElem.size();
    FuncElem.resize(Offset + Addrs.size());
    for (unsigned int I = OriginSize; I < FuncElem.size(); I++) {
      FuncElem.at(I) = 0;
    }
  }
  for (auto It = Addrs.begin(); It != Addrs.end(); It++) {
    FuncElem.at((It - Addrs.begin()) + Offset) = *It;
  }
  return ErrCode::Success;
}

/// Getter of address. See "include/executor/instance/table.h".
ErrCode TableInstance::getElemAddr(unsigned int Idx, unsigned int &Addr) {
  if (Idx >= FuncElem.size()) {
    return ErrCode::AccessForbidMemory;
  }
  Addr = FuncElem[Idx];
  return ErrCode::Success;
}

} // namespace Instance
} // namespace Executor
} // namespace SSVM
