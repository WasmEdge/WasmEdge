// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/gc/coherent_slot.h - Coherent (type,pointer) slot access -===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// A managed struct/array field is a 128-bit RefVariant: the low 64-bit word
/// (Raw[0]) is the type tag (Externalize bit + heap type), the high word
/// (Raw[1]) is the object pointer. During concurrent marking a collector worker
/// reads such a field while a mutator may struct.set/array.set the SAME slot.
/// Two *independent* word accesses can fabricate a torn (type, pointer) pair --
/// e.g. the new type with the old pointer -- that the runtime would then
/// dereference, a memory-safety break.
///
/// This header provides the coherent-pair accessors that close that gap:
///   * loadPointerWordRelaxed -- the marker's fast path: a single relaxed
///     atomic load of the pointer word only (no pair, no retry).
///   * loadCoherent / storeCoherent -- read/write BOTH words as one atomic
///     transaction, so a full-reference reader never observes a torn pair.
///
/// Design:
///
///  PRIMARY path -- a single 16-byte atomic on the whole slot. It is coherent
///  by construction (no seqlock, no retry) and, crucially, TSan-clean: under
///  -fsanitize=thread the compiler lowers the 16-byte __atomic op to
///  __tsan_atomic128_*, and the marker's 8-byte __atomic load lowers to
///  __tsan_atomic64_load. TSan never reports a race between two *atomic*
///  accesses regardless of their differing sizes, so the marker-vs-writer word
///  overlap is race-free. This path is selected ONLY when the toolchain offers
///  a genuinely inline-lock-free double-width CAS (x86-64 cmpxchg16b via
///  -mcx16, aarch64 casp/ldxp-stxp, or the MSVC 128-bit interlocked intrinsic);
///  it never degrades to an __atomic_*_16 libcall (a link error under Windows
///  lld, a hidden lock on Linux). Where no such lock-free CAS exists the
///  FALLBACK path below is used instead.
///
///  FALLBACK path -- only for a platform that offers neither the GCC/Clang
///  16-byte __atomic builtins nor the MSVC 128-bit interlocked intrinsic. It is
///  a striped SPINLOCK (NOT a bare seqlock): both reader and writer take the
///  per-stripe lock, so same-stripe writers serialize and a reader always sees
///  a whole pair. (A bare seqlock with a shared, non-atomically-incremented
///  stripe counter races itself across slots that hash to one stripe -- one
///  writer can drive the sequence even while another is mid-write, letting a
///  reader accept a torn value under a "stable" sequence. The spinlock avoids
///  that class of bug entirely.) Every word is still accessed with a relaxed
///  atomic so the lockless marker read never forms a data race with a locked
///  writer's store.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/types.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>

// Both MSVC and clang-cl (which also defines _MSC_VER) expose
// _InterlockedCompareExchange128 / __iso_volatile_load64 via <intrin.h>.
#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace WasmEdge {
namespace GC {

// A managed slot is exactly a 128-bit RefVariant payload: 16 bytes, 16-byte
// aligned, no discriminant (ValVariant is a tagless union). Keep in sync with
// RefVariant in common/types.h.
static_assert(sizeof(ValVariant) == 16,
              "coherent slot access assumes a 16-byte ValVariant payload");
static_assert(alignof(ValVariant) >= 16,
              "coherent slot access assumes 16-byte ValVariant alignment");

// The per-word atomic helpers below (marker load, and the spinlock fallback's
// word load/store) require a real atomic primitive for a plain uint64_t that
// lives in non-atomic storage: GCC/Clang __atomic_* or the MSVC volatile
// intrinsics. Reinterpreting that storage as std::atomic<uint64_t>* and calling
// through it is undefined under the C++17 object model (no atomic object was
// constructed there), so there is no valid generic fallback. WasmEdge supports
// GCC, Clang, and MSVC (incl. clang-cl) only; reject any other toolchain at
// compile time rather than emit that invalid access.
#if !defined(__clang__) && !defined(__GNUC__) && !defined(_MSC_VER)
#error "coherent_slot.h: no atomic word primitive for this toolchain. "         \
    "WasmEdge supports GCC, Clang, and MSVC only; provide __atomic_* or "       \
    "MSVC volatile-load/store equivalents before targeting it."
#endif

//===----------------------------------------------------------------------===//
// Marker path: single relaxed atomic load of the pointer word (Raw[1]).
//===----------------------------------------------------------------------===//
inline uint8_t *loadPointerWordRelaxed(const ValVariant &Slot) noexcept {
  const auto *P = reinterpret_cast<const uint64_t *>(&Slot);
#if defined(__clang__) || defined(__GNUC__)
  const uint64_t Hi = __atomic_load_n(&P[1], __ATOMIC_RELAXED);
#elif defined(_MSC_VER)
  const uint64_t Hi = static_cast<uint64_t>(
      __iso_volatile_load64(reinterpret_cast<const long long *>(&P[1])));
#else
#error "unreachable: unsupported toolchain rejected by the guard above"
#endif
  return reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(Hi));
}

