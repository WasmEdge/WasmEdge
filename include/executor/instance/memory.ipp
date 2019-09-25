#include "executor/instance/memory.h"

#include <string.h>

namespace SSVM {
namespace Executor {
namespace Instance {

/// Load value from data. See "include/executor/instance/memory.h".
template <typename T>
typename std::enable_if_t<Support::IsWasmTypeV<T>, ErrCode>
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
typename std::enable_if_t<Support::IsWasmBuiltInV<T>, ErrCode>
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

} // namespace Instance
} // namespace Executor
} // namespace SSVM
