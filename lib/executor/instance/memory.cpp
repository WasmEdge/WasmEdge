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

/// Load value from data. See "include/executor/instance/memory.h".
template <typename T>
typename std::enable_if_t<Support::IsWasmType<T>::value, ErrCode>
MemoryInstance::loadValue(unsigned int Offset, unsigned int Length, T &Value) {
  /// Check data boundary.
  if (Length > sizeof(T)) {
    return ErrCode::AccessForbidMemory;
  }

  /// Check memory size.
  ErrCode Status = ErrCode::Success;
  if ((Status = checkDataSize(Offset + Length)) != ErrCode::Success) {
    return Status;
  }

  /// Load data to a value.
  uint64_t LoadVal = 0;
  for (unsigned int I = 0; I < Length; I++) {
    LoadVal |= static_cast<uint64_t>(Data.at(I + Offset)) << (I * 8);
  }

  if (std::is_floating_point_v<T>) {
    /// Floating case. Do memory copy.
    memcpy(&Value, &LoadVal, sizeof(T));
  } else {
    /// Integer case. Do extend to result type.
    if (std::is_signed_v<T> && (LoadVal >> (Length * 8 - 1))) {
      /// Signed extend.
      for (unsigned int I = Length; I < 8; I++) {
        LoadVal |= 0xFFULL << (I * 8);
      }
    }
    Value = static_cast<T>(LoadVal);
  }
  return Status;
}

/// Store value to data. See "include/executor/instance/memory.h".
template <typename T>
typename std::enable_if_t<Support::IsWasmBuiltIn<T>::value, ErrCode>
MemoryInstance::storeValue(unsigned int Offset, unsigned int Length,
                           const T &Value) {
  /// Check data boundary.
  if (Length > sizeof(T)) {
    return ErrCode::AccessForbidMemory;
  }

  /// Check memory size.
  ErrCode Status = ErrCode::Success;
  if ((Status = checkDataSize(Offset + Length)) != ErrCode::Success) {
    return Status;
  }

  /// Copy store data to a value.
  uint64_t StoreVal = 0;
  memcpy(&StoreVal, &Value, Length / 8);
  for (unsigned int I = 0; I < Length; I++) {
    Data.at(I + Offset) = static_cast<Byte>(StoreVal & 0xFFU);
    StoreVal >>= 8;
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
