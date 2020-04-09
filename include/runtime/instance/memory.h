// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/runtime/instance/memory.h - Memory Instance definition --=----===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the memory instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/ast/type.h"
#include "common/errcode.h"
#include "common/value.h"
#include "support/casting.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <vector>

namespace SSVM {
namespace Runtime {
namespace Instance {

class MemoryInstance {
public:
  MemoryInstance() = delete;
  MemoryInstance(const AST::Limit &Lim)
      : HasMaxPage(Lim.hasMax()), MinPage(Lim.getMin()), MaxPage(Lim.getMax()),
        CurrPage(Lim.getMin()) {}
  virtual ~MemoryInstance() = default;

  /// Get page size of memory.data
  uint32_t getDataPageSize() const { return CurrPage; }

  /// Getter of limit definition.
  bool getHasMax() const { return HasMaxPage; }

  /// Getter of limit definition.
  uint32_t getMin() const { return MinPage; }

  /// Getter of limit definition.
  uint32_t getMax() const { return MaxPage; }

  /// Grow page
  Expect<void> growPage(const uint32_t Count) {
    if ((HasMaxPage && Count + CurrPage > MaxPage) ||
        Count + CurrPage > 65536) {
      return Unexpect(ErrCode::MemorySizeExceeded);
    }
    CurrPage += Count;
    return {};
  }

  /// Get memory length.
  const Bytes &getDataVector() const { return Data; }

  /// Get slice of Data[Offset : Offset + Length - 1]
  Expect<Bytes> getBytes(const uint32_t Offset, const uint32_t Length) {
    /// Check memory boundary.
    if (!checkDataSize(Offset, Length)) {
      return Unexpect(ErrCode::MemorySizeExceeded);
    }
    Bytes Slice;
    if (Length > 0) {
      Slice.resize(Length);
      std::copy(Data.begin() + Offset, Data.begin() + Offset + Length,
                Slice.begin());
    }
    return Slice;
  }

  /// Replace the bytes of Data[Offset :] by Slice[Start : Start + Legnth - 1]
  Expect<void> setBytes(const Bytes &Slice, const uint32_t Offset,
                        const uint32_t Start, const uint32_t Length) {
    /// Check memory boundary.
    if (!checkDataSize(Offset, Length)) {
      return Unexpect(ErrCode::MemorySizeExceeded);
    }

    /// Check input data validation.
    if ((Slice.size() > 0 && Start >= Slice.size()) ||
        Start + Length > Slice.size()) {
      return Unexpect(ErrCode::AccessForbidMemory);
    }

    /// Copy data.
    if (Length > 0) {
      std::copy(Slice.begin() + Start, Slice.begin() + Start + Length,
                Data.begin() + Offset);
    }
    return {};
  }

  /// Get an uint8 array from Data[Offset : Offset + Length - 1]
  Expect<void> getArray(uint8_t *Arr, const uint32_t Offset,
                        const uint32_t Length, const bool IsReverse = false) {
    /// Check memory boundary.
    if (!checkDataSize(Offset, Length)) {
      return Unexpect(ErrCode::MemorySizeExceeded);
    }
    if (Length > 0) {
      /// Copy data.
      if (IsReverse) {
        for (uint32_t I = 0; I < Length; I++) {
          Arr[I] = Data[Offset + Length - I - 1];
        }
      } else {
        std::copy(Data.begin() + Offset, Data.begin() + Offset + Length, Arr);
      }
    }
    return {};
  }

  /// Replace Data[Offset : Offset + Length - 1] to an uint8 array
  Expect<void> setArray(const uint8_t *Arr, const uint32_t Offset,
                        const uint32_t Length, const bool IsReverse = false) {
    /// Check memory boundary.
    if (!checkDataSize(Offset, Length)) {
      return Unexpect(ErrCode::MemorySizeExceeded);
    }
    if (Length > 0) {
      /// Copy data.
      if (IsReverse) {
        for (uint32_t I = 0; I < Length; I++) {
          Data[Offset + Length - I - 1] = Arr[I];
        }
      } else {
        std::copy(Arr, Arr + Length, Data.begin() + Offset);
      }
    }
    return {};
  }

