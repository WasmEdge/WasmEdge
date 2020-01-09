// SPDX-License-Identifier: Apache-2.0
#include "executor/instance/memory.h"

#include <cstring>

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
ErrCode MemoryInstance::getBytes(Bytes &Slice, unsigned int Offset,
                                 unsigned int Length) {
  if (Length == 0) {
    return ErrCode::Success;
  }
  /// Check memory size.
  ErrCode Status = ErrCode::Success;
  if ((Status = checkDataSize(Offset + Length)) != ErrCode::Success) {
    return Status;
  }

  unsigned int OriginSize = Slice.size();
  Slice.resize(Slice.size() + Length);
  std::memcpy(&Slice[OriginSize], &Data[Offset], Length);
  return ErrCode::Success;
}

/// Setter of data list. See "include/executor/instance/memory.h".
ErrCode MemoryInstance::setBytes(Bytes &Slice, unsigned int Offset,
                                 unsigned int Start, unsigned int Length) {
  if (Length == 0) {
    return ErrCode::Success;
  }
  /// Check memory size.
  ErrCode Status = ErrCode::Success;
  if ((Status = checkDataSize(Offset + Length)) != ErrCode::Success) {
    return Status;
  }

  /// Check input data validation.
  if (Start >= Slice.size() || Start + Length > Slice.size()) {
    return ErrCode::AccessForbidMemory;
  }

  /// Copy data.
  std::memcpy(&Data[Offset], &Slice[Start], Length);
  return ErrCode::Success;
}

/// Check access size and vector size. See "include/executor/instance/memory.h".
ErrCode MemoryInstance::checkDataSize(unsigned int AccessSize) {
  if (HasMaxPage && AccessSize > MaxPage * 65536) {
    return ErrCode::MemorySizeExceeded;
  }
  if (Data.size() < AccessSize) {
    unsigned int TargetSize = AccessSize / 8 + 1;
    if (TargetSize < 32 * 65536) {
      TargetSize *= 2;
    } else {
      TargetSize *= 1.1;
    }
    Data.resize(TargetSize * 8);
  }
  return ErrCode::Success;
}

} // namespace Instance
} // namespace Executor
} // namespace SSVM
