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

ErrCode MemoryInstance::getBytes(std::unique_ptr<Bytes> &Slice, int Start, int Length) {
  Slice = std::make_unique<Bytes>(Data.data() + Start, Data.data() + Start + Length - 1);
  if (Slice.get() == nullptr) {
    return ErrCode::SliceDataFailed;
  }
  return ErrCode::Success;
}

ErrCode MemoryInstance::setBytes(Bytes &TheBytes, int Start, int Length) {
  if (Length != TheBytes.size()) {
    return ErrCode::AccessForbidMemory;
  }
  for (auto Iter = Start; Iter <= Start + Length - 1; Iter++) {
    Data.at(Iter) = TheBytes.at(Iter-Start);
  }
  return ErrCode::Success;
}

} // namespace Instance
} // namespace Executor
} // namespace SSVM