  /// Get pointer to specific offset of memory or null.
  template <typename T>
  typename std::enable_if_t<std::is_pointer_v<T>, T>
  getPointerOrNull(const uint32_t Offset) {
    if (Offset >= Data.size() || Offset == 0) {
      return nullptr;
    }
    return reinterpret_cast<T>(&Data[Offset]);
  }

  /// Get pointer to specific offset of memory.
  template <typename T>
  typename std::enable_if_t<std::is_pointer_v<T>, T>
  getPointer(const uint32_t Offset) {
    if (Offset >= Data.size()) {
      return nullptr;
    }
    return reinterpret_cast<T>(&Data[Offset]);
  }

  /// Template of loading bytes and convert to a value.
  ///
  /// Load the length of vector and construct into a value.
  /// Only output value of int32, uint32, int64, uint64, float, and double are
  /// allowed.
  ///
  /// \param Value the constructed output value.
  /// \param Offset the start offset in data array.
  /// \param Length the load length from data. Need to <= sizeof(T).
  ///
  /// \returns void when success, ErrCode when failed.
  template <typename T>
  typename std::enable_if_t<Support::IsWasmTypeV<T>, Expect<void>>
  loadValue(T &Value, const uint32_t Offset, const uint32_t Length) {
    /// Check data boundary.
    if (Length > sizeof(T)) {
      return Unexpect(ErrCode::AccessForbidMemory);
    }
    /// Check memory boundary.
    if (!checkDataSize(Offset, Length)) {
      return Unexpect(ErrCode::MemorySizeExceeded);
    }
    /// Load data to a value.
    if (Length > 0) {
      if (std::is_floating_point_v<T>) {
        /// Floating case. Do memory copy.
        std::memcpy(&Value, &Data[Offset], sizeof(T));
      } else {
        uint64_t LoadVal = 0;
        /// Integer case. Extends to result type.
        std::memcpy(&LoadVal, &Data[Offset], Length);
        if (std::is_signed_v<T> && (LoadVal >> (Length * 8 - 1))) {
          /// Signed extend.
          for (unsigned int I = Length; I < 8; I++) {
            LoadVal |= 0xFFULL << (I * 8);
          }
        }
        Value = static_cast<T>(LoadVal);
      }
    }
    return {};
  }

  /// Template of loading bytes and convert to a value.
  ///
  /// Destruct and Store the value to length of vector.
  /// Only input value of uint32, uint64, float, and double are allowed.
  ///
  /// \param Value the value want to store into data array.
  /// \param Offset the start offset in data array.
  /// \param Length the store length to data. Need to <= sizeof(T).
  ///
  /// \returns void when success, ErrCode when failed.
  template <typename T>
  typename std::enable_if_t<Support::IsWasmBuiltInV<T>, Expect<void>>
  storeValue(const T &Value, const uint32_t Offset, const uint32_t Length) {
    /// Check data boundary.
    if (Length > sizeof(T)) {
      return Unexpect(ErrCode::AccessForbidMemory);
    }
    /// Check memory boundary.
    if (!checkDataSize(Offset, Length)) {
      return Unexpect(ErrCode::MemorySizeExceeded);
    }
    /// Copy store data to value.
    if (Length > 0) {
      std::memcpy(&Data[Offset], &Value, Length);
    }
    return {};
  }

private:
  /// Check access size is valid and adjust vector.
  bool checkDataSize(uint32_t Offset, uint32_t Length) {
    uint64_t AccessLen =
        static_cast<uint64_t>(Offset) + static_cast<uint64_t>(Length);
    if (AccessLen > CurrPage * 65536ULL) {
      return false;
    }
    /// Note: the vector size will <= CurrPage * 65536
    if (Data.size() < Offset + Length) {
      uint32_t TargetSize = (Offset + Length) / 8 + 1;
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
    return true;
  }

  /// \name Data of memory instance.
  /// @{
  const bool HasMaxPage;
  const uint32_t MinPage;
  const uint32_t MaxPage;
  uint32_t CurrPage;
  Bytes Data;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace SSVM
