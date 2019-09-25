#include "executor/instance/memory.h"
#include <iterator>

namespace SSVM {
namespace Executor {
namespace Instance {

/// Setter of memory limit. See "include/executor/instance/memory.h".
ErrCode MemoryInstance::setLimit(unsigned int Min, bool HasMax,
                                 unsigned int Max) {
  HasMaxPage = HasMax;
  MinPage = Min;
  MaxPage = Max;
  CurrPage = Min;
  return ErrCode::Success;
}

/// Grow memory page. See "include/executor/instance/memory.h".
ErrCode MemoryInstance::growPage(unsigned int Count) {
  if ((HasMaxPage && Count + CurrPage > MaxPage) || Count + CurrPage > 65536) {
    return ErrCode::MemorySizeExceeded;
  }
  CurrPage += Count;
  return ErrCode::Success;
}

/// Getter of data list. See "include/executor/instance/memory.h".
ErrCode MemoryInstance::getBytes(Bytes &Slice, unsigned int Start,
                                 unsigned int Length) { /// Check memory size.
  ErrCode Status = ErrCode::Success;
  if ((Status = checkDataSize(Start + Length)) != ErrCode::Success) {
    return Status;
  }

  for (auto Iter = Start; Iter < Start + Length; Iter++) {
    Slice.push_back(Data.at(Iter));
  }
  return ErrCode::Success;
}

/// Setter of data list. See "include/executor/instance/memory.h".
ErrCode MemoryInstance::setBytes(Bytes &Slice, unsigned int Offset) {
  /// Check memory size.
  ErrCode Status = ErrCode::Success;
  if ((Status = checkDataSize(Offset + Slice.size())) != ErrCode::Success) {
    return Status;
  }

  /// Copy data.
  for (unsigned int I = Offset; I < Offset + Slice.size(); I++) {
    Data.at(I) = Slice.at(I - Offset);
  }
  return ErrCode::Success;
}

/// Check access size and vector size. See "include/executor/instance/memory.h".
ErrCode MemoryInstance::checkDataSize(unsigned int accessSize) {
  if (HasMaxPage && accessSize > MaxPage * 65536) {
    return ErrCode::MemorySizeExceeded;
  }
  if (Data.size() < accessSize)
    Data.resize(accessSize);
  return ErrCode::Success;
}

} // namespace Instance
} // namespace Executor
} // namespace SSVM
