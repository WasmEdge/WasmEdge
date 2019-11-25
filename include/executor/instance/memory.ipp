#include "executor/instance/memory.h"

#include <string.h>

namespace SSVM {
namespace Executor {
namespace Instance {

/// Get pointer or null in memory. See "include/executor/instance/memory.h".
template <typename T>
typename std::enable_if_t<std::is_pointer_v<T>, T>
MemoryInstance::getPointerOrNull(unsigned int Offset) {
  if (Offset >= Data.size() || Offset == 0) {
    return nullptr;
  }
  return reinterpret_cast<T>(&Data[Offset]);
}

/// Get pointer to offset of memory. See "include/executor/instance/memory.h".
template <typename T>
typename std::enable_if_t<std::is_pointer_v<T>, T>
MemoryInstance::getPointer(unsigned int Offset) {
  if (Offset >= Data.size()) {
    return nullptr;
  }
  return reinterpret_cast<T>(&Data[Offset]);
}

/// Load value from data. See "include/executor/instance/memory.h".
template <typename T>
typename std::enable_if_t<Support::IsWasmTypeV<T>, ErrCode>
MemoryInstance::loadValue(T &Value, unsigned int Offset, unsigned int Length) {
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
  if (std::is_floating_point_v<T>) {
    /// Floating case. Do memory copy.
    memcpy(&Value, &Data[Offset], sizeof(T));
  } else {
    uint64_t LoadVal = 0;
    memcpy(&LoadVal, &Data[Offset], Length);
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
MemoryInstance::storeValue(const T &Value, unsigned int Offset,
                           unsigned int Length) {
  /// Check data boundary.
  if (Length > sizeof(T)) {
    return ErrCode::AccessForbidMemory;
  }

  /// Check memory size.
  ErrCode Status = ErrCode::Success;
  if ((Status = checkDataSize(Offset + Length)) != ErrCode::Success) {
    return Status;
  }

  /// Copy store data to value.
  memcpy(&Data[Offset], &Value, Length);
  return ErrCode::Success;
}

} // namespace Instance
} // namespace Executor
} // namespace SSVM
