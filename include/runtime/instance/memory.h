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
#include "support/span.h"

#include <algorithm>
#include <boost/align/aligned_allocator.hpp>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

namespace SSVM {
namespace Runtime {
namespace Instance {

class MemoryInstance {
public:
  static inline constexpr const uint64_t kPageSize = UINT64_C(65536);
  MemoryInstance() = delete;
  MemoryInstance(const AST::Limit &Lim)
      : HasMaxPage(Lim.hasMax()), MinPage(Lim.getMin()), MaxPage(Lim.getMax()),
        CurrPage(Lim.getMin()), Data(CurrPage * kPageSize) {}
  virtual ~MemoryInstance() noexcept = default;

  /// Get page size of memory.data
  uint32_t getDataPageSize() const noexcept { return CurrPage; }

  /// Getter of limit definition.
  bool getHasMax() const noexcept { return HasMaxPage; }

  /// Getter of limit definition.
  uint32_t getMin() const noexcept { return MinPage; }

  /// Getter of limit definition.
  uint32_t getMax() const noexcept { return MaxPage; }

  /// Check is out of bound.
  bool checkAccessBound(const uint32_t Offset) noexcept {
    return checkDataSize(Offset, 0);
  }

  /// Grow page
  Expect<void> growPage(const uint32_t Count) {
    uint32_t MaxPageCaped = UINT32_C(65536);
    if (HasMaxPage) {
      MaxPageCaped = std::min(MaxPage, MaxPageCaped);
    }
    if (Count + CurrPage > MaxPageCaped) {
      return Unexpect(ErrCode::MemoryOutOfBounds);
    }
    CurrPage += Count;
    Data.resize(CurrPage * kPageSize);
    if (Symbol) {
      *Symbol = Data.data();
    }
    return {};
  }

  /// Get slice of Data[Offset : Offset + Length - 1]
  Expect<Span<Byte>> getBytes(const uint32_t Offset, const uint32_t Length) {
    /// Check memory boundary.
    if (!checkDataSize(Offset, Length)) {
      return Unexpect(ErrCode::MemoryOutOfBounds);
    }
    return Span<Byte>(&Data[Offset], Length);
  }

  /// Replace the bytes of Data[Offset :] by Slice[Start : Start + Legnth - 1]
  Expect<void> setBytes(Span<const Byte> Slice, const uint32_t Offset,
                        const uint32_t Start, const uint32_t Length) {
    /// Check memory boundary.
    if (!checkDataSize(Offset, Length)) {
      return Unexpect(ErrCode::MemoryOutOfBounds);
    }

    /// Check input data validation.
    if ((Slice.size() > 0 && Start >= Slice.size()) ||
        Start + Length > Slice.size()) {
      return Unexpect(ErrCode::MemoryOutOfBounds);
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
      return Unexpect(ErrCode::MemoryOutOfBounds);
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
      return Unexpect(ErrCode::MemoryOutOfBounds);
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
    if (Offset == 0 ||
        !checkDataSize(Offset, sizeof(std::remove_pointer_t<T>))) {
      return nullptr;
    }
    return reinterpret_cast<T>(&Data[Offset]);
  }

  /// Get pointer to specific offset of memory.
  template <typename T>
  typename std::enable_if_t<std::is_pointer_v<T>, T>
  getPointer(const uint32_t Offset, const uint32_t Size = 1) {
    using Type = std::remove_pointer_t<T>;
    size_t ByteSize = sizeof(Type) * Size;
    if (!checkDataSize(Offset, ByteSize)) {
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
      return Unexpect(ErrCode::MemoryOutOfBounds);
    }
    /// Check memory boundary.
    if (!checkDataSize(Offset, Length)) {
      return Unexpect(ErrCode::MemoryOutOfBounds);
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
      return Unexpect(ErrCode::MemoryOutOfBounds);
    }
    /// Check memory boundary.
    if (!checkDataSize(Offset, Length)) {
      return Unexpect(ErrCode::MemoryOutOfBounds);
    }
    /// Copy store data to value.
    if (Length > 0) {
      std::memcpy(&Data[Offset], &Value, Length);
    }
    return {};
  }

  /// Getter of symbol
  void *getSymbol() const { return Symbol; }
  /// Setter of symbol
  void setSymbol(void *S) {
    Symbol = reinterpret_cast<uint8_t **>(S);
    *Symbol = Data.data();
  }

private:
  /// Check access size is valid and adjust vector.
  bool checkDataSize(uint32_t Offset, uint32_t Length) const noexcept {
    const uint64_t AccessLen =
        static_cast<uint64_t>(Offset) + static_cast<uint64_t>(Length);
    return AccessLen <= Data.size();
  }

  /// \name Data of memory instance.
  /// @{
  const bool HasMaxPage;
  const uint32_t MinPage;
  const uint32_t MaxPage;
  uint32_t CurrPage;
  std::vector<Byte, boost::alignment::aligned_allocator<Byte, 65536>> Data;
  uint8_t **Symbol = nullptr;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace SSVM
