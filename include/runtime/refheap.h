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
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <set>
#include <shared_mutex>
#include <type_traits>
#include <unordered_map>
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

/// Address mode of a heap.
enum class HeapMode : uint8_t {
  /// One reservation of 4 GiB and a guard for each region.
  Flat,
  /// A table of 1 MiB chunks for each region.
  Chunk,
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

/// Heap fields that compiled code reads to decode a function offset.
struct RefHeapDescriptor {
  /// Base of the object region in flat mode, or null.
  uint8_t *ObjectBase;
  /// Chunk table of the object region in chunk mode, or null.
  std::atomic<uint8_t *> *ObjectTable;
  /// Committed bytes of the function part.
  const std::atomic<uint32_t> *FuncTop;
  /// Tagged offset of the first function slot.
  uint32_t FuncPartBase;
  /// Function slot size minus one.
  uint32_t FuncSlotMask;
};
static_assert(offsetof(RefHeapDescriptor, ObjectBase) == 0);
static_assert(offsetof(RefHeapDescriptor, ObjectBase) <
              offsetof(RefHeapDescriptor, ObjectTable));
static_assert(offsetof(RefHeapDescriptor, ObjectTable) <
              offsetof(RefHeapDescriptor, FuncTop));
static_assert(offsetof(RefHeapDescriptor, FuncTop) <
              offsetof(RefHeapDescriptor, FuncPartBase));
static_assert(offsetof(RefHeapDescriptor, FuncPartBase) <
              offsetof(RefHeapDescriptor, FuncSlotMask));

/// Map a C++ type to the kind of its heap slot.
template <typename T> struct HeapTraits;

template <typename T> class UniqueHeapPtr;
template <typename T> class SharedHeapPtr;

/// Box of a host pointer that an externref holds.
struct ExternBox {
  explicit ExternBox(const void *H) noexcept : Host(H) {}
  const void *Host;
};
template <> struct HeapTraits<ExternBox> {
  static constexpr HeapKind Kind = HeapKind::ExternBox;
};

/// Heap with a data region and an object region, addressed by 32-bit offsets.
class RefHeap {
public:
  static constexpr uint32_t kTagI31 = 1;
  static constexpr uint32_t kTagObject = 2;
  static constexpr uint32_t kBlockSize = 65536;
  static constexpr uint32_t kPartSize = UINT32_C(1) << 30;
  static constexpr uint32_t kChunkShift = 20;
  static constexpr uint32_t kChunkSize = UINT32_C(1) << kChunkShift;
  static constexpr uint32_t kChunkCount = UINT32_C(1) << (32 - kChunkShift);

  /// Address mode of the heap that `getGlobal` creates.
  static HeapMode getDefaultMode() noexcept;

  /// Whether this host can create a heap in `Mode`. Chunk mode needs memory
  /// that the allocator gives as zero pages and that it can make
  /// inaccessible, which the allocator does only with mmap or on Windows.
  static bool isModeSupported(HeapMode Mode) noexcept;

  struct Limits {
    uint64_t DataBytes = UINT64_C(1) << 32;
    uint64_t PartBytes = UINT64_C(1) << 30;
    HeapMode Mode = getDefaultMode();
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

  /// Create a heap. Fails when the host does not support the mode or the
  /// reservation fails.
  static Expect<std::unique_ptr<RefHeap>> create(const Limits &L) noexcept;

  /// The heap shared by the whole process, or null if it cannot be created.
  /// It is never destroyed.
  WASMEDGE_EXPORT static RefHeap *getGlobal() noexcept;

  /// Use `Heap` as the global heap of this runtime copy. Fails if this copy
  /// already uses a different global heap.
  WASMEDGE_EXPORT static bool attachGlobal(RefHeap *Heap) noexcept;

  ~RefHeap() noexcept;
  RefHeap(const RefHeap &) = delete;
  RefHeap &operator=(const RefHeap &) = delete;

  HeapMode getMode() const noexcept { return Mode; }

  /// Descriptor that compiled code reads to decode a function offset.
  const RefHeapDescriptor &getDescriptor() const noexcept { return Desc; }

  /// Allocate a slot of `Size` bytes with the header. Returns its offset.
  Expect<uint32_t> allocate(HeapKind Kind, uint64_t Size) noexcept;

  /// Free the slot at `Off`. The caller already destroyed the object.
  WASMEDGE_EXPORT void free(uint32_t Off) noexcept;

  /// Header of the live slot at `Off`, or null.
  HeapHeader *getHeader(uint32_t Off) const noexcept;

  /// Object of the live slot of `Kind` at `Off`, or null.
  void *decodeObject(HeapKind Kind, uint32_t Off) const noexcept;

  /// Offset of the live slot whose object starts at `Obj`, or 0.
  WASMEDGE_EXPORT uint32_t getOffset(const void *Obj) const noexcept;

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
  /// New data slot for the empty marker type `T`, with `FieldBytes` zero
  /// bytes after the header. The header holds `TypeId` and `Length`.
  template <typename T>
  Expect<UniqueHeapPtr<T>> makeData(uint32_t TypeId, uint32_t Length,
                                    uint64_t FieldBytes) noexcept;

