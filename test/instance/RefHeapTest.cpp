// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "runtime/refheap.h"
#include "runtime/typetable.h"

#include "common/config.h"
#include "system/allocator.h"

#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>
#include <set>
#include <thread>
#include <vector>

namespace {

struct Counted {
  explicit Counted(int &C, uint64_t V) noexcept : Count(C), Value(V) {}
  ~Counted() noexcept { ++Count; }
  int &Count;
  uint64_t Value;
};

struct Tailed {
  uint64_t Head = 7;
};

struct Probe {
  Probe(WasmEdge::Runtime::RefHeap &H, uint32_t &Built,
        uint32_t &Destroyed) noexcept
      : Heap(H), SeenAtDestroy(Destroyed) {
    Built = Heap.getOffset(this);
  }
  ~Probe() noexcept { SeenAtDestroy = Heap.getOffset(this); }
  WasmEdge::Runtime::RefHeap &Heap;
  uint32_t &SeenAtDestroy;
};

class Hidden {
public:
  uint64_t getValue() const noexcept { return Value; }

private:
  friend class WasmEdge::Runtime::RefHeap;
  explicit Hidden(uint64_t V) noexcept : Value(V) {}
  uint64_t Value;
};

class TestData {
public:
  TestData() = delete;
};

} // namespace

namespace {
/// Allocations that the replaced `operator new` allows on this thread before
/// it throws, or a negative number for no limit.
thread_local int AllowedNews = -1;
thread_local uint64_t NewCount = 0;
} // namespace

void *operator new(std::size_t Size) {
  ++NewCount;
  if (AllowedNews == 0) {
    throw std::bad_alloc();
  }
  if (AllowedNews > 0) {
    --AllowedNews;
  }
  if (void *P = std::malloc(Size == 0 ? 1 : Size)) {
    return P;
  }
  throw std::bad_alloc();
}
void operator delete(void *P) noexcept { std::free(P); }
void operator delete(void *P, std::size_t) noexcept { std::free(P); }

namespace WasmEdge::Runtime {
template <> struct HeapTraits<TestData> {
  static constexpr HeapKind Kind = HeapKind::Struct;
};
template <> struct HeapTraits<Hidden> {
  static constexpr HeapKind Kind = HeapKind::Exception;
};
template <> struct HeapTraits<Counted> {
  static constexpr HeapKind Kind = HeapKind::Function;
};
template <> struct HeapTraits<Probe> {
  static constexpr HeapKind Kind = HeapKind::Function;
};
template <> struct HeapTraits<Tailed> {
  static constexpr HeapKind Kind = HeapKind::Struct;
};
} // namespace WasmEdge::Runtime

