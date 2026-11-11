// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "runtime/refheap.h"

#include "common/config.h"
#include "system/allocator.h"

#include <algorithm>
#include <new>

namespace WasmEdge {
namespace Runtime {

namespace {

constexpr uint32_t kBlockShift = 16;
constexpr uint32_t kBlockMask = RefHeap::kBlockSize - 1;
constexpr uint32_t kMaxSmallSize = UINT32_C(1) << 15;
constexpr uint32_t kBitmapWords = RefHeap::kBlockSize / 32 / 64;
constexpr uint64_t k4G = UINT64_C(1) << 32;

} // namespace

RefHeap::RefHeap(const Limits &L) noexcept
    : DataBlocks(
          static_cast<uint32_t>(std::min(L.DataBytes, k4G) / kBlockSize)),
      PartLimit(static_cast<uint32_t>(
          std::min<uint64_t>(L.PartBytes, kPartSize) & ~uint64_t(kBlockMask))),
      Blocks(new (std::nothrow) Block[DataBlocks]()), DataTop(1),
      DataCommitted(0) {
  for (uint32_t I = 0; I < kPartCount; ++I) {
    const auto Kind =
        static_cast<HeapKind>(static_cast<uint32_t>(HeapKind::Function) + I);
    Parts[I].Base = getPartBase(Kind);
    Parts[I].SlotSize = getSlotSize(Kind);
  }
}

Expect<std::unique_ptr<RefHeap>> RefHeap::create(const Limits &L) noexcept {
#if WASMEDGE_ALLOCATOR_IS_STABLE
  std::unique_ptr<RefHeap> Heap(new (std::nothrow) RefHeap(L));
  if (!Heap) {
    return Unexpect(ErrCode::Value::RefHeapFull);
  }
  Heap->DataBase = Allocator::allocate(0);
  Heap->ObjectBase = Allocator::allocate(0);
  if (Heap->DataBase == nullptr || Heap->ObjectBase == nullptr) {
    return Unexpect(ErrCode::Value::RefHeapFull);
  }
  return Heap;
#else
  static_cast<void>(L);
  return Unexpect(ErrCode::Value::RefHeapFull);
#endif
}

RefHeap *RefHeap::getGlobal() noexcept {
  static RefHeap *Heap = []() noexcept -> RefHeap * {
    auto Res = create(Limits{});
    return Res ? Res->release() : nullptr;
  }();
  return Heap;
}

RefHeap::~RefHeap() noexcept {
  if (DataBase != nullptr) {
    Allocator::release(DataBase, 0);
  }
  if (ObjectBase != nullptr) {
    Allocator::release(ObjectBase, 0);
  }
}

uint32_t RefHeap::getClassIndex(uint64_t Size) noexcept {
  uint32_t Shift = kMinClassShift;
  while ((UINT64_C(1) << Shift) < Size) {
    ++Shift;
  }
  return Shift - kMinClassShift;
}

void RefHeap::initHeader(uint8_t *Slot, HeapKind Kind, uint32_t Off,
                         bool Live) noexcept {
  auto *Header = reinterpret_cast<HeapHeader *>(Slot);
  Header->Kind = Kind;
  Header->Reserved0 = 0;
  Header->SelfOffset = Off;
  Header->TypeId = 0;
  Header->Length = 0;
  Header->RefCount.store(1, std::memory_order_relaxed);
  Header->AllNext = 0;
  Header->GrayNext = 0;
  Header->Reserved1 = 0;
  Header->Live.store(Live ? 1 : 0, std::memory_order_release);
}

void RefHeap::setLive(uint32_t Off, bool Live) noexcept {
  auto *Header =
      reinterpret_cast<HeapHeader *>(ObjectBase + (Off & ~kTagObject));
  Header->Live.store(Live ? 1 : 0, std::memory_order_release);
}

Expect<uint32_t> RefHeap::allocate(HeapKind Kind, uint64_t Size) noexcept {
  return allocateSlot(Kind, Size, true);
}

Expect<uint32_t> RefHeap::allocateSlot(HeapKind Kind, uint64_t Size,
                                       bool Live) noexcept {
  Size = std::max<uint64_t>(Size, sizeof(HeapHeader));
  if (isObjectKind(Kind)) {
    return allocateObject(Kind, Size, Live);
  }
  if (Kind == HeapKind::Struct || Kind == HeapKind::Array) {
    return allocateData(Kind, Size);
  }
  return Unexpect(ErrCode::Value::RefHeapFull);
}

Expect<uint32_t> RefHeap::allocateObject(HeapKind Kind, uint64_t Size,
                                         bool Live) noexcept {
  auto &P = Parts[getPartIndex(Kind)];
  if (Size > P.SlotSize) {
    return Unexpect(ErrCode::Value::RefHeapFull);
  }
  std::unique_lock Lock(P.Mutex);
  uint32_t Rel;
  bool Fresh = false;
  if (!P.FreeList.empty()) {
    Rel = P.FreeList.back();
    P.FreeList.pop_back();
  } else {
    Rel = P.Top.load(std::memory_order_relaxed);
    if (static_cast<uint64_t>(Rel) + P.SlotSize > PartLimit) {
      return Unexpect(ErrCode::Value::RefHeapFull);
    }
    if (Rel + P.SlotSize > P.Committed) {
      uint8_t *PartStart = ObjectBase + (P.Base & ~kTagObject);
      if (Allocator::resize(PartStart, P.Committed / kBlockSize,
                            P.Committed / kBlockSize + 1) == nullptr) {
        return Unexpect(ErrCode::Value::RefHeapFull);
      }
      P.Committed += kBlockSize;
    }
    Fresh = true;
  }
  const uint32_t Off = P.Base + Rel;
  uint8_t *Slot = ObjectBase + (Off & ~kTagObject);
  if (Fresh) {
    new (Slot) HeapHeader();
  }
  initHeader(Slot, Kind, Off, Live);
  if (Fresh) {
    P.Top.store(Rel + P.SlotSize, std::memory_order_release);
  }
  return Off;
}

Expect<uint32_t> RefHeap::takeBlocks(uint32_t Count) noexcept {
  if (auto It = FreeRuns.lower_bound(Count); It != FreeRuns.end()) {
    const uint32_t Have = It->first;
    const uint32_t Start = It->second;
    auto Node = FreeRuns.extract(It);
    if (Have > Count) {
      Node.key() = Have - Count;
      Node.mapped() = Start + Count;
      FreeRuns.insert(std::move(Node));
    }
    return Start;
  }
  if (static_cast<uint64_t>(DataTop) + Count > DataBlocks) {
    return Unexpect(ErrCode::Value::RefHeapFull);
  }
  const uint32_t Start = DataTop;
  if (Start + Count > DataCommitted) {
    if (Allocator::resize(DataBase, DataCommitted, Start + Count) == nullptr) {
      return Unexpect(ErrCode::Value::RefHeapFull);
    }
    DataCommitted = Start + Count;
  }
  DataTop = Start + Count;
  return Start;
}

void RefHeap::setupBlocks(uint32_t Start, uint32_t Count, uint32_t SlotSize) {
  for (uint32_t B = Start; B < Start + Count; ++B) {
    auto &Blk = Blocks[B];
    if (Blk.LiveBits.load(std::memory_order_relaxed) == nullptr) {
      auto Bits = std::make_unique<std::atomic<uint64_t>[]>(kBitmapWords);
      for (uint32_t I = 0; I < kBitmapWords; ++I) {
        std::atomic_init(&Bits[I], UINT64_C(0));
      }
      Bitmaps.push_back(std::move(Bits));
      Blk.LiveBits.store(Bitmaps.back().get(), std::memory_order_release);
    }
    Blk.Info.store((static_cast<uint64_t>(Start) << 32) | SlotSize,
                   std::memory_order_release);
  }
}

Expect<uint32_t> RefHeap::allocateData(HeapKind Kind,
                                       uint64_t Size) noexcept try {
  if (Size > static_cast<uint64_t>(DataBlocks) * kBlockSize) {
    return Unexpect(ErrCode::Value::RefHeapFull);
  }
  std::unique_lock Lock(DataMutex);
  uint32_t Off;
  uint32_t SlotIdx = 0;
  if (Size <= kMaxSmallSize) {
    const uint32_t Class = getClassIndex(Size);
    const uint32_t SlotSize = UINT32_C(1) << (Class + kMinClassShift);
    auto &FreeList = ClassFree[Class];
    if (FreeList.empty()) {
      EXPECTED_TRY(const uint32_t Start, takeBlocks(1));
      setupBlocks(Start, 1, SlotSize);
      const uint32_t BlockOff = Start << kBlockShift;
      for (uint32_t Pos = kBlockSize; Pos > 0; Pos -= SlotSize) {
        FreeList.push_back(BlockOff + Pos - SlotSize);
      }
    }
    Off = FreeList.back();
    FreeList.pop_back();
    SlotIdx = (Off & kBlockMask) / SlotSize;
  } else {
    const auto Count =
        static_cast<uint32_t>((Size + kBlockSize - 1) / kBlockSize);
    EXPECTED_TRY(const uint32_t Start, takeBlocks(Count));
    setupBlocks(Start, Count, Count * kBlockSize);
    Off = Start << kBlockShift;
  }
  new (DataBase + Off) HeapHeader();
  initHeader(DataBase + Off, Kind, Off, true);
  auto *Bits =
      Blocks[Off >> kBlockShift].LiveBits.load(std::memory_order_relaxed);
  Bits[SlotIdx / 64].fetch_or(UINT64_C(1) << (SlotIdx % 64),
                              std::memory_order_release);
  return Off;
} catch (const std::bad_alloc &) {
  return Unexpect(ErrCode::Value::RefHeapFull);
}

bool RefHeap::findDataSlot(uint32_t Off, uint32_t &BlockIdx, uint32_t &SlotIdx,
                           uint32_t &SlotSize) const noexcept {
  if (Off == 0 || (Off & 15) != 0) {
    return false;
  }
  const uint32_t B = Off >> kBlockShift;
  if (B >= DataBlocks) {
    return false;
  }
  const uint64_t Info = Blocks[B].Info.load(std::memory_order_acquire);
  const auto S = static_cast<uint32_t>(Info);
  if (S == 0) {
    return false;
  }
  if (S <= kMaxSmallSize) {
    if ((Off & kBlockMask) % S != 0) {
      return false;
    }
    SlotIdx = (Off & kBlockMask) / S;
  } else {
    if ((Off & kBlockMask) != 0 || (Info >> 32) != B) {
      return false;
    }
    SlotIdx = 0;
  }
  const auto *Bits = Blocks[B].LiveBits.load(std::memory_order_acquire);
  if (Bits == nullptr ||
      ((Bits[SlotIdx / 64].load(std::memory_order_acquire) >> (SlotIdx % 64)) &
       1) == 0) {
    return false;
  }
  BlockIdx = B;
  SlotSize = S;
  return true;
}

const RefHeap::Part *RefHeap::findSlot(uint32_t Off) const noexcept {
  if ((Off & (kTagI31 | kTagObject)) != kTagObject) {
    return nullptr;
  }
  const uint32_t Idx = Off >> 30;
  if (Idx == 0) {
    return nullptr;
  }
  const auto &P = Parts[Idx - 1];
  const uint32_t Rel = Off - P.Base;
  if (Rel >= P.Top.load(std::memory_order_acquire) ||
      (Rel & (P.SlotSize - 1)) != 0) {
    return nullptr;
  }
  return &P;
}

const RefHeap::Part *RefHeap::findPart(uint32_t Off) const noexcept {
  const auto *P = findSlot(Off);
  if (P == nullptr) {
    return nullptr;
  }
  const auto *Header =
      reinterpret_cast<const HeapHeader *>(ObjectBase + (Off & ~kTagObject));
  if (Header->Live.load(std::memory_order_acquire) == 0) {
    return nullptr;
  }
  return P;
}

HeapHeader *RefHeap::getHeader(uint32_t Off) const noexcept {
  if ((Off & kTagI31) != 0) {
    return nullptr;
  }
  if ((Off & kTagObject) != 0) {
    if (findPart(Off) == nullptr) {
      return nullptr;
    }
    return reinterpret_cast<HeapHeader *>(ObjectBase + (Off & ~kTagObject));
  }
  uint32_t BlockIdx, SlotIdx, SlotSize;
  if (!findDataSlot(Off, BlockIdx, SlotIdx, SlotSize)) {
    return nullptr;
  }
  return reinterpret_cast<HeapHeader *>(DataBase + Off);
}

void *RefHeap::decodeObject(HeapKind Kind, uint32_t Off) const noexcept {
  if (!isObjectKind(Kind)) {
    return nullptr;
  }
  const auto *P = findPart(Off);
  if (P != &Parts[getPartIndex(Kind)]) {
    return nullptr;
  }
  return ObjectBase + (Off & ~kTagObject) + sizeof(HeapHeader);
}

uint32_t RefHeap::getOffset(const void *Obj) const noexcept {
  const auto Addr = reinterpret_cast<uintptr_t>(Obj) - sizeof(HeapHeader);
  const auto ObjDelta =
      static_cast<uint64_t>(Addr - reinterpret_cast<uintptr_t>(ObjectBase));
  if (ObjDelta < k4G) {
    const auto Off = static_cast<uint32_t>(ObjDelta) | kTagObject;
    if ((ObjDelta & (kTagI31 | kTagObject)) != 0 || findPart(Off) == nullptr) {
      return 0;
    }
    return Off;
  }
  const auto DataDelta =
      static_cast<uint64_t>(Addr - reinterpret_cast<uintptr_t>(DataBase));
  if (DataDelta < static_cast<uint64_t>(DataBlocks) * kBlockSize) {
    const auto Off = static_cast<uint32_t>(DataDelta);
    uint32_t BlockIdx, SlotIdx, SlotSize;
    if (findDataSlot(Off, BlockIdx, SlotIdx, SlotSize)) {
      return Off;
    }
  }
  return 0;
}

Expect<uint8_t *> RefHeap::getDataSpan(uint32_t Off, uint32_t Pos,
                                       uint32_t Size) const noexcept {
  uint32_t BlockIdx, SlotIdx, SlotSize;
  if (!findDataSlot(Off, BlockIdx, SlotIdx, SlotSize) ||
      static_cast<uint64_t>(Pos) + Size > SlotSize) {
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  return DataBase + Off + Pos;
}

void RefHeap::free(uint32_t Off) noexcept {
  if ((Off & (kTagI31 | kTagObject)) == kTagObject) {
    freeObject(Off);
  } else if (Off != 0 && (Off & kTagI31) == 0) {
    freeData(Off);
  }
}

void RefHeap::freeObject(uint32_t Off) noexcept {
  const auto *Found = findSlot(Off);
  if (Found == nullptr) {
    return;
  }
  auto &P = Parts[Found - Parts];
  std::unique_lock Lock(P.Mutex);
  uint8_t *Slot = ObjectBase + (Off & ~kTagObject);
  auto *Header = reinterpret_cast<HeapHeader *>(Slot);
  if (Header->Kind == HeapKind::Free) {
    return;
  }
  Header->Live.store(0, std::memory_order_release);
  Header->Kind = HeapKind::Free;
  std::memset(Slot + sizeof(HeapHeader), 0, P.SlotSize - sizeof(HeapHeader));
  try {
    P.FreeList.push_back(Off - P.Base);
  } catch (const std::bad_alloc &) {
  }
}

void RefHeap::freeData(uint32_t Off) noexcept {
  std::unique_lock Lock(DataMutex);
  uint32_t BlockIdx, SlotIdx, SlotSize;
  if (!findDataSlot(Off, BlockIdx, SlotIdx, SlotSize)) {
    return;
  }
  auto *Bits = Blocks[BlockIdx].LiveBits.load(std::memory_order_relaxed);
  Bits[SlotIdx / 64].fetch_and(~(UINT64_C(1) << (SlotIdx % 64)),
                               std::memory_order_acq_rel);
  std::memset(DataBase + Off, 0, SlotSize);
  try {
    if (SlotSize <= kMaxSmallSize) {
      ClassFree[getClassIndex(SlotSize)].push_back(Off);
      return;
    }
    const uint32_t Count = SlotSize / kBlockSize;
    for (uint32_t B = BlockIdx; B < BlockIdx + Count; ++B) {
      Blocks[B].Info.store(0, std::memory_order_release);
    }
    FreeRuns.emplace(Count, BlockIdx);
  } catch (const std::bad_alloc &) {
  }
}

} // namespace Runtime
} // namespace WasmEdge
