// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "runtime/refheap.h"
#include "runtime/instance/function.h"

#include "common/config.h"
#include "common/types.h"
#include "system/allocator.h"

#include <algorithm>
#include <cstddef>
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

RefHeap::RefHeap(const Limits &L) noexcept
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
  std::unique_ptr<RefHeap> Heap(new (std::nothrow) RefHeap(L));
  if (!Heap || !Heap->Blocks) {
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
    return allocateData(Kind, Size, 0, 0);
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
        try {
          FreeRuns.emplace(Start - DataTop, DataTop);
        } catch (const std::bad_alloc &) {
        }
      }
      DataCommitted = End * kBlocksPerChunk;
    }
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

Expect<uint32_t> RefHeap::allocateDataSlot(HeapKind Kind, uint64_t FieldBytes,
                                           uint32_t TypeId,
                                           uint32_t Length) noexcept {
  if (FieldBytes > k4G) {
    return Unexpect(ErrCode::Value::RefHeapFull);
  }
  return allocateData(Kind, sizeof(HeapHeader) + FieldBytes, TypeId, Length);
}

Expect<uint32_t> RefHeap::allocateData(HeapKind Kind, uint64_t Size,
                                       uint32_t TypeId,
                                       uint32_t Length) noexcept try {
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
  if (Runtime::RefHeap::isObjectKind(Kind)) {
    return Heap->decodeObject(Kind, Off);
  }
  if ((Off & (Runtime::RefHeap::kTagI31 | Runtime::RefHeap::kTagObject)) != 0) {
    return nullptr;
  }
  auto *Header = Heap->getHeader(Off);
  if (Header == nullptr || Header->Kind != Kind) {
    return nullptr;
  }
  return reinterpret_cast<uint8_t *>(Header) + sizeof(Runtime::HeapHeader);
}

} // namespace

const Runtime::Instance::FunctionInstance *
RefVariant::getFuncInst() const noexcept {
  return static_cast<const Runtime::Instance::FunctionInstance *>(
      decodeRef(Runtime::HeapKind::Function, getOffset()));
}

Runtime::Instance::StructInstance *RefVariant::getStructInst() const noexcept {
  return static_cast<Runtime::Instance::StructInstance *>(
      decodeRef(Runtime::HeapKind::Struct, getOffset()));
}

Runtime::Instance::ArrayInstance *RefVariant::getArrayInst() const noexcept {
  return static_cast<Runtime::Instance::ArrayInstance *>(
      decodeRef(Runtime::HeapKind::Array, getOffset()));
}

const Runtime::Instance::ExceptionInstance *
RefVariant::getExceptionInst() const noexcept {
  return static_cast<const Runtime::Instance::ExceptionInstance *>(
      decodeRef(Runtime::HeapKind::Exception, getOffset()));
}

const Runtime::Instance::CompositeBase *
RefVariant::getCompositeInst() const noexcept {
  return getFuncInst();
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

Expect<uint8_t *> RefHeap::getDataSpan(uint32_t Off, uint32_t Pos,
                                       uint32_t Size) const noexcept {
  uint32_t BlockIdx, SlotIdx, SlotSize;
  if (!findDataSlot(Off, BlockIdx, SlotIdx, SlotSize) ||
      static_cast<uint64_t>(Pos) + Size > SlotSize) {
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  return getDataAddr(Off) + Pos;
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
  std::memset(getDataAddr(Off), 0, SlotSize);
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