namespace {

using namespace WasmEdge;
using Runtime::HeapHeader;
using Runtime::HeapKind;
using Runtime::RefHeap;

using Runtime::HeapMode;

std::unique_ptr<RefHeap> makeHeap(HeapMode Mode, RefHeap::Limits L = {}) {
  L.Mode = Mode;
  auto Res = RefHeap::create(L);
  if (!Res) {
    return nullptr;
  }
  return std::move(*Res);
}

class RefHeapModes : public ::testing::TestWithParam<HeapMode> {};

#define MAKE_HEAP_OR_SKIP(Name, ...)                                           \
  if (!RefHeap::isModeSupported(GetParam())) {                                 \
    GTEST_SKIP() << "this address mode is not available on this host";         \
  }                                                                            \
  auto Name = makeHeap(GetParam(), ##__VA_ARGS__);                             \
  ASSERT_TRUE(Name) << "heap creation failed";

#define MAKE_CHUNK_HEAP(Name, ...)                                             \
  auto Name = makeHeap(HeapMode::Chunk, ##__VA_ARGS__);                        \
  ASSERT_TRUE(Name) << "heap creation failed";

TEST(RefHeap, HeaderLayout) {
  EXPECT_EQ(sizeof(HeapHeader), 32U);
  EXPECT_EQ(alignof(HeapHeader), 16U);
  EXPECT_EQ(offsetof(HeapHeader, Kind), 0U);
  EXPECT_EQ(offsetof(HeapHeader, Live), 1U);
  EXPECT_EQ(offsetof(HeapHeader, Reserved0), 2U);
  EXPECT_EQ(offsetof(HeapHeader, SelfOffset), 4U);
  EXPECT_EQ(offsetof(HeapHeader, TypeId), 8U);
  EXPECT_EQ(offsetof(HeapHeader, Length), 12U);
  EXPECT_EQ(offsetof(HeapHeader, RefCount), 16U);
  EXPECT_EQ(offsetof(HeapHeader, AllNext), 20U);
  EXPECT_EQ(offsetof(HeapHeader, GrayNext), 24U);
  EXPECT_EQ(offsetof(HeapHeader, Reserved1), 28U);
}

TEST_P(RefHeapModes, DataSlots) {
  MAKE_HEAP_OR_SKIP(Heap);
  auto A = Heap->allocate(HeapKind::Struct, 40);
  auto B = Heap->allocate(HeapKind::Struct, 100);
  ASSERT_TRUE(A);
  ASSERT_TRUE(B);
  EXPECT_NE(*A, 0U);
  EXPECT_NE(*B, 0U);
  EXPECT_NE(*A, *B);
  EXPECT_EQ(*A % 16, 0U);
  EXPECT_EQ(*B % 16, 0U);

  auto Span = Heap->getDataSpan(*A, 0, 40);
  ASSERT_TRUE(Span);
  auto *Header = Heap->getHeader(*A);
  ASSERT_NE(Header, nullptr);
  EXPECT_EQ(reinterpret_cast<uint8_t *>(Header), *Span);
  EXPECT_EQ(Header->Kind, HeapKind::Struct);
  EXPECT_EQ(Header->SelfOffset, *A);

  EXPECT_EQ(Heap->getDataSpan(*A, 0, 4096).error(),
            ErrCode::Value::MemoryOutOfBounds);
  EXPECT_FALSE(Heap->getDataSpan(*A + 16, 0, 16));
  EXPECT_FALSE(Heap->getDataSpan(0, 0, 16));

  Heap->free(*A);
  EXPECT_EQ(Heap->getDataSpan(*A, 0, 16).error(),
            ErrCode::Value::MemoryOutOfBounds);
  EXPECT_EQ(Heap->getHeader(*A), nullptr);
  auto C = Heap->allocate(HeapKind::Struct, 48);
  ASSERT_TRUE(C);
  EXPECT_EQ(*C, *A);
}

TEST_P(RefHeapModes, LargeData) {
  MAKE_HEAP_OR_SKIP(Heap);
  auto A = Heap->allocate(HeapKind::Array, 200000);
  ASSERT_TRUE(A);
  EXPECT_TRUE(Heap->getDataSpan(*A, 0, 200000));
  EXPECT_TRUE(Heap->getDataSpan(*A, 199999, 1));
  EXPECT_FALSE(Heap->getDataSpan(*A, 0, 4 * 65536 + 1));
  EXPECT_FALSE(Heap->getDataSpan(*A, 4 * 65536, 1));

  Heap->free(*A);
  EXPECT_FALSE(Heap->getDataSpan(*A, 0, 16));
  std::vector<uint32_t> Small;
  for (uint32_t I = 0; I < 65536 / 64; ++I) {
    auto S = Heap->allocate(HeapKind::Struct, 64);
    ASSERT_TRUE(S);
    Small.push_back(*S);
  }
  EXPECT_FALSE(Heap->getDataSpan(*A, 0, 128));
  EXPECT_FALSE(Heap->getDataSpan(*A, 65536, 16));
}

TEST_P(RefHeapModes, ObjectSlots) {
  MAKE_HEAP_OR_SKIP(Heap);
  auto A = Heap->allocate(HeapKind::Function, 128);
  ASSERT_TRUE(A);
  EXPECT_EQ(*A & RefHeap::kTagObject, RefHeap::kTagObject);
  EXPECT_EQ(*A & RefHeap::kTagI31, 0U);

  auto *Obj =
      static_cast<uint8_t *>(Heap->decodeObject(HeapKind::Function, *A));
  ASSERT_NE(Obj, nullptr);
  auto *Header = Heap->getHeader(*A);
  ASSERT_NE(Header, nullptr);
  EXPECT_EQ(reinterpret_cast<uint8_t *>(Header) + sizeof(HeapHeader), Obj);
  EXPECT_EQ(Header->Kind, HeapKind::Function);
  EXPECT_EQ(Header->SelfOffset, *A);
  EXPECT_EQ(Header->RefCount.load(), 1U);
  EXPECT_EQ(Heap->getOffset(Obj), *A);

  EXPECT_EQ(Heap->decodeObject(HeapKind::Exception, *A), nullptr);
  EXPECT_EQ(Heap->decodeObject(HeapKind::ExternBox, *A), nullptr);
  EXPECT_EQ(Heap->decodeObject(HeapKind::Function, *A + 16), nullptr);
  EXPECT_EQ(Heap->decodeObject(HeapKind::Function, 0), nullptr);
  EXPECT_EQ(Heap->getOffset(Obj + 16), 0U);
  EXPECT_EQ(Heap->getOffset(Obj + 2), 0U);
  EXPECT_EQ(Heap->getOffset(Obj + 4), 0U);

  std::memset(Obj, 0xA5, 256 - sizeof(HeapHeader));
  Heap->free(*A);
  EXPECT_EQ(Heap->decodeObject(HeapKind::Function, *A), nullptr);
  EXPECT_EQ(Heap->getOffset(Obj), 0U);
  for (uint32_t I = 0; I < 256 - sizeof(HeapHeader); ++I) {
    ASSERT_EQ(Obj[I], 0U) << I;
  }

  auto E = Heap->allocate(HeapKind::Exception, 64);
  auto X = Heap->allocate(HeapKind::ExternBox, 40);
  ASSERT_TRUE(E);
  ASSERT_TRUE(X);
  EXPECT_NE(Heap->decodeObject(HeapKind::Exception, *E), nullptr);
  EXPECT_NE(Heap->decodeObject(HeapKind::ExternBox, *X), nullptr);
  EXPECT_EQ(Heap->decodeObject(HeapKind::Function, *E), nullptr);
  EXPECT_EQ(Heap->decodeObject(HeapKind::Function, *X), nullptr);
  EXPECT_FALSE(Heap->allocate(HeapKind::ExternBox, 65));
}

TEST_P(RefHeapModes, Forged) {
  MAKE_HEAP_OR_SKIP(Heap);
  auto D = Heap->allocate(HeapKind::Struct, 32);
  ASSERT_TRUE(D);
  EXPECT_FALSE(Heap->getDataSpan(UINT32_C(0xFFFFFFF0), 0, 16));
  EXPECT_FALSE(Heap->getDataSpan(*D, UINT32_C(0xFFFFFFF0), 32));
  EXPECT_FALSE(Heap->getDataSpan(*D, 16, UINT32_C(0xFFFFFFF8)));
  for (uint32_t Off = 1; Off < 4096; Off += 2) {
    ASSERT_FALSE(Heap->getDataSpan(*D + Off, 0, 1)) << Off;
  }
  EXPECT_FALSE(Heap->getDataSpan(*D | RefHeap::kTagObject, 0, 1));

  std::set<uint32_t> Live;
  for (int I = 0; I < 3; ++I) {
    auto F = Heap->allocate(HeapKind::Function, 64);
    ASSERT_TRUE(F);
    Live.insert(*F);
  }
  const uint32_t First = *Live.begin();
  for (uint32_t Off = First - 2; Off < First - 2 + 65536; Off += 2) {
    const bool Found = Heap->decodeObject(HeapKind::Function, Off) != nullptr;
    ASSERT_EQ(Found, Live.count(Off) == 1) << Off;
  }
  for (uint32_t Off = 0; Off < UINT32_C(1) << 30; Off += UINT32_C(1) << 20) {
    ASSERT_EQ(Heap->decodeObject(HeapKind::Function, Off | 2), nullptr);
  }
  EXPECT_EQ(Heap->getHeader(UINT32_C(0xFFFFFFF2)), nullptr);
  EXPECT_EQ(Heap->getHeader(UINT32_C(0x7FFFFFF0)), nullptr);
}

TEST_P(RefHeapModes, Full) {
  {
    MAKE_HEAP_OR_SKIP(Heap, RefHeap::Limits{4 * 65536, UINT64_C(1) << 30});
    auto A = Heap->allocate(HeapKind::Array, 2 * 65536);
    ASSERT_TRUE(A);
    auto B = Heap->allocate(HeapKind::Array, 2 * 65536);
    ASSERT_FALSE(B);
    EXPECT_EQ(B.error(), ErrCode::Value::RefHeapFull);
    EXPECT_FALSE(Heap->allocate(HeapKind::Array, UINT64_C(1) << 33));
  }
  {
    MAKE_HEAP_OR_SKIP(Heap, RefHeap::Limits{UINT64_C(1) << 32, 65536});
    for (uint32_t I = 0; I < 256; ++I) {
      ASSERT_TRUE(Heap->allocate(HeapKind::Function, 200)) << I;
    }
    auto F = Heap->allocate(HeapKind::Function, 200);
    ASSERT_FALSE(F);
    EXPECT_EQ(F.error(), ErrCode::Value::RefHeapFull);
    EXPECT_TRUE(Heap->allocate(HeapKind::Exception, 64));
  }
}

TEST_P(RefHeapModes, EmptyBlocksReturn) {
  MAKE_HEAP_OR_SKIP(Heap, RefHeap::Limits{4 * 65536, UINT64_C(1) << 30});
  std::vector<uint32_t> Small;
  while (auto S = Heap->allocate(HeapKind::Struct, 64)) {
    Small.push_back(*S);
  }
  EXPECT_EQ(Small.size(), 3U * 65536 / 64);
  for (const auto Off : Small) {
    Heap->free(Off);
  }
  auto A = Heap->allocate(HeapKind::Array, 3 * 65536);
  ASSERT_TRUE(A);
  EXPECT_TRUE(Heap->getDataSpan(*A, 0, 3 * 65536));
  Heap->free(*A);
  for (uint32_t I = 0; I < 3 * 65536 / 128; ++I) {
    ASSERT_TRUE(Heap->allocate(HeapKind::Struct, 128)) << I;
  }
  EXPECT_FALSE(Heap->allocate(HeapKind::Struct, 64));
}

TEST_P(RefHeapModes, AllocationFailureKeepsBlocks) {
  MAKE_HEAP_OR_SKIP(Heap, RefHeap::Limits{4 * 65536, UINT64_C(1) << 30});
  const uint64_t Before = NewCount;
  AllowedNews = 0;
  auto First = Heap->allocate(HeapKind::Array, 2 * 65536);
  AllowedNews = -1;
  if (NewCount == Before) {
    GTEST_SKIP() << "the replaced operator new does not reach the heap";
  }
  ASSERT_FALSE(First);
  EXPECT_EQ(First.error(), ErrCode::Value::RefHeapFull);

  for (int N = 1; N < 16; ++N) {
    std::vector<uint32_t> Offs;
    Offs.reserve(4);
    AllowedNews = N;
    for (const uint64_t Size :
         {UINT64_C(40), UINT64_C(2) * 65536, UINT64_C(64), UINT64_C(1000)}) {
      if (auto Off = Heap->allocate(HeapKind::Struct, Size)) {
        Offs.push_back(*Off);
      }
    }
    AllowedNews = -1;
    for (const auto Off : Offs) {
      Heap->free(Off);
    }
  }
  auto All = Heap->allocate(HeapKind::Array, 3 * 65536);
  ASSERT_TRUE(All);
  Heap->free(*All);

  MAKE_HEAP_OR_SKIP(Fresh, RefHeap::Limits{4 * 65536, UINT64_C(1) << 30});
  auto Large = Fresh->allocate(HeapKind::Array, 2 * 65536);
  ASSERT_TRUE(Large);
  AllowedNews = 0;
  Fresh->free(*Large);
  AllowedNews = -1;
  EXPECT_EQ(Fresh->getHeader(*Large), nullptr);
  EXPECT_FALSE(Fresh->getDataSpan(*Large, 0, 32));
  auto Small = Fresh->allocate(HeapKind::Struct, 64);
  ASSERT_TRUE(Small);
  AllowedNews = 0;
  Fresh->free(*Small);
  AllowedNews = -1;
  EXPECT_EQ(Fresh->getHeader(*Small), nullptr);
  auto Again = Fresh->allocate(HeapKind::Struct, 64);
  ASSERT_TRUE(Again);
  EXPECT_NE(Fresh->getHeader(*Again), nullptr);
}

TEST_P(RefHeapModes, LoadStore) {
  MAKE_HEAP_OR_SKIP(Heap);
  auto A = Heap->allocate(HeapKind::Struct, 64);
  ASSERT_TRUE(A);
  ASSERT_TRUE(Heap->store<uint64_t>(*A, 32, UINT64_C(0x0123456789ABCDEF)));
  auto V = Heap->load<uint64_t>(*A, 32);
  ASSERT_TRUE(V);
  EXPECT_EQ(*V, UINT64_C(0x0123456789ABCDEF));
  EXPECT_FALSE(Heap->load<uint32_t>(*A, 62));
  EXPECT_FALSE(Heap->store<uint32_t>(*A, 62, 1));
}

TEST_P(RefHeapModes, Threads) {
  MAKE_HEAP_OR_SKIP(Heap);
  constexpr int kThreads = 4;
  std::vector<std::vector<uint32_t>> Kept(kThreads);
  std::vector<std::thread> Workers;
  for (int T = 0; T < kThreads; ++T) {
    Workers.emplace_back([&, T]() {
      const HeapKind Kinds[] = {HeapKind::Struct, HeapKind::Array,
                                HeapKind::Function, HeapKind::ExternBox};
      const uint64_t Sizes[] = {48, 70000, 200, 40};
      for (int I = 0; I < 1000; ++I) {
        const auto K = static_cast<size_t>(I % 4);
        auto Off = Heap->allocate(Kinds[K], Sizes[K]);
        if (!Off) {
          continue;
        }
        if (I % 2 == 0) {
          Kept[static_cast<size_t>(T)].push_back(*Off);
        } else {
          Heap->free(*Off);
        }
      }
    });
  }
  for (auto &W : Workers) {
    W.join();
  }
  std::set<uint32_t> All;
  size_t Count = 0;
  for (auto &V : Kept) {
    Count += V.size();
    All.insert(V.begin(), V.end());
  }
  EXPECT_EQ(Count, static_cast<size_t>(kThreads) * 500);
  EXPECT_EQ(All.size(), Count);
  for (auto Off : All) {
    ASSERT_NE(Heap->getHeader(Off), nullptr) << Off;
  }
}

TEST_P(RefHeapModes, LargeSlotRace) {
  MAKE_HEAP_OR_SKIP(Heap);
  std::atomic<uint32_t> Probe{0};
  std::atomic<bool> Stop{false};
  std::atomic<uint32_t> Hits{0};
  std::thread Reader([&]() {
    while (!Stop.load(std::memory_order_relaxed)) {
      const uint32_t Off = Probe.load(std::memory_order_relaxed);
      if (Off != 0 && Heap->getDataSpan(Off, 65536, 1)) {
        Hits.fetch_add(1, std::memory_order_relaxed);
      }
    }
  });
  for (int I = 0; I < 20000; ++I) {
    auto A = Heap->allocate(HeapKind::Array, 2 * 65536);
    ASSERT_TRUE(A);
    Probe.store(*A + 65536, std::memory_order_relaxed);
    Heap->free(*A);
    ASSERT_TRUE(Heap->allocate(HeapKind::Array, 65536));
    ASSERT_TRUE(Heap->allocate(HeapKind::Array, 65536));
  }
  Stop.store(true);
  Reader.join();
  EXPECT_EQ(Hits.load(), 0U);
}

TEST(RefHeap, Global) {
  auto *Heap = RefHeap::getGlobal();
  ASSERT_NE(Heap, nullptr);
  EXPECT_EQ(RefHeap::getGlobal(), Heap);
  EXPECT_EQ(Heap->getMode(), RefHeap::getDefaultMode());
}

TEST(RefHeap, Modes) {
  EXPECT_TRUE(RefHeap::isModeSupported(HeapMode::Chunk));
  EXPECT_EQ(RefHeap::isModeSupported(HeapMode::Flat),
            WASMEDGE_ALLOCATOR_IS_STABLE && sizeof(void *) == 8);
  if (!RefHeap::isModeSupported(HeapMode::Flat)) {
    EXPECT_EQ(RefHeap::getDefaultMode(), HeapMode::Chunk);
  }
}

TEST(RefHeap, ChunkRunContiguous) {
  MAKE_CHUNK_HEAP(Heap);
  EXPECT_EQ(Heap->getMode(), HeapMode::Chunk);
  auto S = Heap->allocate(HeapKind::Struct, 64);
  ASSERT_TRUE(S);
  EXPECT_EQ(*S, UINT32_C(1) << 16);
  const uint32_t Size = 20 * 65536;
  auto A = Heap->allocate(HeapKind::Array, Size);
  ASSERT_TRUE(A);
  EXPECT_EQ(*A, RefHeap::kChunkSize);
  auto Span = Heap->getDataSpan(*A, 0, Size);
  ASSERT_TRUE(Span);
  std::memset(*Span, 0x5A, Size);
  auto Last = Heap->load<uint8_t>(*A, Size - 1);
  ASSERT_TRUE(Last);
  EXPECT_EQ(*Last, 0x5AU);
  EXPECT_FALSE(Heap->getDataSpan(*A, 0, Size + 65536));
  auto B = Heap->allocate(HeapKind::Array, 10 * 65536);
  ASSERT_TRUE(B);
  EXPECT_EQ(*B, UINT32_C(2) << 16);
  auto BSpan = Heap->getDataSpan(*B, 0, 10 * 65536);
  ASSERT_TRUE(BSpan);
  std::memset(*BSpan, 0x3C, 10 * 65536);
  EXPECT_EQ(Heap->getOffset(*BSpan + sizeof(HeapHeader)), *B);
}

TEST(RefHeap, ChunkObjectParts) {
  MAKE_CHUNK_HEAP(Heap);
  const uint32_t PerChunk = RefHeap::kChunkSize / 256;
  std::vector<uint32_t> Offs;
  for (uint32_t I = 0; I <= PerChunk; ++I) {
    auto F = Heap->allocate(HeapKind::Function, 256);
    ASSERT_TRUE(F) << I;
    Offs.push_back(*F);
  }
  EXPECT_EQ(Offs.back() - Offs.front(), RefHeap::kChunkSize);
  for (const uint32_t Off : {Offs.front(), Offs[PerChunk - 1], Offs.back()}) {
    auto *Obj =
        static_cast<uint8_t *>(Heap->decodeObject(HeapKind::Function, Off));
    ASSERT_NE(Obj, nullptr) << Off;
    std::memset(Obj, 0x11, 256 - sizeof(HeapHeader));
    EXPECT_EQ(Heap->getOffset(Obj), Off);
    EXPECT_EQ(Heap->getHeader(Off)->SelfOffset, Off);
  }
  auto *Last = static_cast<uint8_t *>(
      Heap->decodeObject(HeapKind::Function, Offs[PerChunk - 1]));
  EXPECT_EQ(Heap->getOffset(Last + 256), 0U);
  int Local = 0;
  EXPECT_EQ(Heap->getOffset(&Local), 0U);
}

TEST_P(RefHeapModes, Unique) {
  MAKE_HEAP_OR_SKIP(Heap);
  int Count = 0;
  {
    auto P = Heap->makeUnique<Counted>(Count, UINT64_C(42));
    ASSERT_TRUE(P);
    ASSERT_TRUE(*P);
    EXPECT_EQ((*P)->Value, 42U);
    const uint32_t Off = P->getOffset();
    EXPECT_EQ(Heap->decodeObject(HeapKind::Function, Off), P->get());
    EXPECT_EQ(Heap->getOffset(P->get()), Off);

    Runtime::UniqueHeapPtr<Counted> Q = std::move(*P);
    EXPECT_FALSE(*P);
    EXPECT_EQ(P->get(), nullptr);
    EXPECT_EQ(P->getOffset(), 0U);
    EXPECT_EQ(Q.getOffset(), Off);
    EXPECT_EQ(Q->Value, 42U);
    EXPECT_EQ((*Q).Value, 42U);
    EXPECT_EQ(Count, 0);

    Q.reset();
    EXPECT_EQ(Count, 1);
    EXPECT_FALSE(Q);
    EXPECT_EQ(Heap->decodeObject(HeapKind::Function, Off), nullptr);
    Q.reset();
    EXPECT_EQ(Count, 1);
  }
  EXPECT_EQ(Count, 1);
  {
    auto P = Heap->makeUnique<Counted>(Count, UINT64_C(1));
    ASSERT_TRUE(P);
    auto Q = Heap->makeUnique<Counted>(Count, UINT64_C(2));
    ASSERT_TRUE(Q);
    *P = std::move(*Q);
    EXPECT_EQ(Count, 2);
    EXPECT_EQ((*P)->Value, 2U);
  }
  EXPECT_EQ(Count, 3);
}

TEST_P(RefHeapModes, NotLiveWhileBuilt) {
  MAKE_HEAP_OR_SKIP(Heap);
  uint32_t Built = 1;
  uint32_t Destroyed = 1;
  auto P = Heap->makeUnique<Probe>(*Heap, Built, Destroyed);
  ASSERT_TRUE(P);
  EXPECT_EQ(Built, 0U);
  EXPECT_EQ(Heap->getOffset(P->get()), P->getOffset());
  const uint32_t Off = P->getOffset();
  P->reset();
  EXPECT_EQ(Destroyed, 0U);
  EXPECT_EQ(Heap->decodeObject(HeapKind::Function, Off), nullptr);
  auto Q = Heap->allocate(HeapKind::Function, 64);
  ASSERT_TRUE(Q);
  EXPECT_EQ(*Q, Off);
  Heap->free(Off);
  Heap->free(Off);
  auto R1 = Heap->allocate(HeapKind::Function, 64);
  auto R2 = Heap->allocate(HeapKind::Function, 64);
  ASSERT_TRUE(R1);
  ASSERT_TRUE(R2);
  EXPECT_NE(*R1, *R2);
}

TEST_P(RefHeapModes, Shared) {
  MAKE_HEAP_OR_SKIP(Heap);
  int Count = 0;
  {
    auto P = Heap->makeShared<Counted>(Count, UINT64_C(5));
    ASSERT_TRUE(P);
    const uint32_t Off = P->getOffset();
    auto *Header = Heap->getHeader(Off);
    ASSERT_NE(Header, nullptr);
    EXPECT_EQ(P->getUseCount(), 1U);
    {
      Runtime::SharedHeapPtr<Counted> Q = *P;
      EXPECT_EQ(P->getUseCount(), 2U);
      EXPECT_EQ(Header->RefCount.load(), 2U);
      EXPECT_EQ(Q.get(), P->get());
      Runtime::SharedHeapPtr<Counted> R;
      R = Q;
      EXPECT_EQ(Q.getUseCount(), 3U);
      Runtime::SharedHeapPtr<Counted> S = std::move(R);
      EXPECT_FALSE(R);
      EXPECT_EQ(S.getUseCount(), 3U);
    }
    EXPECT_EQ(Count, 0);
    EXPECT_EQ(P->getUseCount(), 1U);
    P->reset();
    EXPECT_EQ(Count, 1);
    EXPECT_EQ(Heap->getHeader(Off), nullptr);
  }
  EXPECT_EQ(Count, 1);
}

TEST_P(RefHeapModes, WithTail) {
  MAKE_HEAP_OR_SKIP(Heap);
  auto P = Heap->makeUniqueWithTail<Tailed>(64);
  ASSERT_TRUE(P);
  EXPECT_EQ((*P)->Head, 7U);
  const uint32_t Off = P->getOffset();
  EXPECT_EQ(Off & 15, 0U);
  const uint32_t End =
      static_cast<uint32_t>(sizeof(HeapHeader) + sizeof(Tailed) + 64);
  EXPECT_TRUE(Heap->getDataSpan(Off, 0, End));
  EXPECT_EQ(Heap->getOffset(P->get()), Off);
  auto S = Heap->makeSharedWithTail<Tailed>(0);
  ASSERT_TRUE(S);
  EXPECT_EQ((*S)->Head, 7U);
}

TEST_P(RefHeapModes, ReleaseAdopt) {
  MAKE_HEAP_OR_SKIP(Heap);
  int Count = 0;
  auto P = Heap->makeUnique<Counted>(Count, UINT64_C(9));
  ASSERT_TRUE(P);
  const uint32_t Off = P->getOffset();
  Counted *Raw = P->release();
  EXPECT_FALSE(*P);
  EXPECT_EQ(P->getOffset(), 0U);
  EXPECT_EQ(Count, 0);
  EXPECT_NE(Heap->getHeader(Off), nullptr);

  auto Q = Heap->adopt(Raw);
  ASSERT_TRUE(Q);
  EXPECT_EQ(Q.get(), Raw);
  EXPECT_EQ(Q.getOffset(), Off);
  EXPECT_EQ(Q->Value, 9U);
  Q.reset();
  EXPECT_EQ(Count, 1);
  EXPECT_EQ(Heap->getHeader(Off), nullptr);
}

TEST_P(RefHeapModes, AdoptForeign) {
  MAKE_HEAP_OR_SKIP(Heap);
  MAKE_HEAP_OR_SKIP(Other);
  int Count = 0;
  Counted Local(Count, UINT64_C(1));
  EXPECT_FALSE(Heap->adopt(&Local));
  EXPECT_FALSE(Heap->adopt(static_cast<Counted *>(nullptr)));

  auto P = Other->makeUnique<Counted>(Count, UINT64_C(2));
  ASSERT_TRUE(P);
  EXPECT_FALSE(Heap->adopt(P->get()));
  EXPECT_EQ(Count, 0);

  auto H = Heap->makeUnique<Hidden>(UINT64_C(3));
  ASSERT_TRUE(H);
  EXPECT_FALSE(Heap->adopt(reinterpret_cast<Counted *>(H->get())));
  EXPECT_EQ(Count, 0);
}

TEST_P(RefHeapModes, PrivateConstructor) {
  MAKE_HEAP_OR_SKIP(Heap);
  auto P = Heap->makeUnique<Hidden>(UINT64_C(11));
  ASSERT_TRUE(P);
  EXPECT_EQ((*P)->getValue(), 11U);
  EXPECT_EQ(Heap->decodeObject(HeapKind::Exception, P->getOffset()), P->get());
}

TEST_P(RefHeapModes, MakeData) {
  MAKE_HEAP_OR_SKIP(Heap);
  auto P = Heap->makeData<TestData>(7U, 3U, 24U);
  ASSERT_TRUE(P);
  const uint32_t Off = P->getOffset();
  EXPECT_NE(Off, 0U);
  EXPECT_EQ(Off & (RefHeap::kTagI31 | RefHeap::kTagObject), 0U);
  EXPECT_EQ(Heap->getOffset(P->get()), Off);
  const auto *Header = Heap->getHeader(Off);
  ASSERT_NE(Header, nullptr);
  EXPECT_EQ(Header->Kind, HeapKind::Struct);
  EXPECT_EQ(Header->TypeId, 7U);
  EXPECT_EQ(Header->Length, 3U);
  for (uint32_t Pos = sizeof(HeapHeader); Pos < sizeof(HeapHeader) + 24U;
       ++Pos) {
    auto B = Heap->load<uint8_t>(Off, Pos);
    ASSERT_TRUE(B);
    EXPECT_EQ(*B, 0U);
  }
  ASSERT_TRUE(Heap->store<uint64_t>(Off, sizeof(HeapHeader) + 8U,
                                    UINT64_C(0x1122334455667788)));
  auto V = Heap->load<uint64_t>(Off, sizeof(HeapHeader) + 8U);
  ASSERT_TRUE(V);
  EXPECT_EQ(*V, UINT64_C(0x1122334455667788));
  EXPECT_FALSE(Heap->load<uint8_t>(Off, 64U));
}

TEST_P(RefHeapModes, EmptyDataSlot) {
  MAKE_HEAP_OR_SKIP(Heap);
  auto P = Heap->makeData<TestData>(1U, 0U, 0U);
  ASSERT_TRUE(P);
  const auto *Header = Heap->getHeader(P->getOffset());
  ASSERT_NE(Header, nullptr);
  EXPECT_EQ(Header->Kind, HeapKind::Struct);
  EXPECT_EQ(Heap->getOffset(P->get()), P->getOffset());
}

TEST_P(RefHeapModes, DataSlotTooBig) {
  MAKE_HEAP_OR_SKIP(Heap);
  auto P = Heap->makeData<TestData>(1U, 0U, UINT64_C(1) << 32);
  ASSERT_FALSE(P);
  EXPECT_EQ(P.error(), ErrCode::Value::RefHeapFull);
}

TEST_P(RefHeapModes, DataSlotReset) {
  MAKE_HEAP_OR_SKIP(Heap);
  auto P = Heap->makeData<TestData>(1U, 0U, 16U);
  ASSERT_TRUE(P);
  const auto *Obj = P->get();
  const uint32_t Off = P->getOffset();
  P->reset();
  EXPECT_EQ(Heap->getOffset(Obj), 0U);
  EXPECT_EQ(Heap->getHeader(Off), nullptr);
}

TEST_P(RefHeapModes, Descriptor) {
  MAKE_HEAP_OR_SKIP(Heap);
  int Count = 0;
  auto P = Heap->makeUnique<Counted>(Count, UINT64_C(4));
  ASSERT_TRUE(P);
  const uint32_t Off = P->getOffset();
  const auto &D = Heap->getDescriptor();
  ASSERT_NE(D.FuncTop, nullptr);
  const uint32_t Rel = Off - D.FuncPartBase;
  EXPECT_LT(Rel, D.FuncTop->load());
  EXPECT_EQ(Rel & D.FuncSlotMask, 0U);
  EXPECT_EQ(D.FuncSlotMask + 1U, RefHeap::getSlotSize(HeapKind::Function));
  const uint32_t Raw = Off & ~RefHeap::kTagObject;
  uint8_t *Slot = nullptr;
  if (GetParam() == HeapMode::Flat) {
    ASSERT_NE(D.ObjectBase, nullptr);
    EXPECT_EQ(D.ObjectTable, nullptr);
    Slot = D.ObjectBase + Raw;
  } else {
    ASSERT_NE(D.ObjectTable, nullptr);
    EXPECT_EQ(D.ObjectBase, nullptr);
    Slot = D.ObjectTable[Raw >> RefHeap::kChunkShift].load() +
           (Raw & (RefHeap::kChunkSize - 1));
  }
  EXPECT_EQ(static_cast<void *>(Slot + sizeof(HeapHeader)),
            static_cast<void *>(P->get()));
  EXPECT_GE(D.FuncTop->load(), Rel + D.FuncSlotMask + 1U);
}

TEST_P(RefHeapModes, ExternBoxes) {
  MAKE_HEAP_OR_SKIP(Heap);
  int HostA = 0;
  int HostB = 0;
  auto A1 = Heap->boxExtern(&HostA);
  auto A2 = Heap->boxExtern(&HostA);
  auto B = Heap->boxExtern(&HostB);
  ASSERT_TRUE(A1);
  ASSERT_TRUE(A2);
  ASSERT_TRUE(B);
  ASSERT_NE(*A1, nullptr);
  EXPECT_EQ(*A1, *A2);
  EXPECT_NE(*A1, *B);
  const uint32_t Off = Heap->getOffset(*A1);
  EXPECT_LT(Off - RefHeap::getPartBase(HeapKind::ExternBox),
            RefHeap::kPartSize);
  EXPECT_EQ(Heap->decodeObject(HeapKind::ExternBox, Off), *A1);
  EXPECT_EQ(Heap->unboxExtern(*A1), &HostA);
  EXPECT_EQ(Heap->unboxExtern(*B), &HostB);

  auto Null = Heap->boxExtern(nullptr);
  ASSERT_TRUE(Null);
  EXPECT_EQ(*Null, nullptr);
  EXPECT_EQ(Heap->unboxExtern(nullptr), nullptr);
  EXPECT_EQ(Heap->unboxExtern(&HostA), nullptr);
  int Count = 0;
  auto F = Heap->makeUnique<Counted>(Count, UINT64_C(1));
  ASSERT_TRUE(F);
  EXPECT_EQ(Heap->unboxExtern(F->get()), nullptr);
}

TEST_P(RefHeapModes, ExternBoxesFillPartition) {
  MAKE_HEAP_OR_SKIP(Heap, RefHeap::Limits{4 * 65536, 65536});
  const uint32_t Slots = 65536 / RefHeap::getSlotSize(HeapKind::ExternBox);
  std::vector<uint8_t> Hosts(Slots + 1);
  uint32_t Boxed = 0;
  for (; Boxed <= Slots; ++Boxed) {
    auto Box = Heap->boxExtern(&Hosts[Boxed]);
    if (!Box) {
      EXPECT_EQ(Box.error(), ErrCode::Value::RefHeapFull);
      break;
    }
  }
  ASSERT_GT(Boxed, 0U);
  EXPECT_LE(Boxed, Slots);
  auto Again = Heap->boxExtern(&Hosts[0]);
  ASSERT_TRUE(Again);
  EXPECT_EQ(Heap->unboxExtern(*Again), &Hosts[0]);
}

TEST_P(RefHeapModes, ExternBoxThreads) {
  MAKE_HEAP_OR_SKIP(Heap);
  constexpr uint32_t kThreads = 8;
  constexpr uint32_t kHosts = 64;
  std::vector<uint64_t> Hosts(kHosts);
  std::vector<std::vector<const void *>> Seen(kThreads);
  std::vector<std::thread> Workers;
  for (uint32_t T = 0; T < kThreads; ++T) {
    Workers.emplace_back([&, T] {
      for (uint32_t I = 0; I < kHosts; ++I) {
        auto Box = Heap->boxExtern(&Hosts[(I + T * 7) % kHosts]);
        Seen[T].push_back(Box ? static_cast<const void *>(*Box) : nullptr);
      }
    });
  }
  for (auto &W : Workers) {
    W.join();
  }
  for (uint32_t I = 0; I < kHosts; ++I) {
    auto Box = Heap->boxExtern(&Hosts[I]);
    ASSERT_TRUE(Box);
    for (uint32_t T = 0; T < kThreads; ++T) {
      const uint32_t Pos = (I + kHosts - (T * 7) % kHosts) % kHosts;
      EXPECT_EQ(Seen[T][Pos], *Box);
    }
    EXPECT_EQ(Heap->unboxExtern(*Box), &Hosts[I]);
  }
}

INSTANTIATE_TEST_SUITE_P(Modes, RefHeapModes,
                         ::testing::Values(HeapMode::Flat, HeapMode::Chunk),
                         [](const ::testing::TestParamInfo<HeapMode> &Info) {
                           return Info.param == HeapMode::Flat ? "Flat"
                                                               : "Chunk";
                         });

TEST(RefHeap, AttachGlobal) {
  auto *Heap = RefHeap::getGlobal();
  ASSERT_NE(Heap, nullptr);
  EXPECT_TRUE(RefHeap::attachGlobal(Heap));
  EXPECT_EQ(RefHeap::getGlobal(), Heap);
  RefHeap::Limits L;
  L.Mode = HeapMode::Chunk;
  auto Other = RefHeap::create(L);
  ASSERT_TRUE(Other);
  EXPECT_FALSE(RefHeap::attachGlobal(Other->get()));
  EXPECT_FALSE(RefHeap::attachGlobal(nullptr));
  EXPECT_EQ(RefHeap::getGlobal(), Heap);

  auto &Types = Runtime::CanonicalTypeTable::getGlobal();
  EXPECT_TRUE(Runtime::CanonicalTypeTable::attachGlobal(&Types));
  Runtime::CanonicalTypeTable OtherTypes;
  EXPECT_FALSE(Runtime::CanonicalTypeTable::attachGlobal(&OtherTypes));
  EXPECT_EQ(&Runtime::CanonicalTypeTable::getGlobal(), &Types);

  EXPECT_TRUE(wasmedge_refheap_attach(Heap, &Types));
  EXPECT_FALSE(wasmedge_refheap_attach(Other->get(), &Types));
  EXPECT_FALSE(wasmedge_refheap_attach(Heap, &OtherTypes));
}

} // namespace
