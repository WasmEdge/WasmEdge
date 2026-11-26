// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "runtime/refheap.h"
#include "runtime/instance/function.h"
#include "runtime/typetable.h"

#include "common/config.h"
#include "common/types.h"
#include "system/allocator.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <new>

namespace WasmEdge {
namespace Runtime {

namespace {

constexpr uint32_t kBlockShift = 16;
constexpr uint32_t kBlockMask = RefHeap::kBlockSize - 1;
constexpr uint32_t kMaxSmallSize = UINT32_C(1) << 15;
constexpr uint32_t kBitmapWords = RefHeap::kBlockSize / 32 / 64;
constexpr uint64_t k4G = UINT64_C(1) << 32;
constexpr uint32_t kChunkGuard = 65536;
constexpr uint32_t kBlocksPerChunk = RefHeap::kChunkSize / RefHeap::kBlockSize;

} // namespace

HeapMode RefHeap::getDefaultMode() noexcept {
#if defined(WASMEDGE_REFHEAP_FORCE_CHUNK_MODE)
  return HeapMode::Chunk;
#else
  return isModeSupported(HeapMode::Flat) ? HeapMode::Flat : HeapMode::Chunk;
#endif
}

bool RefHeap::isModeSupported(HeapMode M) noexcept {
  if (M == HeapMode::Chunk) {
#if WASMEDGE_OS_WINDOWS || defined(HAVE_MMAP)
    return true;
#else
    return false;
#endif
  }
#if WASMEDGE_ALLOCATOR_IS_STABLE
  return sizeof(void *) == 8;
#else
  return false;
#endif
}

RefHeap::RefHeap(const Limits &L)
    : Mode(L.Mode), DataBlocks(static_cast<uint32_t>(
                        std::min(L.DataBytes, k4G) / kBlockSize)),
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
  if (!isModeSupported(L.Mode)) {
    return Unexpect(ErrCode::Value::RefHeapFull);
  }
  std::unique_ptr<RefHeap> Heap;
  try {
    Heap.reset(new RefHeap(L));
  } catch (const std::bad_alloc &) {
    return Unexpect(ErrCode::Value::RefHeapFull);
  }
  if (!Heap->Blocks) {
    return Unexpect(ErrCode::Value::RefHeapFull);
  }
  if (L.Mode == HeapMode::Flat) {
#if WASMEDGE_ALLOCATOR_IS_STABLE
    Heap->DataBase = Allocator::allocate(0);
    Heap->ObjectBase = Allocator::allocate(0);
#endif
    if (Heap->DataBase == nullptr || Heap->ObjectBase == nullptr) {
      return Unexpect(ErrCode::Value::RefHeapFull);
    }
    Heap->initDescriptor();
    return Heap;
  }
  Heap->DataTable.reset(new (std::nothrow) std::atomic<uint8_t *>[kChunkCount]);
  Heap->ObjectTable.reset(new (std::nothrow)
                              std::atomic<uint8_t *>[kChunkCount]);
  Heap->NoAccess = Allocator::allocate_chunk(kChunkSize + kChunkGuard);
  if (!Heap->DataTable || !Heap->ObjectTable || Heap->NoAccess == nullptr ||
      !Allocator::set_chunk_no_access(Heap->NoAccess,
                                      kChunkSize + kChunkGuard)) {
    return Unexpect(ErrCode::Value::RefHeapFull);
  }
  for (uint32_t I = 0; I < kChunkCount; ++I) {
    std::atomic_init(&Heap->DataTable[I], Heap->NoAccess);
    std::atomic_init(&Heap->ObjectTable[I], Heap->NoAccess);
  }
  Heap->initDescriptor();
  return Heap;
}

void RefHeap::initDescriptor() noexcept {
  auto &P = Parts[getPartIndex(HeapKind::Function)];
  Desc.ObjectBase = Mode == HeapMode::Flat ? ObjectBase : nullptr;
  Desc.ObjectTable = Mode == HeapMode::Chunk ? ObjectTable.get() : nullptr;
  Desc.FuncTop = &P.Top;
  Desc.FuncPartBase = P.Base;
  Desc.FuncSlotMask = P.SlotSize - 1;
}

namespace {
std::atomic<RefHeap *> GlobalHeap = nullptr;
std::mutex GlobalHeapMutex;
} // namespace

RefHeap *RefHeap::getGlobal() noexcept {
  if (auto *Heap = GlobalHeap.load(std::memory_order_acquire)) {
    return Heap;
  }
  std::unique_lock Lock(GlobalHeapMutex);
  auto *Heap = GlobalHeap.load(std::memory_order_relaxed);
  if (Heap == nullptr) {
    auto Res = create(Limits{});
    if (!Res && getDefaultMode() == HeapMode::Flat &&
        isModeSupported(HeapMode::Chunk)) {
      Limits L;
      L.Mode = HeapMode::Chunk;
      Res = create(L);
    }
    Heap = Res ? Res->release() : nullptr;
    GlobalHeap.store(Heap, std::memory_order_release);
  }
  return Heap;
}

HeapMode RefHeap::getGlobalMode() noexcept {
  const auto *Heap = getGlobal();
  return Heap != nullptr ? Heap->getMode() : getDefaultMode();
}

bool RefHeap::attachGlobal(RefHeap *Heap) noexcept {
  if (Heap == nullptr || !isModeSupported(Heap->getMode())) {
    return false;
  }
  std::unique_lock Lock(GlobalHeapMutex);
  auto *Current = GlobalHeap.load(std::memory_order_relaxed);
  if (Current == nullptr) {
    GlobalHeap.store(Heap, std::memory_order_release);
    return true;
  }
  return Current == Heap;
}

RefHeap::~RefHeap() noexcept {
  if (DataBase != nullptr) {
    Allocator::release(DataBase, 0);
  }
  if (ObjectBase != nullptr) {
    Allocator::release(ObjectBase, 0);
  }
  for (const auto &[Host, E] : Extents) {
    Allocator::release_chunk(reinterpret_cast<uint8_t *>(Host),
                             E.Size + kChunkGuard);
  }
  if (NoAccess != nullptr) {
    Allocator::release_chunk(NoAccess, kChunkSize + kChunkGuard);
  }
}

bool RefHeap::mapChunks(bool Object, uint32_t RawOff, uint32_t Count) noexcept {
  const uint64_t Bytes = static_cast<uint64_t>(Count) * kChunkSize;
  if (Bytes + kChunkGuard >
      static_cast<uint64_t>(std::numeric_limits<std::ptrdiff_t>::max())) {
    return false;
  }
  uint8_t *Host = Allocator::allocate_chunk(Bytes + kChunkGuard);
  if (Host == nullptr) {
    return false;
  }
  if (!Allocator::set_chunk_no_access(Host + Bytes, kChunkGuard)) {
    Allocator::release_chunk(Host, Bytes + kChunkGuard);
    return false;
  }
  try {
    std::unique_lock Lock(ExtentMutex);
    Extents.emplace(reinterpret_cast<uintptr_t>(Host),
                    Extent{RawOff, Bytes, Object});
  } catch (const std::bad_alloc &) {
    Allocator::release_chunk(Host, Bytes + kChunkGuard);
    return false;
  }
  auto *Table = Object ? ObjectTable.get() : DataTable.get();
  for (uint32_t I = 0; I < Count; ++I) {
    Table[(RawOff >> kChunkShift) + I].store(Host + I * uint64_t(kChunkSize),
                                             std::memory_order_release);
  }
  return true;
}

uint32_t RefHeap::findRawOffset(uintptr_t Addr, bool &Object) const noexcept {
  std::shared_lock Lock(ExtentMutex);
  auto It = Extents.upper_bound(Addr);
  if (It == Extents.begin()) {
    return 0;
  }
  --It;
  const uint64_t Delta = Addr - It->first;
  if (Delta >= It->second.Size) {
    return 0;
  }
  Object = It->second.Object;
  return It->second.RawOff + static_cast<uint32_t>(Delta);
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
  auto *Header = reinterpret_cast<HeapHeader *>(getObjectAddr(Off));
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
    EXPECTED_TRY(const auto View, allocateData(Kind, Size, 0, 0));
    return View.getOffset();
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
  if (P.FreeHead != kNoFreeSlot) {
    Rel = P.FreeHead;
    P.FreeHead =
        reinterpret_cast<HeapHeader *>(getObjectAddr(P.Base + Rel))->TypeId;
  } else {
    Rel = P.Top.load(std::memory_order_relaxed);
    if (static_cast<uint64_t>(Rel) + P.SlotSize > PartLimit) {
      return Unexpect(ErrCode::Value::RefHeapFull);
    }
    if (Rel + P.SlotSize > P.Committed) {
      const uint32_t RawStart = P.Base & ~kTagObject;
      if (Mode == HeapMode::Flat) {
        if (Allocator::resize(ObjectBase + RawStart, P.Committed / kBlockSize,
                              P.Committed / kBlockSize + 1) == nullptr) {
          return Unexpect(ErrCode::Value::RefHeapFull);
        }
        P.Committed += kBlockSize;
      } else {
        if (!mapChunks(true, RawStart + P.Committed, 1)) {
          return Unexpect(ErrCode::Value::RefHeapFull);
        }
        P.Committed += kChunkSize;
      }
    }
    Fresh = true;
  }
  const uint32_t Off = P.Base + Rel;
  uint8_t *Slot = getObjectAddr(Off);
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
  if (auto It = FreeRuns.lower_bound({Count, 0}); It != FreeRuns.end()) {
    const auto [Have, Start] = *It;
    auto Node = FreeRuns.extract(It);
    auto StartNode = RunStarts.extract(Start);
    if (Have > Count) {
      Node.value() = {Have - Count, Start + Count};
      FreeRuns.insert(std::move(Node));
      StartNode.key() = Start + Count;
      StartNode.mapped() = Have - Count;
      RunStarts.insert(std::move(StartNode));
    }
    return Start;
  }
  uint32_t Start = DataTop;
  const bool Grow = static_cast<uint64_t>(Start) + Count > DataCommitted;
  if (Grow && Mode == HeapMode::Chunk && Start < DataCommitted) {
    Start = DataCommitted;
  }
  if (static_cast<uint64_t>(Start) + Count > DataBlocks) {
    return Unexpect(ErrCode::Value::RefHeapFull);
  }
  if (Grow) {
    if (Mode == HeapMode::Flat) {
      if (Allocator::resize(DataBase, DataCommitted, Start + Count) ==
          nullptr) {
        return Unexpect(ErrCode::Value::RefHeapFull);
      }
      DataCommitted = Start + Count;
    } else {
      const uint32_t First = DataCommitted / kBlocksPerChunk;
      const uint32_t End =
          (Start + Count + kBlocksPerChunk - 1) / kBlocksPerChunk;
      if (!mapChunks(false, First << kChunkShift, End - First)) {
        return Unexpect(ErrCode::Value::RefHeapFull);
      }
      if (DataTop < Start) {
        addRun(DataTop, Start - DataTop);
      }
      DataCommitted = End * kBlocksPerChunk;
    }
  }
  DataTop = Start + Count;
  return Start;
}

void RefHeap::reserveRun() {
  if (!SpareRun) {
    decltype(FreeRuns) Tmp;
    Tmp.emplace(0, 0);
    SpareRun = Tmp.extract(Tmp.begin());
  }
  if (!SpareStart) {
    decltype(RunStarts) Tmp;
    Tmp.emplace(0, 0);
    SpareStart = Tmp.extract(Tmp.begin());
  }
}

void RefHeap::reserveClass() {
  if (!SpareClass) {
    std::set<uint32_t> Tmp;
    Tmp.insert(0);
    SpareClass = Tmp.extract(Tmp.begin());
  }
}

void RefHeap::reserveRelease(uint32_t BlockIdx, uint32_t SlotSize) {
  if (SlotSize > kMaxSmallSize || Blocks[BlockIdx].Used == 1) {
    reserveRun();
  } else if (Blocks[BlockIdx].Used == kBlockSize / SlotSize) {
    reserveClass();
  }
}

void RefHeap::reserveData() {
  reserveRun();
  reserveClass();
  if (!SpareBitmap) {
    auto Bits = std::make_unique<std::atomic<uint64_t>[]>(kBitmapWords);
    for (uint32_t I = 0; I < kBitmapWords; ++I) {
      std::atomic_init(&Bits[I], UINT64_C(0));
    }
    SpareBitmap = std::move(Bits);
  }
  if (Bitmaps.size() == Bitmaps.capacity()) {
    Bitmaps.reserve(Bitmaps.size() * 2 + 1);
  }
}

void RefHeap::addRun(uint32_t Start, uint32_t Count) noexcept {
  auto Joins = [this](uint32_t First, uint32_t N, uint32_t Next) {
    return reinterpret_cast<uintptr_t>(getDataAddr(First << kBlockShift)) +
               (static_cast<uintptr_t>(N) << kBlockShift) ==
           reinterpret_cast<uintptr_t>(getDataAddr(Next << kBlockShift));
  };
  auto Next = RunStarts.lower_bound(Start);
  if (Next != RunStarts.end() && Next->first == Start + Count &&
      Joins(Start, Count, Next->first)) {
    FreeRuns.erase({Next->second, Next->first});
    Count += Next->second;
    Next = RunStarts.erase(Next);
  }
  if (Next != RunStarts.begin()) {
    if (auto Prev = std::prev(Next); Prev->first + Prev->second == Start &&
                                     Joins(Prev->first, Prev->second, Start)) {
      FreeRuns.erase({Prev->second, Prev->first});
      Start = Prev->first;
      Count += Prev->second;
      RunStarts.erase(Prev);
    }
  }
  assuming(SpareStart && SpareRun);
  SpareStart.key() = Start;
  SpareStart.mapped() = Count;
  RunStarts.insert(std::move(SpareStart));
  SpareRun.value() = {Count, Start};
  FreeRuns.insert(std::move(SpareRun));
}

void RefHeap::setupBlocks(uint32_t Start, uint32_t Count,
                          uint32_t SlotSize) noexcept {
  auto &First = Blocks[Start];
  if (First.LiveBits.load(std::memory_order_relaxed) == nullptr) {
    assuming(SpareBitmap && Bitmaps.size() < Bitmaps.capacity());
    Bitmaps.push_back(std::move(SpareBitmap));
    First.LiveBits.store(Bitmaps.back().get(), std::memory_order_release);
  }
  for (uint32_t B = Start; B < Start + Count; ++B) {
    Blocks[B].Info.store((static_cast<uint64_t>(Start) << 32) | SlotSize,
                         std::memory_order_release);
  }
}

Expect<RefHeap::DataView> RefHeap::allocateDataSlot(HeapKind Kind,
                                                    uint64_t FieldBytes,
                                                    uint32_t TypeId,
                                                    uint32_t Length) noexcept {
  if (FieldBytes > k4G) {
    return Unexpect(ErrCode::Value::RefHeapFull);
  }
  return allocateData(Kind, sizeof(HeapHeader) + FieldBytes, TypeId, Length);
}

Expect<RefHeap::DataView> RefHeap::allocateData(HeapKind Kind, uint64_t Size,
                                                uint32_t TypeId,
                                                uint32_t Length) noexcept try {
  if (Size > static_cast<uint64_t>(DataBlocks) * kBlockSize) {
    return Unexpect(ErrCode::Value::RefHeapFull);
  }
  std::unique_lock Lock(DataMutex);
  releasePending();
  reserveData();
  uint32_t Off;
  uint32_t SlotIdx = 0;
  uint32_t SlotSize;
  if (Size <= kMaxSmallSize) {
    const uint32_t Class = getClassIndex(Size);
    SlotSize = UINT32_C(1) << (Class + kMinClassShift);
    const uint32_t SlotCount = kBlockSize / SlotSize;
    auto &Partial = ClassBlocks[Class];
    if (Partial.empty()) {
      EXPECTED_TRY(const uint32_t Start, takeBlocks(1));
      setupBlocks(Start, 1, SlotSize);
      SpareClass.value() = Start;
      Partial.insert(std::move(SpareClass));
    }
    const uint32_t B = *Partial.begin();
    const auto *Bits = Blocks[B].LiveBits.load(std::memory_order_relaxed);
    uint32_t Word = 0;
    uint64_t Free = 0;
    for (; Word * 64 < SlotCount; ++Word) {
      Free = ~Bits[Word].load(std::memory_order_relaxed);
      if (SlotCount - Word * 64 < 64) {
        Free &= (UINT64_C(1) << (SlotCount - Word * 64)) - 1;
      }
      if (Free != 0) {
        break;
      }
    }
    assuming(Free != 0);
    SlotIdx = Word * 64;
    while ((Free & 1) == 0) {
      Free >>= 1;
      ++SlotIdx;
    }
    if (++Blocks[B].Used == SlotCount) {
      Partial.erase(Partial.begin());
    }
    Off = (B << kBlockShift) + SlotIdx * SlotSize;
  } else {
    const auto Count =
        static_cast<uint32_t>((Size + kBlockSize - 1) / kBlockSize);
    EXPECTED_TRY(const uint32_t Start, takeBlocks(Count));
    SlotSize = Count * kBlockSize;
    setupBlocks(Start, Count, SlotSize);
    Off = Start << kBlockShift;
  }
  uint8_t *Slot = getDataAddr(Off);
  new (Slot) HeapHeader();
  initHeader(Slot, Kind, Off, true);
  auto *Header = reinterpret_cast<HeapHeader *>(Slot);
  Header->TypeId = TypeId;
  Header->Length = Length;
  auto *Bits =
      Blocks[Off >> kBlockShift].LiveBits.load(std::memory_order_relaxed);
  Bits[SlotIdx / 64].fetch_or(UINT64_C(1) << (SlotIdx % 64),
                              std::memory_order_release);
  return DataView{Slot, SlotSize};
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
       1) == 0 ||
      reinterpret_cast<const HeapHeader *>(getDataAddr(Off))->Kind ==
          HeapKind::Free) {
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
  const auto *Header = reinterpret_cast<const HeapHeader *>(getObjectAddr(Off));
  if (Header->Live.load(std::memory_order_acquire) == 0) {
    return nullptr;
  }
  return P;
}

} // namespace Runtime

