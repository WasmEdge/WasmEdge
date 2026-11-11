// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "runtime/refheap.h"

#include "common/config.h"
#include "system/allocator.h"

#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
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

} // namespace

namespace WasmEdge::Runtime {
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

std::unique_ptr<RefHeap> makeHeap(const RefHeap::Limits &L = {}) {
  auto Res = RefHeap::create(L);
  if (!Res) {
    return nullptr;
  }
  return std::move(*Res);
}

constexpr bool kHasFlatMode = WASMEDGE_ALLOCATOR_IS_STABLE;

#define MAKE_HEAP_OR_SKIP(Name, ...)                                           \
  auto Name = makeHeap(__VA_ARGS__);                                           \
  if (!Name) {                                                                 \
    if (kHasFlatMode) {                                                        \
      FAIL() << "heap creation failed";                                        \
    }                                                                          \
    GTEST_SKIP() << "flat mode is not available on this host";                 \
  }

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

TEST(RefHeap, DataSlots) {
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

TEST(RefHeap, LargeData) {
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

TEST(RefHeap, ObjectSlots) {
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

TEST(RefHeap, Forged) {
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

TEST(RefHeap, Full) {
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

TEST(RefHeap, LoadStore) {
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

TEST(RefHeap, Threads) {
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

TEST(RefHeap, LargeSlotRace) {
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
  if (Heap == nullptr) {
    ASSERT_FALSE(kHasFlatMode);
    GTEST_SKIP() << "flat mode is not available on this host";
  }
  EXPECT_EQ(RefHeap::getGlobal(), Heap);
}

TEST(RefHeap, Unique) {
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

TEST(RefHeap, NotLiveWhileBuilt) {
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

TEST(RefHeap, Shared) {
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

TEST(RefHeap, WithTail) {
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

} // namespace
