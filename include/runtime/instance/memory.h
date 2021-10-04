// SPDX-License-Identifier: Apache-2.0
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
#include "common/log.h"
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
                 const uint32_t PageLim = UINT32_C(65536)) noexcept
      : MemType(MType), PageLimit(PageLim) {
    if (MemType.getLimit().getMin() > PageLimit) {
      spdlog::error(
          "Create memory instance failed -- exceeded limit page size: {}",
          PageLimit);
      return;
    }
    DataPtr = Allocator::allocate(MemType.getLimit().getMin());
    if (DataPtr == nullptr) {
      spdlog::error("Unable to find usable memory address");
      return;
    }
  }
  ~MemoryInstance() noexcept {
    Allocator::release(DataPtr, MemType.getLimit().getMin());
  }

  /// Get page size of memory.data
  uint32_t getDataPageSize() const noexcept {
    return MemType.getLimit().getMin();
  }

  /// Getter of limit definition.
  bool getHasMax() const noexcept { return MemType.getLimit().hasMax(); }

  /// Getter of limit definition.
  uint32_t getMin() const noexcept { return MemType.getLimit().getMin(); }

  /// Getter of limit definition.
  uint32_t getMax() const noexcept { return MemType.getLimit().getMax(); }

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
    /// Maximum pages count, 65536
    uint32_t MaxPageCaped = k4G / kPageSize;
    uint32_t Min = MemType.getLimit().getMin();
    uint32_t Max = MemType.getLimit().getMax();
    if (MemType.getLimit().hasMax()) {
      MaxPageCaped = std::min(Max, MaxPageCaped);
    }
    if (Count + Min > MaxPageCaped) {
      return false;
    }
    if (Count + Min > PageLimit) {
      spdlog::error("Memory grow page failed -- exceeded limit page size: {}",
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
  Expect<Span<Byte>> getBytes(const uint32_t Offset,
                              const uint32_t Length) const noexcept {
    /// Check memory boundary.
    if (!checkAccessBound(Offset, Length)) {
      spdlog::error(ErrCode::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::MemoryOutOfBounds);
    }
    return Span<Byte>(&DataPtr[Offset], Length);
  }

  /// Replace the bytes of Data[Offset :] by Slice[Start : Start + Legnth - 1]
  Expect<void> setBytes(Span<const Byte> Slice, const uint32_t Offset,
                        const uint32_t Start, const uint32_t Length) {
    /// Check memory boundary.
    if (!checkAccessBound(Offset, Length)) {
      spdlog::error(ErrCode::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::MemoryOutOfBounds);
    }

    /// Check input data validation.
    if (Start + Length > Slice.size()) {
      spdlog::error(ErrCode::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::MemoryOutOfBounds);
    }

    /// Copy data.
    if (Length > 0) {
      std::copy(Slice.begin() + Start, Slice.begin() + Start + Length,
                DataPtr + Offset);
    }
    return {};
  }

  /// Fill the bytes of Data[Offset : Offset + Length - 1] by Val.
  Expect<void> fillBytes(const uint8_t Val, const uint32_t Offset,
                         const uint32_t Length) {
    /// Check memory boundary.
    if (!checkAccessBound(Offset, Length)) {
      spdlog::error(ErrCode::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::MemoryOutOfBounds);
    }

    /// Copy data.
    if (Length > 0) {
      std::fill(DataPtr + Offset, DataPtr + Offset + Length, Val);
    }
    return {};
  }

  /// Get an uint8 array from Data[Offset : Offset + Length - 1]
  Expect<void> getArray(uint8_t *Arr, const uint32_t Offset,
                        const uint32_t Length,
                        const bool IsReverse = false) const noexcept {
    /// Check memory boundary.
    if (!checkAccessBound(Offset, Length)) {
      spdlog::error(ErrCode::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::MemoryOutOfBounds);
    }
    if (Length > 0) {
      /// Copy data.
      if (IsReverse) {
        std::reverse_copy(DataPtr + Offset, DataPtr + Offset + Length, Arr);
      } else {
        std::copy(DataPtr + Offset, DataPtr + Offset + Length, Arr);
      }
    }
    return {};
  }

  /// Replace Data[Offset : Offset + Length - 1] to an uint8 array
  Expect<void> setArray(const uint8_t *Arr, const uint32_t Offset,
                        const uint32_t Length, const bool IsReverse = false) {
    /// Check memory boundary.
    if (!checkAccessBound(Offset, Length)) {
      spdlog::error(ErrCode::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::MemoryOutOfBounds);
    }
    if (Length > 0) {
      /// Copy data.
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
  getPointerOrNull(const uint32_t Offset) const {
    if (Offset == 0 ||
        !checkAccessBound(Offset, sizeof(std::remove_pointer_t<T>))) {
      return nullptr;
    }
    return reinterpret_cast<T>(&DataPtr[Offset]);
  }

  /// Get pointer to specific offset of memory.
  template <typename T>
  typename std::enable_if_t<std::is_pointer_v<T>, T>
  getPointer(const uint32_t Offset, const uint32_t Size = 1) const {
    using Type = std::remove_pointer_t<T>;
    uint32_t ByteSize = static_cast<uint32_t>(sizeof(Type)) * Size;
    if (!checkAccessBound(Offset, ByteSize)) {
      return nullptr;
    }
    return reinterpret_cast<T>(&DataPtr[Offset]);
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
  typename std::enable_if_t<IsWasmNumV<T>, Expect<void>>
  loadValue(T &Value, const uint32_t Offset,
            const uint32_t Length) const noexcept {
    /// Check data boundary.
    if (Length > sizeof(T)) {
      spdlog::error(ErrCode::MemoryOutOfBounds);
      spdlog::error(
          ErrInfo::InfoBoundary(Offset, Length, Offset + sizeof(T) - 1));
      return Unexpect(ErrCode::MemoryOutOfBounds);
    }
    /// Check memory boundary.
    if (!checkAccessBound(Offset, Length)) {
      spdlog::error(ErrCode::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::MemoryOutOfBounds);
    }
    /// Load data to a value.
    if (Length > 0) {
      if (std::is_floating_point_v<T>) {
        /// Floating case. Do memory copy.
        std::memcpy(&Value, &DataPtr[Offset], sizeof(T));
      } else {
        if constexpr (sizeof(T) > 8) {
          static_assert(sizeof(T) == 16);
          Value = 0;
          std::memcpy(&Value, &DataPtr[Offset], Length);
        } else {
          uint64_t LoadVal = 0;
          /// Integer case. Extends to result type.
          std::memcpy(&LoadVal, &DataPtr[Offset], Length);
          if (std::is_signed_v<T> && (LoadVal >> (Length * 8 - 1))) {
            /// Signed extend.
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
  /// \param Length the store length to data. Need to <= sizeof(T).
  ///
  /// \returns void when success, ErrCode when failed.
  template <typename T>
  typename std::enable_if_t<IsWasmNativeNumV<T>, Expect<void>>
  storeValue(const T &Value, const uint32_t Offset, const uint32_t Length) {
    /// Check data boundary.
    if (Length > sizeof(T)) {
      spdlog::error(ErrCode::MemoryOutOfBounds);
      spdlog::error(
          ErrInfo::InfoBoundary(Offset, Length, Offset + sizeof(T) - 1));
      return Unexpect(ErrCode::MemoryOutOfBounds);
    }
    /// Check memory boundary.
    if (!checkAccessBound(Offset, Length)) {
      spdlog::error(ErrCode::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::MemoryOutOfBounds);
    }
    /// Copy store data to value.
    if (Length > 0) {
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