RefVariant RefVariant::makeExtern(const void *P) noexcept {
  const ValType VT(TypeCode::RefNull, TypeCode::ExternRef);
  auto *Heap = Runtime::RefHeap::getGlobal();
  if (P == nullptr || Heap == nullptr) {
    return make<void>(VT, nullptr);
  }
  auto Box = Heap->boxExtern(P);
  if (!Box) {
    return make<void>(VT, nullptr);
  }
  return make(VT, *Box);
}

const void *RefVariant::getExternHost() const noexcept {
  auto *Heap = Runtime::RefHeap::getGlobal();
  if (Heap == nullptr) {
    return nullptr;
  }
  const auto *Box = static_cast<const Runtime::ExternBox *>(
      Heap->decodeObject(Runtime::HeapKind::ExternBox, getOffset()));
  return Box != nullptr ? Box->Host : nullptr;
}

uint32_t RefVariant::encode(const void *P) noexcept {
  auto *Heap = Runtime::RefHeap::getGlobal();
  return Heap != nullptr ? Heap->getOffset(P) : 0U;
}

namespace {

void *decodeRef(Runtime::HeapKind Kind, uint32_t Off) noexcept {
  auto *Heap = Runtime::RefHeap::getGlobal();
  if (Heap == nullptr) {
    return nullptr;
  }
  return Heap->decodeObject(Kind, Off);
}

} // namespace

