// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/runtime/instance/memory.h - Memory Instance definition ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the memory instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/type.h"
#include "common/errcode.h"
#include "common/errinfo.h"
#include "common/spdlog.h"
#include "system/allocator.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <set>
#include <utility>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class MemoryInstance {

public:
  static inline constexpr const uint64_t kPageSize = UINT64_C(65536);
  static inline constexpr const uint64_t k4G = UINT64_C(0x100000000);
  MemoryInstance() = delete;
  MemoryInstance(MemoryInstance &&Inst) noexcept
      : MemType(Inst.MemType), DataPtr(Inst.DataPtr),
        PageLimit(Inst.PageLimit) {
    Inst.DataPtr = nullptr;
  }
  MemoryInstance(const AST::MemoryType &MType,
                 uint32_t PageLim = UINT32_C(65536)) noexcept
      : MemType(MType), PageLimit(PageLim) {
    if (MemType.getLimit().getMin() > PageLimit) {
      spdlog::error("Memory Instance: Limited {} page in configuration.",
                    PageLimit);
      MemType.getLimit().setMin(PageLimit);
    }
    DataPtr = Allocator::allocate(MemType.getLimit().getMin());
    if (DataPtr == nullptr) {
      spdlog::error("Memory Instance: Unable to find usable memory address.");
      MemType.getLimit().setMin(0U);
      return;
    }
  }
  ~MemoryInstance() noexcept {
    Allocator::release(DataPtr, MemType.getLimit().getMin());
  }

  bool isShared() const noexcept { return MemType.getLimit().isShared(); }

  /// Get page size of memory.data
  uint32_t getPageSize() const noexcept {
    // The memory page size is binded with the limit in memory type.
    return MemType.getLimit().getMin();
  }

  /// Getter of memory type.
  const AST::MemoryType &getMemoryType() const noexcept { return MemType; }

  /// Check access size is valid.
  bool checkAccessBound(uint32_t Offset, uint32_t Length) const noexcept {
    const uint64_t AccessLen =
        static_cast<uint64_t>(Offset) + static_cast<uint64_t>(Length);
    return AccessLen <= MemType.getLimit().getMin() * kPageSize;
  }

  /// Get boundary index.
  uint32_t getBoundIdx() const noexcept {
    return MemType.getLimit().getMin() > 0
               ? MemType.getLimit().getMin() * kPageSize - 1
               : 0;
  }

  /// Grow page
  bool growPage(const uint32_t Count) {
    if (Count == 0) {
      return true;
    }
    // Maximum pages count, 65536
    uint32_t MaxPageCaped = k4G / kPageSize;
    uint32_t Min = MemType.getLimit().getMin();
    assuming(MaxPageCaped >= Min);
    if (MemType.getLimit().hasMax()) {
      uint32_t Max = MemType.getLimit().getMax();
      assuming(Max >= Min);
      MaxPageCaped = std::min(Max, MaxPageCaped);
    }
    if (Count > MaxPageCaped - Min) {
      return false;
    }
    assuming(PageLimit >= Min);
    if (Count > PageLimit - Min) {
      spdlog::error("Memory Instance: Memory grow page failed, exceeded "
                    "limited {} page size in configuration.",
                    PageLimit);
      return false;
    }
    if (auto NewPtr = Allocator::resize(DataPtr, Min, Min + Count);
        NewPtr == nullptr) {
      return false;
    } else {
      DataPtr = NewPtr;
    }
    MemType.getLimit().setMin(Min + Count);
    return true;
  }

  /// Get slice of Data[Offset : Offset + Length - 1]
  Expect<Span<Byte>> getBytes(uint32_t Offset, uint32_t Length) const noexcept {
    // Check the memory boundary.
    if (unlikely(!checkAccessBound(Offset, Length))) {
      spdlog::error(ErrCode::Value::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::Value::MemoryOutOfBounds);
    }
    return Span<Byte>(&DataPtr[Offset], Length);
  }

  /// Replace the bytes of Data[Offset :] by Slice[Start : Start + Length - 1]
  Expect<void> setBytes(Span<const Byte> Slice, uint32_t Offset, uint32_t Start,
                        uint32_t Length) noexcept {
    // Check the memory boundary.
    if (unlikely(!checkAccessBound(Offset, Length))) {
      spdlog::error(ErrCode::Value::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::Value::MemoryOutOfBounds);
    }

    // Check the input data validation.
    if (unlikely(static_cast<uint64_t>(Start) + static_cast<uint64_t>(Length) >
                 Slice.size())) {
      spdlog::error(ErrCode::Value::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::Value::MemoryOutOfBounds);
    }

    // Copy the data.
    if (likely(Length > 0)) {
      std::copy(Slice.begin() + Start, Slice.begin() + Start + Length,
                DataPtr + Offset);
    }
    return {};
  }

  /// Fill the bytes of Data[Offset : Offset + Length - 1] by Val.
  Expect<void> fillBytes(uint8_t Val, uint32_t Offset,
                         uint32_t Length) noexcept {
    // Check the memory boundary.
    if (unlikely(!checkAccessBound(Offset, Length))) {
      spdlog::error(ErrCode::Value::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::Value::MemoryOutOfBounds);
    }

    // Copy the data.
    if (likely(Length > 0)) {
      std::fill(DataPtr + Offset, DataPtr + Offset + Length, Val);
    }
    return {};
  }

  /// Get an uint8 array from Data[Offset : Offset + Length - 1]
  Expect<void> getArray(uint8_t *Arr, uint32_t Offset, uint32_t Length,
                        bool IsReverse = false) const noexcept {
    // Check the memory boundary.
    if (unlikely(!checkAccessBound(Offset, Length))) {
      spdlog::error(ErrCode::Value::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::Value::MemoryOutOfBounds);
    }
    if (likely(Length > 0)) {
      // Copy the data.
      if (IsReverse) {
        std::reverse_copy(DataPtr + Offset, DataPtr + Offset + Length, Arr);
      } else {
        std::copy(DataPtr + Offset, DataPtr + Offset + Length, Arr);
      }
    }
    return {};
  }

  /// Replace Data[Offset : Offset + Length - 1] to an uint8 array
  Expect<void> setArray(const uint8_t *Arr, uint32_t Offset, uint32_t Length,
                        bool IsReverse = false) noexcept {
    // Check the memory boundary.
    if (unlikely(!checkAccessBound(Offset, Length))) {
      spdlog::error(ErrCode::Value::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::Value::MemoryOutOfBounds);
    }
    if (likely(Length > 0)) {
      // Copy the data.
      if (IsReverse) {
        std::reverse_copy(Arr, Arr + Length, DataPtr + Offset);
      } else {
        std::copy(Arr, Arr + Length, DataPtr + Offset);
      }
    }
    return {};
  }

  /// Get pointer to specific offset of memory or null.
  template <typename T>
  typename std::enable_if_t<std::is_pointer_v<T>, T>
  getPointerOrNull(uint32_t Offset) const noexcept {
    if (Offset == 0 ||
        unlikely(!checkAccessBound(Offset, sizeof(std::remove_pointer_t<T>)))) {
      return nullptr;
    }
    return reinterpret_cast<T>(&DataPtr[Offset]);
  }

  /// Get pointer to specific offset of memory.
  template <typename T>
  typename std::enable_if_t<std::is_pointer_v<T>, T>
  getPointer(uint32_t Offset) const noexcept {
    using Type = std::remove_pointer_t<T>;
    uint32_t ByteSize = static_cast<uint32_t>(sizeof(Type));
    if (unlikely(!checkAccessBound(Offset, ByteSize))) {
      return nullptr;
    }
    return reinterpret_cast<T>(&DataPtr[Offset]);
  }

  /// Get array of object at specific offset of memory.
  template <typename T>
  Span<T> getSpan(uint32_t Offset, uint32_t Size) const noexcept {
    uint32_t ByteSize = static_cast<uint32_t>(sizeof(T) * Size);
    if (unlikely(!checkAccessBound(Offset, ByteSize))) {
      return Span<T>();
    }
    return Span<T>(reinterpret_cast<T *>(&DataPtr[Offset]), Size);
  }

  /// Get array of object at specific offset of memory.
  std::string_view getStringView(uint32_t Offset,
                                 uint32_t Size) const noexcept {
    if (unlikely(!checkAccessBound(Offset, Size))) {
      return {};
    }
    return {reinterpret_cast<const char *>(&DataPtr[Offset]), Size};
  }

  /// Template of loading bytes and convert to a value.
  ///
  /// Load the length of vector and construct into a value.
  /// Only output value of int32, uint32, int64, uint64, float, and double are
  /// allowed.
  ///
  /// \param Value the constructed output value.
  /// \param Offset the start offset in data array.
  ///
  /// \returns void when success, ErrCode when failed.
  template <typename T, uint32_t Length = sizeof(T)>
  typename std::enable_if_t<IsWasmNumV<T>, Expect<void>>
  loadValue(T &Value, uint32_t Offset) const noexcept {
    // Check the data boundary.
    static_assert(Length <= sizeof(T));
    // Check the memory boundary.
    if (unlikely(!checkAccessBound(Offset, Length))) {
      spdlog::error(ErrCode::Value::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::Value::MemoryOutOfBounds);
    }
    // Load the data to the value.
    if (likely(Length > 0)) {
      if constexpr (std::is_floating_point_v<T>) {
        // Floating case. Do the memory copy.
        std::memcpy(&Value, &DataPtr[Offset], sizeof(T));
      } else {
        if constexpr (sizeof(T) > 8) {
          assuming(sizeof(T) == 16);
          Value = 0;
          std::memcpy(&Value, &DataPtr[Offset], Length);
        } else {
          uint64_t LoadVal = 0;
          // Integer case. Extends to the result type.
          std::memcpy(&LoadVal, &DataPtr[Offset], Length);
          if (std::is_signed_v<T> && (LoadVal >> (Length * 8 - 1))) {
            // Signed extension.
            for (unsigned int I = Length; I < 8; I++) {
              LoadVal |= 0xFFULL << (I * 8);
            }
          }
          Value = static_cast<T>(LoadVal);
        }
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
  ///
  /// \returns void when success, ErrCode when failed.
  template <typename T, uint32_t Length = sizeof(T)>
  typename std::enable_if_t<IsWasmNativeNumV<T>, Expect<void>>
  storeValue(const T &Value, uint32_t Offset) noexcept {
    // Check the data boundary.
    static_assert(Length <= sizeof(T));
    // Check the memory boundary.
    if (unlikely(!checkAccessBound(Offset, Length))) {
      spdlog::error(ErrCode::Value::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::Value::MemoryOutOfBounds);
    }
    // Copy the stored data to the value.
    if (likely(Length > 0)) {
      std::memcpy(&DataPtr[Offset], &Value, Length);
    }
    return {};
  }

  uint8_t *getDataPtr() const noexcept { return DataPtr; }

private:
  /// \name Data of memory instance.
  /// @{
  AST::MemoryType MemType;
  uint8_t *DataPtr = nullptr;
  const uint32_t PageLimit;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