  /// Box of `Host`, or null for a null `Host`. The same host pointer always
  /// gives the same box. Boxes live until the heap is destroyed.
  WASMEDGE_EXPORT Expect<const ExternBox *>
  boxExtern(const void *Host) noexcept;

  /// Host pointer in the box whose object starts at `Obj`, or null if `Obj`
  /// is not a live box of this heap.
  WASMEDGE_EXPORT const void *unboxExtern(const void *Obj) const noexcept;

  /// Owning handle of an object that `UniqueHeapPtr::release` gave out, or
  /// an empty handle if `Ptr` is not a live object of `T` in this heap.
  template <typename T> UniqueHeapPtr<T> adopt(T *Ptr) noexcept;

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
    static_assert(noexcept(T(std::declval<ArgsT>()...)));
    if constexpr (isObjectKind(Kind)) {
      static_assert(sizeof(HeapHeader) + sizeof(T) <= getSlotSize(Kind));
    }
    EXPECTED_TRY(const uint32_t Off,
                 allocateSlot(Kind,
                              sizeof(HeapHeader) + sizeof(T) +
                                  static_cast<uint64_t>(TailBytes),
                              false));
    uint8_t *Slot = isObjectKind(Kind) ? getObjectAddr(Off) : getDataAddr(Off);
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
    if constexpr (!std::is_trivially_destructible_v<T>) {
      Ptr->~T();
    }
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
    /// Live slots of a small slot block.
    uint32_t Used = 0;
  };

  struct Part {
    uint32_t Base = 0;
    uint32_t SlotSize = 0;
    std::atomic<uint32_t> Top = 0;
    uint32_t Committed = 0;
    std::vector<uint32_t> FreeList;
    std::mutex Mutex;
  };

  struct Extent {
    uint32_t RawOff = 0;
    uint64_t Size = 0;
    bool Object = false;
  };

  RefHeap(const Limits &L) noexcept;
  void initDescriptor() noexcept;

  static uint8_t *loadChunk(const std::atomic<uint8_t *> *Table,
                            uint32_t RawOff) noexcept {
    return Table[RawOff >> kChunkShift].load(std::memory_order_acquire) +
           (RawOff & (kChunkSize - 1));
  }
  uint8_t *getDataAddr(uint32_t Off) const noexcept {
    if (Mode == HeapMode::Flat) {
      return DataBase + Off;
    }
    return loadChunk(DataTable.get(), Off);
  }
  uint8_t *getObjectAddr(uint32_t Off) const noexcept {
    if (Mode == HeapMode::Flat) {
      return ObjectBase + (Off & ~kTagObject);
    }
    return loadChunk(ObjectTable.get(), Off & ~kTagObject);
  }
  bool mapChunks(bool Object, uint32_t RawOff, uint32_t Count) noexcept;
  uint32_t findRawOffset(uintptr_t Addr, bool &Object) const noexcept;

  static uint32_t getPartIndex(HeapKind Kind) noexcept {
    return static_cast<uint32_t>(Kind) -
           static_cast<uint32_t>(HeapKind::Function);
  }
  static uint32_t getClassIndex(uint64_t Size) noexcept;

  WASMEDGE_EXPORT Expect<uint32_t> allocateSlot(HeapKind Kind, uint64_t Size,
                                                bool Live) noexcept;
  /// New live data slot with `FieldBytes` zero bytes after the header. The
  /// header holds `TypeId` and `Length` before the slot becomes live.
  WASMEDGE_EXPORT Expect<uint32_t> allocateDataSlot(HeapKind Kind,
                                                    uint64_t FieldBytes,
                                                    uint32_t TypeId,
                                                    uint32_t Length) noexcept;
  Expect<uint32_t> allocateObject(HeapKind Kind, uint64_t Size,
                                  bool Live) noexcept;
  Expect<uint32_t> allocateData(HeapKind Kind, uint64_t Size, uint32_t TypeId,
                                uint32_t Length) noexcept;
  /// Prepare the spare index entries and the spare bitmap. Call it with
  /// `DataMutex` held, before a data allocation changes any state, or before
  /// a free changes the indexes.
  void reserveData();
  Expect<uint32_t> takeBlocks(uint32_t Count) noexcept;
  /// Add the free blocks to the free runs and merge the run with neighbor
  /// runs that are contiguous in host memory. Uses the spare index entries.
  void addRun(uint32_t Start, uint32_t Count) noexcept;
  /// Mark the blocks as one slot of `SlotSize` bytes, or as a block of small
  /// slots. The first block takes the spare bitmap if it has no bitmap.
  void setupBlocks(uint32_t Start, uint32_t Count, uint32_t SlotSize) noexcept;
  void freeObject(uint32_t Off) noexcept;
  /// Make the data slot at `Off` dead, then give its space back. If the
  /// index entries cannot be prepared, the space stays out of the indexes.
  void freeData(uint32_t Off) noexcept;
  void initHeader(uint8_t *Slot, HeapKind Kind, uint32_t Off,
                  bool Live) noexcept;
  WASMEDGE_EXPORT void setLive(uint32_t Off, bool Live) noexcept;

