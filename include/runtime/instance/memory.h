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
#include "common/int128.h"
#include "common/spdlog.h"
#include "common/types.h"
#include "system/allocator.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <set>
#include <type_traits>
#include <utility>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class MemoryInstance {

public:
  static inline constexpr const addr_t kPageSize = UINT64_C(65536);
  static inline constexpr const addr_t kPageLimit32 = UINT64_C(0x10000);
  static inline constexpr const addr_t kPageLimit64 = UINT64_C(0x1000000000000);
  MemoryInstance() = delete;
  MemoryInstance(MemoryInstance &&Inst) noexcept
      : MemType(Inst.MemType), DataPtr(Inst.DataPtr),
        PageLimit(Inst.PageLimit) {
    Inst.DataPtr = nullptr;
  }
  MemoryInstance(const AST::MemoryType &MType,
                 addr_t PageLim = kPageLimit64) noexcept
      : MemType(MType), PageLimit(PageLim) {
    using namespace std::literals;
    if (MemType.getLimit().getMin() > PageLimit) {
      spdlog::error("Memory Instance: Limited {} page in configuration."sv,
                    PageLimit);
      MemType.getLimit().setMin(PageLimit);
    }
    DataPtr = Allocator::allocate(MemType.getLimit().getMin());
    if (DataPtr == nullptr) {
      spdlog::error("Memory Instance: Unable to find usable memory address."sv);
      MemType.getLimit().setMin(0U);
      return;
    }
  }
  ~MemoryInstance() noexcept {
    Allocator::release(DataPtr, MemType.getLimit().getMin());
  }

  bool isShared() const noexcept { return MemType.getLimit().isShared(); }

  /// Get page size of memory.data
  addr_t getPageSize() const noexcept {
    // The memory page size is binded with the limit in memory type.
    return MemType.getLimit().getMin();
  }

  /// Get memory size of memory.data
  addr_t getSize() const noexcept {
    // The memory page size is binded with the limit in memory type.
    return MemType.getLimit().getMin() * kPageSize;
  }

  /// Getter of memory type.
  const AST::MemoryType &getMemoryType() const noexcept { return MemType; }

  /// Check access size is valid.
  bool checkAccessBound(const addr_t Offset,
                        const addr_t Length) const noexcept {
    // Due to applying the Memory64 proposal, we should avoid the overflow issue
    // of the following code:
    //   return Offset + Length <= Limit;
    const addr_t Limit = MemType.getLimit().getMin() * kPageSize;
    return std::numeric_limits<addr_t>::max() - Offset >= Length &&
           Offset + Length <= Limit;
  }

  /// Grow page
  bool growPage(const addr_t Count) noexcept {
    if (Count == 0) {
      return true;
    }
    uint64_t MaxPageCaped = MemType.getPageLimit();
    uint64_t Min = MemType.getLimit().getMin();
    uint64_t Max = MemType.getLimit().getMax();
    if (MemType.getLimit().hasMax()) {
      Max = MemType.getLimit().getMax();
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
  Expect<Span<Byte>> getBytes(const addr_t Offset,
                              const addr_t Length) const noexcept {
    // Check the memory boundary.
    if (unlikely(!checkAccessBound(Offset, Length))) {
      spdlog::error(ErrCode::Value::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getSize()));
      return Unexpect(ErrCode::Value::MemoryOutOfBounds);
    }
    return Span<Byte>(&DataPtr[Offset], Length);
  }

  /// Replace the bytes of Data[Offset :] by Slice[Start : Start + Length - 1]
  Expect<void> setBytes(Span<const Byte> Slice, const addr_t Offset,
                        const addr_t Start, const addr_t Length) noexcept {
    // Check the memory boundary.
    if (unlikely(!checkAccessBound(Offset, Length))) {
      spdlog::error(ErrCode::Value::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getSize()));
      return Unexpect(ErrCode::Value::MemoryOutOfBounds);
    }

    // Check the input data validation.
    if (unlikely(std::numeric_limits<uint64_t>::max() - Start < Length ||
                 Start + Length > static_cast<uint64_t>(Slice.size()))) {
      spdlog::error(ErrCode::Value::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Start, Length,
                                          static_cast<uint64_t>(Slice.size())));
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
  Expect<void> fillBytes(const uint8_t Val, const addr_t Offset,
                         const addr_t Length) noexcept {
    // Check the memory boundary.
    if (unlikely(!checkAccessBound(Offset, Length))) {
      spdlog::error(ErrCode::Value::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getSize()));
      return Unexpect(ErrCode::Value::MemoryOutOfBounds);
    }

    // Copy the data.
    if (likely(Length > 0)) {
      std::fill(DataPtr + Offset, DataPtr + Offset + Length, Val);
    }
    return {};
  }

  /// Get an uint8 array from Data[Offset : Offset + Length - 1]
  Expect<void> getArray(uint8_t *Arr, const addr_t Offset, const addr_t Length,
                        const bool IsReverse = false) const noexcept {
    // Check the memory boundary.
    if (unlikely(!checkAccessBound(Offset, Length))) {
      spdlog::error(ErrCode::Value::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getSize()));
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
  Expect<void> setArray(const uint8_t *Arr, const addr_t Offset,
                        const addr_t Length,
                        const bool IsReverse = false) noexcept {
    // Check the memory boundary.
    if (unlikely(!checkAccessBound(Offset, Length))) {
      spdlog::error(ErrCode::Value::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getSize()));
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
  getPointerOrNull(const addr_t Offset) const noexcept {
    using Type = std::remove_pointer_t<T>;
    if (Offset == 0 || unlikely(!checkAccessBound(Offset, sizeof(Type)))) {
      return nullptr;
    }
    return reinterpret_cast<T>(&DataPtr[Offset]);
  }

  /// Get pointer to specific offset of memory.
  template <typename T>
  typename std::enable_if_t<std::is_pointer_v<T>, T>
  getPointer(const addr_t Offset) const noexcept {
    using Type = std::remove_pointer_t<T>;
    if (unlikely(!checkAccessBound(Offset, sizeof(Type)))) {
      return nullptr;
    }
    return reinterpret_cast<T>(&DataPtr[Offset]);
  }

  /// Get array of object with count at specific offset of memory.
  template <typename T>
  Span<T> getSpan(const addr_t Offset, const addr_t Count) const noexcept {
    addr_t Size;
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
    uint128_t Num =
        static_cast<uint128_t>(sizeof(T)) * static_cast<uint128_t>(Count);
    if ((Num >> 64) != 0) {
      return Span<T>();
    }
    Size = static_cast<addr_t>(Num);
#else
    if (unlikely(__builtin_mul_overflow(static_cast<addr_t>(sizeof(T)), Count,
                                        &Size))) {
      return Span<T>();
    }
#endif
    if (unlikely(!checkAccessBound(Offset, Size))) {
      return Span<T>();
    }
    return Span<T>(reinterpret_cast<T *>(&DataPtr[Offset]), Count);
  }

  /// Get array of object at specific offset of memory.
  std::string_view getStringView(const addr_t Offset,
                                 const addr_t Size) const noexcept {
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
  loadValue(T &Value, const addr_t Offset) const noexcept {
    // Check the data boundary.
    static_assert(Length <= sizeof(T));
    // Check the memory boundary.
    if (unlikely(!checkAccessBound(Offset, Length))) {
      spdlog::error(ErrCode::Value::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getSize()));
      return Unexpect(ErrCode::Value::MemoryOutOfBounds);
    }
    // Load the data to the value.
    if (likely(Length > 0)) {
      if constexpr (std::is_floating_point_v<T>) {
        // Floating case. Do the memory copy.
        EndianValue<T> LoadValue;
        std::memcpy(&LoadValue.raw(), &DataPtr[Offset], Length);
        Value = LoadValue.le();
      } else {
        if constexpr (sizeof(T) > 8) {
          assuming(sizeof(T) == 16);
          EndianValue<T> LoadValue = 0U;
          std::memcpy(&LoadValue.raw(), &DataPtr[Offset], Length);
          Value = LoadValue.le();
        } else {
          // Integer case. Extends to the result type.
          EndianValue<uint64_t> LoadVal = 0;
          std::memcpy(&LoadVal.raw(), &DataPtr[Offset], Length);
          uint64_t Val = LoadVal.le();
          if (std::is_signed_v<T> && (Val >> (Length * 8 - 1))) {
            // Signed extension.
            for (unsigned int I = Length; I < 8; I++) {
              Val |= 0xFFULL << (I * 8);
            }
          }
          Value = static_cast<T>(Val);
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
  storeValue(const T &Value, const addr_t Offset) noexcept {
    // Check the data boundary.
    static_assert(Length <= sizeof(T));
    // Check the memory boundary.
    if (unlikely(!checkAccessBound(Offset, Length))) {
      spdlog::error(ErrCode::Value::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getSize()));
      return Unexpect(ErrCode::Value::MemoryOutOfBounds);
    }
    // Copy the stored data to the value.
    if (likely(Length > 0)) {
      T StoreValue = EndianValue<T>(Value).le();
      std::memcpy(&DataPtr[Offset], &StoreValue, Length);
    }
    return {};
  }

  uint8_t *const &getDataPtr() const noexcept { return DataPtr; }
  uint8_t *&getDataPtr() noexcept { return DataPtr; }

private:
  /// \name Data of memory instance.
  /// @{
  AST::MemoryType MemType;
  uint8_t *DataPtr = nullptr;
  addr_t PageLimit;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
