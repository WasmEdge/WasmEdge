// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/refheap.h - Reference heap -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the process-wide heap of the objects that a reference
/// can point to.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/defines.h"
#include "common/errcode.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Runtime {

/// Kind of the object in a heap slot.
enum class HeapKind : uint8_t {
  Free = 0,
  Function,
  Exception,
  ExternBox,
  Struct,
  Array,
};

/// Header at the start of every heap slot.
struct alignas(16) HeapHeader {
  HeapKind Kind;
  std::atomic<uint8_t> Live;
  uint16_t Reserved0;
  uint32_t SelfOffset;
  uint32_t TypeId;
  uint32_t Length;
  /// Owner count of a SharedHeapPtr.
  std::atomic<uint32_t> RefCount;
  /// Reserved for the garbage collector. The fields keep their place because
  /// compiled code reads the header at fixed positions.
  uint32_t AllNext;
  uint32_t GrayNext;
  uint32_t Reserved1;
};
static_assert(sizeof(HeapHeader) == 32);
static_assert(std::atomic<uint8_t>::is_always_lock_free);
static_assert(std::atomic<uint32_t>::is_always_lock_free);
static_assert(sizeof(std::atomic<uint32_t>) == sizeof(uint32_t));
static_assert(alignof(std::atomic<uint32_t>) == alignof(uint32_t));
static_assert(std::atomic<uint8_t *>::is_always_lock_free);
static_assert(sizeof(std::atomic<uint8_t *>) == sizeof(uint8_t *));
static_assert(alignof(std::atomic<uint8_t *>) == alignof(uint8_t *));

/// Map a C++ type to the kind of its heap slot.
template <typename T> struct HeapTraits;

template <typename T> class UniqueHeapPtr;
template <typename T> class SharedHeapPtr;

/// Heap with a data region and an object region, addressed by 32-bit offsets.
class RefHeap {
public:
  static constexpr uint32_t kTagI31 = 1;
  static constexpr uint32_t kTagObject = 2;
  static constexpr uint32_t kBlockSize = 65536;
  static constexpr uint32_t kPartSize = UINT32_C(1) << 30;

  struct Limits {
    uint64_t DataBytes = UINT64_C(1) << 32;
    uint64_t PartBytes = UINT64_C(1) << 30;
  };

  /// Slot size of an object region kind, or 0 for a data region kind.
  static constexpr uint32_t getSlotSize(HeapKind Kind) noexcept {
    switch (Kind) {
    case HeapKind::Function:
      return 256;
    case HeapKind::Exception:
      return 128;
    case HeapKind::ExternBox:
      return 64;
    default:
      return 0;
    }
  }

  /// Tagged offset of the first slot of an object region kind.
  static constexpr uint32_t getPartBase(HeapKind Kind) noexcept {
    switch (Kind) {
    case HeapKind::Function:
      return (UINT32_C(1) << 30) | kTagObject;
    case HeapKind::Exception:
      return (UINT32_C(2) << 30) | kTagObject;
    case HeapKind::ExternBox:
      return (UINT32_C(3) << 30) | kTagObject;
    default:
      return 0;
    }
  }

  static constexpr bool isObjectKind(HeapKind Kind) noexcept {
    return getSlotSize(Kind) != 0;
  }

  /// Create a heap. Fails when the host has no flat mode or the reservation
  /// fails.
  static Expect<std::unique_ptr<RefHeap>> create(const Limits &L) noexcept;

  /// The heap shared by the whole process, or null if it cannot be created.
  /// It is never destroyed.
  WASMEDGE_EXPORT static RefHeap *getGlobal() noexcept;

  ~RefHeap() noexcept;
  RefHeap(const RefHeap &) = delete;
  RefHeap &operator=(const RefHeap &) = delete;

  /// Allocate a slot of `Size` bytes with the header. Returns its offset.
  Expect<uint32_t> allocate(HeapKind Kind, uint64_t Size) noexcept;

  /// Free the slot at `Off`. The caller already destroyed the object.
  void free(uint32_t Off) noexcept;

