#include "executor/instance/memory.h"
#include <iterator>

namespace SSVM {
namespace Executor {
namespace Instance {

/// Setter of memory limit. See "include/executor/instance/memory.h".
ErrCode MemoryInstance::setLimit(bool HasMax, unsigned int Max) {
  HasMaxPage = HasMax;
  MaxPage = Max;
  return ErrCode::Success;
}

/// Set the initialization list. See "include/executor/instance/memory.h".
ErrCode MemoryInstance::setInitList(unsigned int Offset,
                                    std::vector<unsigned char> &Bytes) {
  if (Data.size() < Offset + Bytes.size())
    Data.resize(Offset + Bytes.size());
  for (auto It = Bytes.begin(); It != Bytes.end(); It++)
    Data[Offset + It - Bytes.begin()] = *It;
  return ErrCode::Success;
}

} // namespace Instance
} // namespace Executor
} // namespace SSVM