const Runtime::Instance::FunctionInstance *
RefVariant::getFuncInst() const noexcept {
  return static_cast<const Runtime::Instance::FunctionInstance *>(
      decodeRef(Runtime::HeapKind::Function, getOffset()));
}

const Runtime::Instance::ExceptionInstance *
RefVariant::getExceptionInst() const noexcept {
  return static_cast<const Runtime::Instance::ExceptionInstance *>(
      decodeRef(Runtime::HeapKind::Exception, getOffset()));
}

namespace Runtime {

Expect<const ExternBox *> RefHeap::boxExtern(const void *Host) noexcept {
  if (Host == nullptr) {
    return nullptr;
  }
  std::unique_lock Lock(BoxMutex);
  if (auto It = Boxes.find(Host); It != Boxes.end()) {
    return It->second;
  }
  EXPECTED_TRY(auto Res, construct<ExternBox>(0, Host));
  try {
    Boxes.emplace(Host, Res.first);
  } catch (const std::bad_alloc &) {
    destroy(Res.first, Res.second);
    return Unexpect(ErrCode::Value::RefHeapFull);
  }
  return Res.first;
}

const void *RefHeap::unboxExtern(const void *Obj) const noexcept {
  if (Obj == nullptr) {
    return nullptr;
  }
  const uint32_t Off = getOffset(Obj);
  if (decodeObject(HeapKind::ExternBox, Off) != Obj) {
    return nullptr;
  }
  return static_cast<const ExternBox *>(Obj)->Host;
}

HeapHeader *RefHeap::getHeader(uint32_t Off) const noexcept {
  if ((Off & kTagI31) != 0) {
    return nullptr;
  }
  if ((Off & kTagObject) != 0) {
    if (findPart(Off) == nullptr) {
      return nullptr;
    }
    return reinterpret_cast<HeapHeader *>(getObjectAddr(Off));
  }
  uint32_t BlockIdx, SlotIdx, SlotSize;
  if (!findDataSlot(Off, BlockIdx, SlotIdx, SlotSize)) {
    return nullptr;
  }
  return reinterpret_cast<HeapHeader *>(getDataAddr(Off));
}

void *RefHeap::decodeObject(HeapKind Kind, uint32_t Off) const noexcept {
  if (!isObjectKind(Kind)) {
    return nullptr;
  }
  const auto *P = findPart(Off);
  if (P != &Parts[getPartIndex(Kind)]) {
    return nullptr;
  }
  return getObjectAddr(Off) + sizeof(HeapHeader);
}

uint32_t RefHeap::getOffset(const void *Obj) const noexcept {
  const auto Addr = reinterpret_cast<uintptr_t>(Obj) - sizeof(HeapHeader);
  auto CheckObject = [this](uint32_t RawOff) noexcept -> uint32_t {
    const uint32_t Off = RawOff | kTagObject;
    if ((RawOff & (kTagI31 | kTagObject)) != 0 || findPart(Off) == nullptr) {
      return 0;
    }
    return Off;
  };
  auto CheckData = [this](uint32_t Off) noexcept -> uint32_t {
    uint32_t BlockIdx, SlotIdx, SlotSize;
    return findDataSlot(Off, BlockIdx, SlotIdx, SlotSize) ? Off : 0;
  };
  if (Mode == HeapMode::Chunk) {
    bool Object = false;
    const uint32_t RawOff = findRawOffset(Addr, Object);
    if (RawOff == 0) {
      return 0;
    }
    return Object ? CheckObject(RawOff) : CheckData(RawOff);
  }
  const auto ObjDelta =
      static_cast<uint64_t>(Addr - reinterpret_cast<uintptr_t>(ObjectBase));
  if (ObjDelta < k4G) {
    return CheckObject(static_cast<uint32_t>(ObjDelta));
  }
  const auto DataDelta =
      static_cast<uint64_t>(Addr - reinterpret_cast<uintptr_t>(DataBase));
  if (DataDelta < static_cast<uint64_t>(DataBlocks) * kBlockSize) {
    return CheckData(static_cast<uint32_t>(DataDelta));
  }
  return 0;
}

Expect<RefHeap::DataView> RefHeap::getDataView(uint32_t Off,
                                               HeapKind Kind) const noexcept {
  uint32_t BlockIdx, SlotIdx, SlotSize;
  if (!findDataSlot(Off, BlockIdx, SlotIdx, SlotSize)) {
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  uint8_t *Slot = getDataAddr(Off);
  if (reinterpret_cast<const HeapHeader *>(Slot)->Kind != Kind) {
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  return DataView{Slot, SlotSize};
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
  uint8_t *Slot = getObjectAddr(Off);
  auto *Header = reinterpret_cast<HeapHeader *>(Slot);
  if (Header->Kind == HeapKind::Free) {
    return;
  }
  Header->Live.store(0, std::memory_order_release);
  Header->Kind = HeapKind::Free;
  Header->TypeId = P.FreeHead;
  std::memset(Slot + sizeof(HeapHeader), 0, P.SlotSize - sizeof(HeapHeader));
  P.FreeHead = Off - P.Base;
}

void RefHeap::freeData(uint32_t Off) noexcept {
  std::unique_lock Lock(DataMutex);
  uint32_t BlockIdx, SlotIdx, SlotSize;
  if (!findDataSlot(Off, BlockIdx, SlotIdx, SlotSize)) {
    return;
  }
  uint8_t *Slot = getDataAddr(Off);
  try {
    reserveRelease(BlockIdx, SlotSize);
  } catch (const std::bad_alloc &) {
    auto *Header = reinterpret_cast<HeapHeader *>(Slot);
    Header->Live.store(0, std::memory_order_release);
    Header->Kind = HeapKind::Free;
    Header->TypeId = PendingFree;
    std::memset(Slot + sizeof(HeapHeader), 0, SlotSize - sizeof(HeapHeader));
    PendingFree = Off;
    return;
  }
  auto *Bits = Blocks[BlockIdx].LiveBits.load(std::memory_order_relaxed);
  Bits[SlotIdx / 64].fetch_and(~(UINT64_C(1) << (SlotIdx % 64)),
                               std::memory_order_acq_rel);
  std::memset(Slot, 0, SlotSize);
  releaseData(BlockIdx, SlotSize);
  releasePending();
}

void RefHeap::releaseData(uint32_t BlockIdx, uint32_t SlotSize) noexcept {
  if (SlotSize <= kMaxSmallSize) {
    auto &Blk = Blocks[BlockIdx];
    auto &Partial = ClassBlocks[getClassIndex(SlotSize)];
    if (--Blk.Used == 0) {
      Partial.erase(BlockIdx);
      Blk.Info.store(0, std::memory_order_release);
      addRun(BlockIdx, 1);
    } else if (Blk.Used + 1 == kBlockSize / SlotSize) {
      SpareClass.value() = BlockIdx;
      Partial.insert(std::move(SpareClass));
    }
    return;
  }
  const uint32_t Count = SlotSize / kBlockSize;
  for (uint32_t B = BlockIdx; B < BlockIdx + Count; ++B) {
    Blocks[B].Info.store(0, std::memory_order_release);
  }
  addRun(BlockIdx, Count);
}

void RefHeap::releasePending() noexcept {
  while (PendingFree != 0) {
    const uint32_t Off = PendingFree;
    const uint32_t BlockIdx = Off >> kBlockShift;
    const auto SlotSize = static_cast<uint32_t>(
        Blocks[BlockIdx].Info.load(std::memory_order_relaxed));
    try {
      reserveRelease(BlockIdx, SlotSize);
    } catch (const std::bad_alloc &) {
      return;
    }
    auto *Header = reinterpret_cast<HeapHeader *>(getDataAddr(Off));
    PendingFree = Header->TypeId;
    const uint32_t SlotIdx =
        SlotSize <= kMaxSmallSize ? (Off & kBlockMask) / SlotSize : 0;
    auto *Bits = Blocks[BlockIdx].LiveBits.load(std::memory_order_relaxed);
    Bits[SlotIdx / 64].fetch_and(~(UINT64_C(1) << (SlotIdx % 64)),
                                 std::memory_order_acq_rel);
    std::memset(Header, 0, sizeof(HeapHeader));
    releaseData(BlockIdx, SlotSize);
  }
}

} // namespace Runtime
} // namespace WasmEdge

bool wasmedge_refheap_attach(void *Heap, void *Types) noexcept {
  using namespace WasmEdge::Runtime;
  return RefHeap::attachGlobal(static_cast<RefHeap *>(Heap)) &&
         CanonicalTypeTable::attachGlobal(
             static_cast<CanonicalTypeTable *>(Types));
}