  /// Header of the live slot at `Off`, or null.
  HeapHeader *getHeader(uint32_t Off) const noexcept;

  /// Object of the live slot of `Kind` at `Off`, or null.
  void *decodeObject(HeapKind Kind, uint32_t Off) const noexcept;

  /// Offset of the live slot whose object starts at `Obj`, or 0.
  uint32_t getOffset(const void *Obj) const noexcept;

  /// Checked address of `Size` bytes at `Pos` in the live data slot at `Off`.
  Expect<uint8_t *> getDataSpan(uint32_t Off, uint32_t Pos,
                                uint32_t Size) const noexcept;

  template <typename T>
  Expect<T> load(uint32_t Off, uint32_t Pos) const noexcept {
    static_assert(std::is_trivially_copyable_v<T>);
    EXPECTED_TRY(auto *P, getDataSpan(Off, Pos, sizeof(T)));
    T V;
    std::memcpy(&V, P, sizeof(T));
    return V;
  }

  template <typename T>
  Expect<void> store(uint32_t Off, uint32_t Pos, T V) const noexcept {
    static_assert(std::is_trivially_copyable_v<T>);
    EXPECTED_TRY(auto *P, getDataSpan(Off, Pos, sizeof(T)));
    std::memcpy(P, &V, sizeof(T));
    return {};
  }

  /// Construct a `T` in a new slot, with `TailBytes` more bytes after it.
  template <typename T, typename... ArgsT>
  Expect<UniqueHeapPtr<T>> makeUniqueWithTail(uint32_t TailBytes,
                                              ArgsT &&...Args) noexcept;
  template <typename T, typename... ArgsT>
  Expect<UniqueHeapPtr<T>> makeUnique(ArgsT &&...Args) noexcept {
    return makeUniqueWithTail<T>(0, std::forward<ArgsT>(Args)...);
  }
  template <typename T, typename... ArgsT>
  Expect<SharedHeapPtr<T>> makeSharedWithTail(uint32_t TailBytes,
                                              ArgsT &&...Args) noexcept;
  template <typename T, typename... ArgsT>
  Expect<SharedHeapPtr<T>> makeShared(ArgsT &&...Args) noexcept {
    return makeSharedWithTail<T>(0, std::forward<ArgsT>(Args)...);
  }

private:
  template <typename T> friend class UniqueHeapPtr;
  template <typename T> friend class SharedHeapPtr;

  template <typename T, typename... ArgsT>
  Expect<std::pair<T *, uint32_t>> construct(uint32_t TailBytes,
                                             ArgsT &&...Args) noexcept {
    constexpr HeapKind Kind = HeapTraits<T>::Kind;
    static_assert(alignof(T) <= alignof(HeapHeader));
    static_assert(std::is_nothrow_constructible_v<T, ArgsT...>);
    if constexpr (isObjectKind(Kind)) {
      static_assert(sizeof(HeapHeader) + sizeof(T) <= getSlotSize(Kind));
    }
    EXPECTED_TRY(const uint32_t Off,
                 allocateSlot(Kind,
                              sizeof(HeapHeader) + sizeof(T) +
                                  static_cast<uint64_t>(TailBytes),
                              false));
    uint8_t *Slot =
        isObjectKind(Kind) ? ObjectBase + (Off & ~kTagObject) : DataBase + Off;
    T *Ptr = new (Slot + sizeof(HeapHeader)) T(std::forward<ArgsT>(Args)...);
    if constexpr (isObjectKind(Kind)) {
      setLive(Off, true);
    }
    return std::make_pair(Ptr, Off);
  }

  template <typename T> void destroy(T *Ptr, uint32_t Off) noexcept {
    if constexpr (isObjectKind(HeapTraits<T>::Kind)) {
      setLive(Off, false);
    }
    Ptr->~T();
    free(Off);
  }

  static constexpr uint32_t kMinClassShift = 5;
  static constexpr uint32_t kMaxClassShift = 15;
  static constexpr uint32_t kClassCount = kMaxClassShift - kMinClassShift + 1;
  static constexpr uint32_t kPartCount = 3;