namespace detail {

// Whole-slot atomic transaction as a pair of raw words {Raw[0], Raw[1]}.
using SlotWords = std::array<uint64_t, 2>;

// Path selection (see file header). The lock-free 128-bit path is chosen ONLY
// when the toolchain genuinely provides a lock-free 16-byte CAS, so no path
// ever degrades to an __atomic_*_16 libcall (a link error under Windows lld, a
// hidden lock on Linux):
//   * MSVC cl.exe (x64/arm64): _InterlockedCompareExchange128 -- always inline
//     lock-free (cmpxchg16b / casp).
//   * GCC/Clang with __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16 (x86-64 built with
//     -mcx16, or aarch64): generic 16-byte __atomic, inline lock-free; under
//     -fsanitize=thread it lowers to __tsan_atomic128_* (modeled, race-clean).
//   * Otherwise (x86-64 without -mcx16 -- the default on Linux AND clang-cl):
//     the striped spinlock. Correct and TSan-clean; not lock-free. Adding
//     -mcx16 to this TU on x86-64 promotes it to the lock-free __atomic path.
#if defined(_MSC_VER) && !defined(__clang__) &&                                \
    (defined(_M_X64) || defined(_M_ARM64))
#define WASMEDGE_GC_COHERENT_MSVC128 1
#elif (defined(__clang__) || defined(__GNUC__)) &&                             \
    defined(__SIZEOF_INT128__) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16)
#define WASMEDGE_GC_COHERENT_INT128 1
#else
#define WASMEDGE_GC_COHERENT_SPINLOCK 1
#endif

#if defined(WASMEDGE_GC_COHERENT_INT128)

inline SlotWords loadWords(const void *Addr) noexcept {
  using U128 = unsigned __int128;
  const U128 V =
      __atomic_load_n(reinterpret_cast<const U128 *>(Addr), __ATOMIC_ACQUIRE);
  SlotWords R;
  std::memcpy(R.data(), &V, sizeof(V));
  return R;
}
inline void storeWords(void *Addr, SlotWords W) noexcept {
  using U128 = unsigned __int128;
  U128 V;
  std::memcpy(&V, W.data(), sizeof(V));
  __atomic_store_n(reinterpret_cast<U128 *>(Addr), V, __ATOMIC_RELEASE);
}

#elif defined(WASMEDGE_GC_COHERENT_MSVC128)

// _InterlockedCompareExchange128 (cmpxchg16b on x64, casp/ldaxp-stlxp on ARM64)
// provides both load and store. Load is a CAS whose "exchange" value equals the
// comparand, so a match is a no-op write and a miss returns the current value
// in ComparandResult -- non-destructive either way.
inline SlotWords loadWords(const void *Addr) noexcept {
  __int64 Cmp[2] = {0, 0}; // [0]=low (Raw[0]), [1]=high (Raw[1])
  _InterlockedCompareExchange128(
      const_cast<__int64 *>(reinterpret_cast<const __int64 *>(Addr)),
      /*ExchangeHigh=*/0, /*ExchangeLow=*/0, Cmp);
  return SlotWords{static_cast<uint64_t>(Cmp[0]),
                   static_cast<uint64_t>(Cmp[1])};
}
inline void storeWords(void *Addr, SlotWords W) noexcept {
  auto *D = reinterpret_cast<__int64 *>(Addr);
  __int64 Cmp[2] = {0, 0};
  // Retry until we win the CAS; each failure refreshes Cmp with the observed
  // value, so this converges to a single coherent 128-bit store.
  while (!_InterlockedCompareExchange128(D, static_cast<__int64>(W[1]),
                                         static_cast<__int64>(W[0]), Cmp)) {
  }
}

#elif defined(WASMEDGE_GC_COHERENT_SPINLOCK)