  /// Part of the object slot start at `Off`, live or not, or null.
  const Part *findSlot(uint32_t Off) const noexcept;
  /// Part of the live object slot at `Off`, or null.
  const Part *findPart(uint32_t Off) const noexcept;
  /// Index of the live bit of the data slot at `Off`, or false.
  bool findDataSlot(uint32_t Off, uint32_t &BlockIdx, uint32_t &SlotIdx,
                    uint32_t &SlotSize) const noexcept;

  HeapMode Mode;
  uint8_t *DataBase = nullptr;
  uint8_t *ObjectBase = nullptr;
  uint8_t *NoAccess = nullptr;
  std::unique_ptr<std::atomic<uint8_t *>[]> DataTable;
  std::unique_ptr<std::atomic<uint8_t *>[]> ObjectTable;
  mutable std::shared_mutex ExtentMutex;
  std::map<uintptr_t, Extent> Extents;
  uint32_t DataBlocks = 0;
  uint32_t PartLimit = 0;
  std::unique_ptr<Block[]> Blocks;
  std::vector<std::unique_ptr<std::atomic<uint64_t>[]>> Bitmaps;
  std::mutex DataMutex;
  uint32_t DataTop = 0;
  uint32_t DataCommitted = 0;
  /// Free block runs as (block count, first block) pairs.
  std::set<std::pair<uint32_t, uint32_t>> FreeRuns;
  /// Free block runs by first block.
  std::map<uint32_t, uint32_t> RunStarts;
  /// Small slot blocks with a free slot, for each size class.
  std::set<uint32_t> ClassBlocks[kClassCount];
  /// Entries and a bitmap that `reserveData` prepares, so that the indexes
  /// can change without an allocation.
  decltype(FreeRuns)::node_type SpareRun;
  decltype(RunStarts)::node_type SpareStart;
  std::set<uint32_t>::node_type SpareClass;
  std::unique_ptr<std::atomic<uint64_t>[]> SpareBitmap;
  Part Parts[kPartCount];
  std::mutex BoxMutex;
  std::unordered_map<const void *, const ExternBox *> Boxes;
  RefHeapDescriptor Desc = {};
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

  /// Give up ownership without destroying the object. `RefHeap::adopt`
  /// takes it back.
  T *release() noexcept {
    Heap = nullptr;
    Off = 0;
    return std::exchange(Ptr, nullptr);
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

template <typename T>
Expect<UniqueHeapPtr<T>> RefHeap::makeData(uint32_t TypeId, uint32_t Length,
                                           uint64_t FieldBytes) noexcept {
  constexpr HeapKind Kind = HeapTraits<T>::Kind;
  static_assert(Kind == HeapKind::Struct || Kind == HeapKind::Array);
  static_assert(std::is_empty_v<T> && std::is_trivially_destructible_v<T>);
  EXPECTED_TRY(const uint32_t Off,
               allocateDataSlot(Kind, FieldBytes, TypeId, Length));
  return UniqueHeapPtr<T>(
      this, reinterpret_cast<T *>(getDataAddr(Off) + sizeof(HeapHeader)), Off);
}

template <typename T> UniqueHeapPtr<T> RefHeap::adopt(T *Ptr) noexcept {
  if (Ptr == nullptr) {
    return {};
  }
  const uint32_t Off = getOffset(Ptr);
  if (Off == 0) {
    return {};
  }
  const auto *Header = getHeader(Off);
  if (Header == nullptr || Header->Kind != HeapTraits<T>::Kind) {
    return {};
  }
  return UniqueHeapPtr<T>(this, Ptr, Off);
}

template <typename T, typename... ArgsT>
Expect<SharedHeapPtr<T>> RefHeap::makeSharedWithTail(uint32_t TailBytes,
                                                     ArgsT &&...Args) noexcept {
  EXPECTED_TRY(auto Res, construct<T>(TailBytes, std::forward<ArgsT>(Args)...));
  return SharedHeapPtr<T>(this, Res.first, Res.second);
}

} // namespace Runtime
} // namespace WasmEdge

/// Give the runtime copy that defines this function the host heap and the
/// host canonical type table. The plugin loader calls it after it checks the
/// API version of the plugin descriptor.
///
/// Every runtime copy exports this function, the shared library included, so
/// the loader finds it in a plugin that links its own copy of the runtime,
/// statically or as another shared library, and the copy then uses the host
/// heap. A plugin that links the shared library of the host finds the host
/// function, and the call does nothing. A plugin without this function has no
/// runtime copy of its own.
extern "C" WASMEDGE_EXPORT bool wasmedge_refheap_attach(void *Heap,
                                                        void *Types) noexcept;