  struct Block {
    /// First block of the slot in the high half, slot size in the low half.
    std::atomic<uint64_t> Info = 0;
    std::atomic<std::atomic<uint64_t> *> LiveBits = nullptr;
  };

  struct Part {
    uint32_t Base = 0;
    uint32_t SlotSize = 0;
    std::atomic<uint32_t> Top = 0;
    uint32_t Committed = 0;
    std::vector<uint32_t> FreeList;
    std::mutex Mutex;
  };

  RefHeap(const Limits &L) noexcept;

  static uint32_t getPartIndex(HeapKind Kind) noexcept {
    return static_cast<uint32_t>(Kind) -
           static_cast<uint32_t>(HeapKind::Function);
  }
  static uint32_t getClassIndex(uint64_t Size) noexcept;

  Expect<uint32_t> allocateSlot(HeapKind Kind, uint64_t Size,
                                bool Live) noexcept;
  Expect<uint32_t> allocateObject(HeapKind Kind, uint64_t Size,
                                  bool Live) noexcept;
  Expect<uint32_t> allocateData(HeapKind Kind, uint64_t Size) noexcept;
  Expect<uint32_t> takeBlocks(uint32_t Count) noexcept;
  void setupBlocks(uint32_t Start, uint32_t Count, uint32_t SlotSize);
  void freeObject(uint32_t Off) noexcept;
  void freeData(uint32_t Off) noexcept;
  void initHeader(uint8_t *Slot, HeapKind Kind, uint32_t Off,
                  bool Live) noexcept;
  void setLive(uint32_t Off, bool Live) noexcept;

  /// Part of the object slot start at `Off`, live or not, or null.
  const Part *findSlot(uint32_t Off) const noexcept;
  /// Part of the live object slot at `Off`, or null.
  const Part *findPart(uint32_t Off) const noexcept;
  /// Index of the live bit of the data slot at `Off`, or false.
  bool findDataSlot(uint32_t Off, uint32_t &BlockIdx, uint32_t &SlotIdx,
                    uint32_t &SlotSize) const noexcept;

  uint8_t *DataBase = nullptr;
  uint8_t *ObjectBase = nullptr;
  uint32_t DataBlocks = 0;
  uint32_t PartLimit = 0;
  std::unique_ptr<Block[]> Blocks;
  std::vector<std::unique_ptr<std::atomic<uint64_t>[]>> Bitmaps;
  std::mutex DataMutex;
  uint32_t DataTop = 0;
  uint32_t DataCommitted = 0;
  std::multimap<uint32_t, uint32_t> FreeRuns;
  std::vector<uint32_t> ClassFree[kClassCount];
  Part Parts[kPartCount];
};

/// Owning handle of a heap object. It cannot be copied.
template <typename T> class UniqueHeapPtr {
public:
  UniqueHeapPtr() noexcept = default;
  UniqueHeapPtr(std::nullptr_t) noexcept {}
  UniqueHeapPtr(UniqueHeapPtr &&Other) noexcept
      : Heap(std::exchange(Other.Heap, nullptr)),
        Ptr(std::exchange(Other.Ptr, nullptr)),
        Off(std::exchange(Other.Off, 0)) {}
  UniqueHeapPtr &operator=(UniqueHeapPtr &&Other) noexcept {
    if (this != &Other) {
      reset();
      Heap = std::exchange(Other.Heap, nullptr);
      Ptr = std::exchange(Other.Ptr, nullptr);
      Off = std::exchange(Other.Off, 0);
    }
    return *this;
  }
  UniqueHeapPtr(const UniqueHeapPtr &) = delete;
  UniqueHeapPtr &operator=(const UniqueHeapPtr &) = delete;
  ~UniqueHeapPtr() noexcept { reset(); }

  T *get() const noexcept { return Ptr; }
  T &operator*() const noexcept { return *Ptr; }
  T *operator->() const noexcept { return Ptr; }
  explicit operator bool() const noexcept { return Ptr != nullptr; }
  uint32_t getOffset() const noexcept { return Off; }

  void reset() noexcept {
    if (Ptr != nullptr) {
      Heap->destroy(std::exchange(Ptr, nullptr), std::exchange(Off, 0));
      Heap = nullptr;
    }
  }

private:
  friend class RefHeap;
  UniqueHeapPtr(RefHeap *H, T *P, uint32_t O) noexcept
      : Heap(H), Ptr(P), Off(O) {}

  RefHeap *Heap = nullptr;
  T *Ptr = nullptr;
  uint32_t Off = 0;
};

