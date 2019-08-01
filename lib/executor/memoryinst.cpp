#include "executor/memoryinst.h"
#include <iterator>

/// Setter of memory limit. See "include/executor/memoryinst.h".
Executor::ErrCode MemoryInstance::setLimit(bool HasMax, unsigned int Max) {
  HasMaxPage = HasMax;
  MaxPage = Max;
  return Executor::ErrCode::Success;
}

/// Set the initialization list.
Executor::ErrCode
MemoryInstance::setInitList(unsigned int Offset,
                            std::vector<unsigned char> &Bytes) {
  if (Data.size() < Offset + Bytes.size())
    Data.resize(Offset + Bytes.size());
  for (auto it = Bytes.begin(); it != Bytes.end(); it++)
    Data[Offset + it - Bytes.begin()] = *it;
  return Executor::ErrCode::Success;
}