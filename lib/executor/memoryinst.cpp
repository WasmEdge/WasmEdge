#include "executor/memoryinst.h"
#include <iterator>

namespace SSVM {
namespace Executor {

/// Setter of memory limit. See "include/executor/memoryinst.h".
Executor::ErrCode MemoryInstance::setLimit(bool HasMax, unsigned int Max) {
  HasMaxPage = HasMax;
  MaxPage = Max;
  return Executor::ErrCode::Success;
}

/// Set the initialization list. See "include/executor/memoryinst.h".
Executor::ErrCode
MemoryInstance::setInitList(unsigned int Offset,
                            std::vector<unsigned char> &Bytes) {
  if (Data.size() < Offset + Bytes.size())
    Data.resize(Offset + Bytes.size());
  for (auto It = Bytes.begin(); It != Bytes.end(); It++)
    Data[Offset + It - Bytes.begin()] = *It;
  return Executor::ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