/// Shared handle of a heap object. The count lives in the slot header.
template <typename T> class SharedHeapPtr {
public:
  SharedHeapPtr() noexcept = default;
  SharedHeapPtr(std::nullptr_t) noexcept {}
  SharedHeapPtr(const SharedHeapPtr &Other) noexcept
      : Heap(Other.Heap), Ptr(Other.Ptr), Off(Other.Off) {
    if (Ptr != nullptr) {
      getHeader()->RefCount.fetch_add(1, std::memory_order_relaxed);
    }
  }
  SharedHeapPtr(SharedHeapPtr &&Other) noexcept
      : Heap(std::exchange(Other.Heap, nullptr)),
        Ptr(std::exchange(Other.Ptr, nullptr)),
        Off(std::exchange(Other.Off, 0)) {}
  SharedHeapPtr &operator=(const SharedHeapPtr &Other) noexcept {
    SharedHeapPtr(Other).swap(*this);
    return *this;
  }
  SharedHeapPtr &operator=(SharedHeapPtr &&Other) noexcept {
    SharedHeapPtr(std::move(Other)).swap(*this);
    return *this;
  }
  ~SharedHeapPtr() noexcept { reset(); }

  T *get() const noexcept { return Ptr; }
  T &operator*() const noexcept { return *Ptr; }
  T *operator->() const noexcept { return Ptr; }
  explicit operator bool() const noexcept { return Ptr != nullptr; }
  uint32_t getOffset() const noexcept { return Off; }
  uint32_t getUseCount() const noexcept {
    return Ptr != nullptr
               ? getHeader()->RefCount.load(std::memory_order_relaxed)
               : 0;
  }

  void reset() noexcept {
    if (Ptr != nullptr) {
      if (getHeader()->RefCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        Heap->destroy(Ptr, Off);
      }
      Heap = nullptr;
      Ptr = nullptr;
      Off = 0;
    }
  }

  void swap(SharedHeapPtr &Other) noexcept {
    std::swap(Heap, Other.Heap);
    std::swap(Ptr, Other.Ptr);
    std::swap(Off, Other.Off);
  }

private:
  friend class RefHeap;
  SharedHeapPtr(RefHeap *H, T *P, uint32_t O) noexcept
      : Heap(H), Ptr(P), Off(O) {}

  HeapHeader *getHeader() const noexcept {
    return reinterpret_cast<HeapHeader *>(reinterpret_cast<uint8_t *>(Ptr) -
                                          sizeof(HeapHeader));
  }

  RefHeap *Heap = nullptr;
  T *Ptr = nullptr;
  uint32_t Off = 0;
};

template <typename T, typename... ArgsT>
Expect<UniqueHeapPtr<T>> RefHeap::makeUniqueWithTail(uint32_t TailBytes,
                                                     ArgsT &&...Args) noexcept {
  EXPECTED_TRY(auto Res, construct<T>(TailBytes, std::forward<ArgsT>(Args)...));
  return UniqueHeapPtr<T>(this, Res.first, Res.second);
}

template <typename T, typename... ArgsT>
Expect<SharedHeapPtr<T>> RefHeap::makeSharedWithTail(uint32_t TailBytes,
                                                     ArgsT &&...Args) noexcept {
  EXPECTED_TRY(auto Res, construct<T>(TailBytes, std::forward<ArgsT>(Args)...));
  return SharedHeapPtr<T>(this, Res.first, Res.second);
}

} // namespace Runtime
} // namespace WasmEdge