// Striped spinlock fallback (see file header). One lock per stripe; the slot
// address hashes to a stripe. Writers on the same stripe serialize; readers
// take the same lock and thus never see a partial pair.
inline std::array<std::atomic_flag, 1024> &seqStripes() noexcept {
  static std::array<std::atomic_flag, 1024> S{};
  return S;
}
inline std::atomic_flag &stripeFor(const void *Slot) noexcept {
  const auto H = (reinterpret_cast<uintptr_t>(Slot) >> 4) & 1023U;
  return seqStripes()[H];
}
inline void lockStripe(std::atomic_flag &F) noexcept {
  while (F.test_and_set(std::memory_order_acquire)) {
  }
}
inline void unlockStripe(std::atomic_flag &F) noexcept {
  F.clear(std::memory_order_release);
}
inline uint64_t loadWordRelaxed(const uint64_t *P) noexcept {
#if defined(__clang__) || defined(__GNUC__)
  return __atomic_load_n(P, __ATOMIC_RELAXED);
#elif defined(_MSC_VER)
  return static_cast<uint64_t>(
      __iso_volatile_load64(reinterpret_cast<const long long *>(P)));
#else
#error "unreachable: unsupported toolchain rejected by the guard above"
#endif
}
inline void storeWordRelaxed(uint64_t *P, uint64_t V) noexcept {
#if defined(__clang__) || defined(__GNUC__)
  __atomic_store_n(P, V, __ATOMIC_RELAXED);
#elif defined(_MSC_VER)
  __iso_volatile_store64(reinterpret_cast<long long *>(P),
                         static_cast<long long>(V));
#else
#error "unreachable: unsupported toolchain rejected by the guard above"
#endif
}
inline SlotWords loadWords(const void *Addr) noexcept {
  auto &F = stripeFor(Addr);
  const auto *P = reinterpret_cast<const uint64_t *>(Addr);
  lockStripe(F);
  SlotWords R{loadWordRelaxed(&P[0]), loadWordRelaxed(&P[1])};
  unlockStripe(F);
  return R;
}
inline void storeWords(void *Addr, SlotWords W) noexcept {
  auto &F = stripeFor(Addr);
  auto *P = reinterpret_cast<uint64_t *>(Addr);
  lockStripe(F);
  storeWordRelaxed(&P[0], W[0]);
  storeWordRelaxed(&P[1], W[1]);
  unlockStripe(F);
}
#endif

} // namespace detail

//===----------------------------------------------------------------------===//
// Reader path: coherent 128-bit read of BOTH words.
//===----------------------------------------------------------------------===//
inline ValVariant loadCoherent(const ValVariant &Slot) noexcept {
  const detail::SlotWords W = detail::loadWords(&Slot);
  ValVariant Out;
  std::memcpy(static_cast<void *>(&Out), W.data(), sizeof(Out));
  return Out;
}

//===----------------------------------------------------------------------===//
// Writer path: publish BOTH words as one coherent transaction.
//===----------------------------------------------------------------------===//
inline void storeCoherent(ValVariant &Slot, const ValVariant &Val) noexcept {
  detail::SlotWords W;
  std::memcpy(W.data(), static_cast<const void *>(&Val), sizeof(Val));
  detail::storeWords(&Slot, W);
}

//===----------------------------------------------------------------------===//
// RefVariant overloads. A table element is a RefVariant -- 16 bytes, 16-byte
// aligned like ValVariant -- so table get/set/fill/copy of managed refs go
// through the SAME coherent path as struct/array ValVariant fields. The raw
// 16-byte atomic access is identical, so these forward through the ValVariant
// implementation over the shared storage.
//===----------------------------------------------------------------------===//
static_assert(sizeof(RefVariant) == 16 && alignof(RefVariant) >= 16,
              "coherent RefVariant slot assumes a 16-byte, 16-aligned payload");

inline uint8_t *loadPointerWordRelaxed(const RefVariant &Slot) noexcept {
  return loadPointerWordRelaxed(reinterpret_cast<const ValVariant &>(Slot));
}
inline RefVariant loadCoherent(const RefVariant &Slot) noexcept {
  const detail::SlotWords W =
      detail::loadWords(reinterpret_cast<const ValVariant *>(&Slot));
  RefVariant Out;
  std::memcpy(static_cast<void *>(&Out), W.data(), sizeof(Out));
  return Out;
}
inline void storeCoherent(RefVariant &Slot, const RefVariant &Val) noexcept {
  detail::SlotWords W;
  std::memcpy(W.data(), static_cast<const void *>(&Val), sizeof(Val));
  detail::storeWords(reinterpret_cast<ValVariant *>(&Slot), W);
}

} // namespace GC
} // namespace WasmEdge
