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

ErrCode MemoryInstance::getBytes(Bytes &Slice, unsigned int Start,
                                 unsigned int Length) {
  for (auto Iter = Start; Iter < Start + Length; Iter++) {
    Slice.push_back(Data.at(Iter));
  }
  return ErrCode::Success;
}

ErrCode MemoryInstance::setBytes(Bytes &Slice, unsigned int Start,
                                 unsigned int Length) {
  if (Length != Slice.size()) {
    return ErrCode::AccessForbidMemory;
  }
  for (auto Iter = Start; Iter <= Start + Length - 1; Iter++) {
    Data.at(Iter) = Slice.at(Iter - Start);
  }
  return ErrCode::Success;
}

/// Load value from data. See "include/executor/instance/memory.h".
template <typename T>
typename std::enable_if_t<Support::IsWasmType<T>::value, ErrCode>
MemoryInstance::loadValue(unsigned int Offset, unsigned int Length, T &Value) {
  /// Check memory boundary
  if (Data.size() < Offset + Length || Length > sizeof(T)) {
    return ErrCode::AccessForbidMemory;
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
  return ErrCode::Success;
}

/// Store value to data. See "include/executor/instance/memory.h".
template <typename T>
typename std::enable_if_t<Support::IsWasmBuiltIn<T>::value, ErrCode>
MemoryInstance::storeValue(unsigned int Offset, unsigned int Length,
                           const T &Value) {
  /// Check memory boundary
  if (Data.size() < Offset + Length || Length > sizeof(T)) {
    return ErrCode::AccessForbidMemory;
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
