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
  /// Check memory size.
  ErrCode Status = ErrCode::Success;
  if ((Status = checkDataSize(Offset, Length)) != ErrCode::Success) {
    return Status;
  }
  if (Length == 0) {
    return ErrCode::Success;
  }

  unsigned int OriginSize = Slice.size();
  Slice.resize(Slice.size() + Length);
  std::memcpy(&Slice[OriginSize], &Data[Offset], Length);
  return ErrCode::Success;
}

/// Setter of data list. See "include/executor/instance/memory.h".
ErrCode MemoryInstance::setBytes(const Bytes &Slice, unsigned int Offset,
                                 unsigned int Start, unsigned int Length) {
  /// Check memory size.
  ErrCode Status = ErrCode::Success;
  if ((Status = checkDataSize(Offset, Length)) != ErrCode::Success) {
    return Status;
  }

  /// Check input data validation.
  if (Start > Slice.size() || Start + Length > Slice.size()) {
    return ErrCode::AccessForbidMemory;
  }

  /// Copy data.
  if (Length == 0) {
    return ErrCode::Success;
  }
  std::memcpy(&Data[Offset], &Slice[Start], Length);
  return ErrCode::Success;
}

/// Getter of data to array. See "include/executor/instance/memory.h".
ErrCode MemoryInstance::getArray(uint8_t *Arr, unsigned int Offset,
                                 unsigned int Length, bool IsReverse) {
  /// Check memory size.
  ErrCode Status = ErrCode::Success;
  if ((Status = checkDataSize(Offset, Length)) != ErrCode::Success) {
    return Status;
  }
  if (Length == 0) {
    return ErrCode::Success;
  }

  /// Copy data.
  if (IsReverse) {
    for (uint32_t I = 0; I < Length; I++) {
      Arr[I] = Data[Offset + Length - I - 1];
    }
  } else {
    std::memcpy(Arr, &Data[Offset], Length);
  }
  return ErrCode::Success;
}

/// Getter of data from array. See "include/executor/instance/memory.h".
ErrCode MemoryInstance::setArray(const uint8_t *Arr, unsigned int Offset,
                                 unsigned int Length, bool IsReverse) {
  /// Check memory size.
  ErrCode Status = ErrCode::Success;
  if ((Status = checkDataSize(Offset, Length)) != ErrCode::Success) {
    return Status;
  }
  if (Length == 0) {
    return ErrCode::Success;
  }

  /// Copy data.
  if (IsReverse) {
    for (uint32_t I = 0; I < Length; I++) {
      Data[Offset + Length - I - 1] = Arr[I];
    }
  } else {
    std::memcpy(&Data[Offset], Arr, Length);
  }
  return ErrCode::Success;
}

/// Check access size and vector size. See "include/executor/instance/memory.h".
ErrCode MemoryInstance::checkDataSize(uint32_t Offset, uint32_t Length) {
  uint64_t AccessLen =
      static_cast<uint64_t>(Offset) + static_cast<uint64_t>(Length);
  if (AccessLen > CurrPage * 65536ULL) {
    return ErrCode::MemorySizeExceeded;
  }
  /// Note: the vector size will <= CurrPage * 65536
  if (Data.size() < Offset + Length) {
    unsigned int TargetSize = (Offset + Length) / 8 + 1;
    if (TargetSize < 32 * 65536) {
      TargetSize *= 2;
    } else {
      TargetSize *= 1.1;
    }
    if (TargetSize * 8 > CurrPage * 65536) {
      Data.resize(CurrPage * 65536);
    } else {
      Data.resize(TargetSize * 8);
    }
  }
  return ErrCode::Success;
}

} // namespace Instance
} // namespace Executor
} // namespace SSVM
