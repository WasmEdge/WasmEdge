// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "experimental/simd.hpp"
#include "common/types.h"
#include "experimental/bit.hpp"
#include "experimental/math.hpp"
#include "experimental/simd/ext.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <limits>
#include <type_traits>
#include <vector>

#if defined(_M_X64) || defined(__SSE2__)
#include <emmintrin.h>
#endif

namespace {

namespace simd = cxx26::simd;
using simd::simd_size_type;

template <template <simd_size_type> class Abi> struct Family {};
using ArrayFamily = Family<simd::detail::array_abi>;
#if defined(__GNUC__) || defined(__clang__)
using VectorExtFamily = Family<simd::detail::vector_ext_abi>;
#endif
#if defined(_M_X64) || defined(__SSE2__)
using M128iFamily = Family<simd::detail::m128i_abi>;
#endif

using Families = ::testing::Types<ArrayFamily
#if defined(__GNUC__) || defined(__clang__)
                                  ,
                                  VectorExtFamily
#endif
#if defined(_M_X64) || defined(__SSE2__)
                                  ,
                                  M128iFamily
#endif
                                  >;

template <class F, class T> struct V16Of;
template <template <simd_size_type> class Abi, class T>
struct V16Of<Family<Abi>, T> {
  using type =
      simd::basic_vec<T, Abi<static_cast<simd_size_type>(16 / sizeof(T))>>;
};
template <class F, class T> using V16 = typename V16Of<F, T>::type;

template <class V> std::vector<typename V::value_type> lanes(const V &X) {
  std::vector<typename V::value_type> R;
  for (simd_size_type I = 0; I < V::size; ++I) {
    R.push_back(X[I]);
  }
  return R;
}

struct NarrowingGenerator {
  int operator()(simd_size_type I) const { return I; }
};
struct ExactGenerator {
  std::uint8_t operator()(simd_size_type I) const {
    return static_cast<std::uint8_t>(I);
  }
};

static_assert(!std::is_constructible_v<simd::vec<std::uint8_t, 16>, int>);
static_assert(std::is_convertible_v<std::uint8_t, simd::vec<std::uint8_t, 16>>);
static_assert(std::is_convertible_v<int, simd::vec<std::int32_t, 4>>);
static_assert(!std::is_constructible_v<simd::vec<float, 4>, int>);
static_assert(std::is_convertible_v<simd::vec<std::int16_t, 8>,
                                    simd::vec<std::int32_t, 8>>);
static_assert(!std::is_convertible_v<simd::vec<std::int32_t, 8>,
                                     simd::vec<std::int16_t, 8>>);
static_assert(std::is_constructible_v<simd::vec<std::int16_t, 8>,
                                      simd::vec<std::int32_t, 8>>);
static_assert(
    !std::is_constructible_v<simd::vec<std::uint8_t, 16>, NarrowingGenerator>);
static_assert(
    std::is_constructible_v<simd::vec<std::uint8_t, 16>, ExactGenerator>);

static_assert(simd::basic_mask<1, simd::detail::array_abi<16>>([](auto I) {
                return I == 0;
              }).to_ullong() == 0x1ULL);

static_assert(std::is_same_v<cxx26::simd_ext::native_type_t<simd::basic_vec<
                                 std::uint32_t, simd::detail::array_abi<4>>>,
                             simd::detail::array_storage<std::uint32_t, 4>>);
#if defined(__GNUC__) || defined(__clang__)
using GnuU32x4 [[gnu::vector_size(16)]] = std::uint32_t;
static_assert(
    std::is_same_v<cxx26::simd_ext::native_type_t<simd::basic_vec<
                       std::uint32_t, simd::detail::vector_ext_abi<4>>>,
                   GnuU32x4>);
#endif
#if defined(_M_X64) || defined(__SSE2__)
[[maybe_unused]] constexpr __m128i *M128iNative =
    static_cast<cxx26::simd_ext::native_type_t<
        simd::basic_vec<std::uint32_t, simd::detail::m128i_abi<4>>> *>(nullptr);
#endif

template <class T> std::vector<T> edgeValues() {
  using L = std::numeric_limits<T>;
  std::vector<T> V{T(0), T(1), T(2), T(3), L::max(), L::lowest()};
  if constexpr (std::is_floating_point_v<T>) {
    V.insert(V.end(), {T(-0.0), T(1.5), T(-2.5), L::min(), L::infinity(),
                       -L::infinity(), L::quiet_NaN()});
  } else {
    V.push_back(static_cast<T>(L::max() / 3));
    V.push_back(static_cast<T>(L::max() - 1));
    if constexpr (std::is_signed_v<T>) {
      V.insert(V.end(), {T(-1), T(-2), static_cast<T>(L::lowest() + 1)});
    } else {
      V.push_back(static_cast<T>(L::max() / 2 + 1));
    }
  }
  return V;
}

template <class T> auto bitsOf(T X) {
  using U = std::conditional_t<sizeof(T) == 8, std::uint64_t, std::uint32_t>;
  return cxx20::bit_cast<U>(X);
}

template <class F, class T, class Check> void forEdgePairs(Check &&C) {
  using V = V16<F, T>;
  const auto E = edgeValues<T>();
  for (std::size_t K = 0; K < E.size(); ++K) {
    const V A([&](auto I) { return E[(I * 7 + K) % E.size()]; });
    const V B([&](auto I) { return E[(I * 5 + 3 * K + 1) % E.size()]; });
    C(A, B);
  }
}

template <class F, class T> void checkIntegerOps() {
  using V = V16<F, T>;
  using UT = std::make_unsigned_t<T>;
  using W = std::conditional_t<sizeof(T) == 8, std::uint64_t, std::uint32_t>;
  forEdgePairs<F, T>([](const V &A, const V &B) {
    const V Mul = A * B;
    const V Neg = -A;
    for (simd_size_type I = 0; I < V::size; ++I) {
      const T X = A[I];
      const T Y = B[I];
      const W UX = static_cast<UT>(X);
      const W UY = static_cast<UT>(Y);
      EXPECT_EQ(Mul[I], static_cast<T>(UX * UY));
      EXPECT_EQ(Neg[I], static_cast<T>(W{0} - UX));
      EXPECT_EQ((A == B)[I], X == Y);
      EXPECT_EQ((A != B)[I], X != Y);
      EXPECT_EQ((A < B)[I], X < Y);
      EXPECT_EQ((A <= B)[I], X <= Y);
      EXPECT_EQ((A > B)[I], X > Y);
      EXPECT_EQ((A >= B)[I], X >= Y);
      EXPECT_EQ(V(X)[V::size - 1 - I], X);
    }
    constexpr simd_size_type Bits = sizeof(T) * 8;
    for (simd_size_type S = 0; S < Bits; ++S) {
      const V Shl = A << S;
      const V Shr = A >> S;
      for (simd_size_type I = 0; I < V::size; ++I) {
        const T X = A[I];
        EXPECT_EQ(Shl[I], static_cast<T>(W{static_cast<UT>(X)} << S));
        if constexpr (std::is_signed_v<T>) {
          EXPECT_EQ(Shr[I], static_cast<T>(X >> S));
        } else {
          EXPECT_EQ(Shr[I], static_cast<T>(static_cast<UT>(X) >> S));
        }
      }
    }
  });
}

template <class F, class T> void checkFloatOps() {
  using V = V16<F, T>;
  forEdgePairs<F, T>([](const V &A, const V &B) {
    const V Neg = -A;
    for (simd_size_type I = 0; I < V::size; ++I) {
      const T X = A[I];
      const T Y = B[I];
      EXPECT_EQ(bitsOf(Neg[I]), bitsOf(-X));
      EXPECT_EQ((A == B)[I], X == Y);
      EXPECT_EQ((A != B)[I], X != Y);
      EXPECT_EQ((A < B)[I], X < Y);
      EXPECT_EQ((A <= B)[I], X <= Y);
      EXPECT_EQ((A > B)[I], X > Y);
      EXPECT_EQ((A >= B)[I], X >= Y);
      EXPECT_EQ(bitsOf(V(X)[V::size - 1 - I]), bitsOf(X));
    }
  });
}

template <class F, class To, class From> void checkConversion() {
  using VFrom = V16<F, From>;
  using VTo = V16<F, To>;
  forEdgePairs<F, From>([](const VFrom &A, const VFrom &) {
    const VTo R(A);
    for (simd_size_type I = 0; I < VFrom::size; ++I) {
      EXPECT_EQ(R[I], static_cast<To>(A[I]));
    }
  });
}

template <class T> std::vector<T> roundingValues() {
  using L = std::numeric_limits<T>;
  const T Magic = static_cast<T>(1ULL << (L::digits - 1));
  const T BelowHalf = std::nextafter(T(0.5), T(0));
  std::vector<T> V{T(0),
                   T(-0.0),
                   T(0.5),
                   T(-0.5),
                   T(1.5),
                   T(-1.5),
                   T(2.5),
                   T(-2.5),
                   T(0.75),
                   T(-0.25),
                   BelowHalf,
                   -BelowHalf,
                   T(6.25),
                   L::denorm_min(),
                   -L::denorm_min(),
                   L::min(),
                   L::max(),
                   L::lowest(),
                   L::infinity(),
                   -L::infinity(),
                   L::quiet_NaN(),
                   -L::quiet_NaN(),
                   L::signaling_NaN()};
  for (const T B : {Magic, -Magic, Magic / 2, -Magic / 2}) {
    V.insert(V.end(), {B - T(1), B - T(0.5), B, B + T(0.5), B + T(1),
                       std::nextafter(B, T(0))});
  }
  return V;
}

template <class T> bool sameFloat(T Got, T Want, bool RequireQuiet) {
  if (std::isnan(Want)) {
    const auto Quiet = decltype(bitsOf(Got)){1}
                       << (std::numeric_limits<T>::digits - 2);
    return std::isnan(Got) && (!RequireQuiet || (bitsOf(Got) & Quiet) != 0);
  }
  return bitsOf(Got) == bitsOf(Want);
}

template <class F, class T> void checkRounding() {
  using V = V16<F, T>;
  constexpr bool Quiet =
#if defined(_M_X64) || defined(__SSE2__)
      std::is_same_v<F, M128iFamily> ||
#endif
#if defined(__GNUC__) || defined(__clang__)
      (std::is_same_v<F, VectorExtFamily> &&
       simd::detail::vector_ext_intrinsics<T, V::size>::has_math) ||
#endif
      false;
  const auto In = roundingValues<T>();
  for (std::size_t K = 0; K < In.size(); K += V::size) {
    const V X([&](auto I) { return In[(K + I) % In.size()]; });
    const V Sqrt = simd::sqrt(X);
    const V Ceil = simd::ceil(X);
    const V Floor = simd::floor(X);
    const V Trunc = simd::trunc(X);
    const V RoundEven = cxx26::simd_ext::roundeven(X);
    for (simd_size_type I = 0; I < V::size; ++I) {
      const T Y = X[I];
      EXPECT_TRUE(sameFloat(Sqrt[I], std::sqrt(Y), Quiet)) << Y;
      EXPECT_TRUE(sameFloat(Ceil[I], std::ceil(Y), Quiet)) << Y;
      EXPECT_TRUE(sameFloat(Floor[I], std::floor(Y), Quiet)) << Y;
      EXPECT_TRUE(sameFloat(Trunc[I], std::trunc(Y), Quiet)) << Y;
      EXPECT_TRUE(sameFloat(RoundEven[I], cxx26::roundeven(Y), Quiet)) << Y;
    }
  }
}

template <class F, class T> void checkMaskBits() {
  using V = V16<F, T>;
  constexpr unsigned long long All = (1ULL << V::size) - 1;
  for (const unsigned long long Pattern :
       {0x0ULL, 0xffffULL, 0x9249ULL, 0x5aa5ULL, 0x8001ULL}) {
    const unsigned long long Want = Pattern & All;
    const V X([&](auto I) { return static_cast<T>((Want >> I) & 1 ? -1 : 1); });
    const auto M = X < V();
    EXPECT_EQ(M.to_ullong(), Want);
    EXPECT_EQ(simd::all_of(M), Want == All);
    EXPECT_EQ(simd::any_of(M), Want != 0);
  }
}

template <class F> class SimdTest : public ::testing::Test {};
TYPED_TEST_SUITE(SimdTest, Families);

TYPED_TEST(SimdTest, Layout) {
  using U8 = V16<TypeParam, std::uint8_t>;
  using F64 = V16<TypeParam, double>;
  EXPECT_EQ(sizeof(U8), 16U);
  EXPECT_EQ(alignof(U8), 16U);
  EXPECT_EQ(simd::alignment_v<U8>, 16U);
  EXPECT_EQ(simd::alignment_v<F64>, 16U);
  EXPECT_TRUE(std::is_trivially_copyable_v<U8>);
  EXPECT_TRUE(std::is_trivially_default_constructible_v<F64>);
}

TYPED_TEST(SimdTest, BroadcastKeepsNegativeZero) {
  const V16<TypeParam, float> F(-0.0f);
  const V16<TypeParam, double> D(-0.0);
  for (simd_size_type I = 0; I < 4; ++I) {
    EXPECT_TRUE(std::signbit(F[I]));
  }
  for (simd_size_type I = 0; I < 2; ++I) {
    EXPECT_TRUE(std::signbit(D[I]));
  }
}

TYPED_TEST(SimdTest, GeneratorAndArithmeticWrap) {
  using U8 = V16<TypeParam, std::uint8_t>;
  const U8 X([](auto I) { return static_cast<std::uint8_t>(250 + I); });
  const U8 Y = X + U8(std::uint8_t{10});
  EXPECT_EQ(Y[0], 4U);
  EXPECT_EQ(Y[5], 9U);
  EXPECT_EQ(Y[15], 19U);
  const U8 Z = (X >> 4) ^ (X << 1);
  for (simd_size_type I = 0; I < 16; ++I) {
    const auto Lane = static_cast<std::uint8_t>(250 + I);
    EXPECT_EQ(Z[I], static_cast<std::uint8_t>((Lane >> 4) ^ (Lane << 1)));
  }
  using I16 = V16<TypeParam, std::int16_t>;
  const I16 S(std::int16_t{-32768});
  EXPECT_EQ((-S)[3], -32768);
  EXPECT_EQ((S >> 15)[0], -1);
}

TYPED_TEST(SimdTest, ComparisonsProduceMasks) {
  using I32 = V16<TypeParam, std::int32_t>;
  const I32 A([](auto I) { return static_cast<std::int32_t>(I); });
  const I32 B(std::int32_t{1});
  EXPECT_EQ(lanes(A == B), (std::vector<bool>{false, true, false, false}));
  EXPECT_EQ(lanes(A < B), (std::vector<bool>{true, false, false, false}));
  EXPECT_EQ(lanes(A >= B), (std::vector<bool>{false, true, true, true}));
  EXPECT_EQ(lanes(-(A != B)), (std::vector<std::int32_t>{-1, 0, -1, -1}));
  EXPECT_EQ(lanes(+(A != B)), (std::vector<std::int32_t>{1, 0, 1, 1}));
  EXPECT_EQ(lanes(~(A != B)), (std::vector<std::int32_t>{-2, -1, -2, -2}));
  EXPECT_EQ(lanes(!(A == B)), (std::vector<bool>{true, false, true, true}));
}

TYPED_TEST(SimdTest, NaNComparesUnequal) {
  using F32 = V16<TypeParam, float>;
  const float NaN = std::numeric_limits<float>::quiet_NaN();
  const F32 X([&](auto I) { return I == 2 ? NaN : 1.0f; });
  EXPECT_EQ(lanes(X == X), (std::vector<bool>{true, true, false, true}));
  EXPECT_EQ(lanes(X != X), (std::vector<bool>{false, false, true, false}));
}

TYPED_TEST(SimdTest, Select) {
  using F32 = V16<TypeParam, float>;
  using I32 = V16<TypeParam, std::int32_t>;
  const F32 A(1.5f);
  const F32 B(-2.5f);
  const I32 Idx([](auto I) { return static_cast<std::int32_t>(I); });
  EXPECT_EQ(lanes(simd::select(Idx > I32(1), A, B)),
            (std::vector<float>{-2.5f, -2.5f, 1.5f, 1.5f}));
}

TYPED_TEST(SimdTest, Conversions) {
  using I16 = V16<TypeParam, std::int16_t>;
  using I32 = V16<TypeParam, std::int32_t>;
  using F32 = V16<TypeParam, float>;
  using H8 = simd::vec<std::int8_t, 8>;
  const H8 Small([](auto I) { return static_cast<std::int8_t>(I * 30 - 100); });
  const I16 Wide = Small;
  EXPECT_EQ(Wide[0], -100);
  EXPECT_EQ(Wide[7], 110);
  const H8 Narrow(Wide * I16(std::int16_t{3}));
  EXPECT_EQ(Narrow[0], static_cast<std::int8_t>(-300));
  EXPECT_EQ(Narrow[7], static_cast<std::int8_t>(330));
  const auto F =
      F32(I32([](auto I) { return static_cast<std::int32_t>(I) - 1; }));
  EXPECT_EQ(lanes(F), (std::vector<float>{-1.0f, 0.0f, 1.0f, 2.0f}));
  const I32 T(F32(2.75f));
  EXPECT_EQ(T[1], 2);
}

TYPED_TEST(SimdTest, ChunkAndCat) {
  using U8 = V16<TypeParam, std::uint8_t>;
  using H = simd::vec<std::uint8_t, 8>;
  const U8 X([](auto I) { return static_cast<std::uint8_t>(I); });
  const auto Halves = simd::chunk<H>(X);
  EXPECT_EQ(Halves[0][0], 0U);
  EXPECT_EQ(Halves[0][7], 7U);
  EXPECT_EQ(Halves[1][0], 8U);
  EXPECT_EQ(Halves[1][7], 15U);
  const auto Joined = simd::cat(Halves[1], Halves[0]);
  EXPECT_EQ(Joined[0], 8U);
  EXPECT_EQ(Joined[15], 7U);
}

TYPED_TEST(SimdTest, Permute) {
  using U8 = V16<TypeParam, std::uint8_t>;
  const U8 X([](auto I) { return static_cast<std::uint8_t>(I * 3); });
  const U8 Reversed = simd::permute(X, [](auto I) { return 15 - I; });
  EXPECT_EQ(Reversed[0], 45U);
  EXPECT_EQ(Reversed[15], 0U);
  const U8 Idx([](auto I) { return static_cast<std::uint8_t>((I * 5) % 16); });
  const U8 Shuffled = simd::permute(X, Idx);
  for (simd_size_type I = 0; I < 16; ++I) {
    EXPECT_EQ(Shuffled[I], static_cast<std::uint8_t>(((I * 5) % 16) * 3));
  }
  const U8 Wrapped = simd::permute(X, Idx + U8(std::uint8_t{16}));
  EXPECT_EQ(lanes(Wrapped), lanes(Shuffled));
}

TYPED_TEST(SimdTest, MaskReductions) {
  using I8 = V16<TypeParam, std::int8_t>;
  using I64 = V16<TypeParam, std::int64_t>;
  const I8 X(
      [](auto I) { return static_cast<std::int8_t>(I % 3 == 0 ? -1 : 1); });
  EXPECT_EQ((X < I8()).to_ullong(), 0x9249ULL);
  EXPECT_TRUE(simd::any_of(X < I8()));
  EXPECT_FALSE(simd::all_of(X < I8()));
  EXPECT_TRUE(simd::all_of(X != I8()));
  EXPECT_TRUE(simd::none_of(X == I8()));
  const I64 Y([](auto I) { return static_cast<std::int64_t>(I) - 1; });
  EXPECT_EQ((Y < I64()).to_ullong(), 0x1ULL);
}

TYPED_TEST(SimdTest, MathFunctions) {
  using F64 = V16<TypeParam, double>;
  const F64 X([](auto I) { return I == 0 ? -2.5 : 6.25; });
  EXPECT_EQ(lanes(simd::sqrt(F64(6.25))), (std::vector<double>{2.5, 2.5}));
  EXPECT_EQ(lanes(simd::ceil(X)), (std::vector<double>{-2.0, 7.0}));
  EXPECT_EQ(lanes(simd::floor(X)), (std::vector<double>{-3.0, 6.0}));
  EXPECT_EQ(lanes(simd::trunc(X)), (std::vector<double>{-2.0, 6.0}));
}

TYPED_TEST(SimdTest, RoundingMatchesScalar) {
  checkRounding<TypeParam, float>();
  checkRounding<TypeParam, double>();
}

TYPED_TEST(SimdTest, MaskBitsMatchLanes) {
  checkMaskBits<TypeParam, std::int8_t>();
  checkMaskBits<TypeParam, std::int16_t>();
  checkMaskBits<TypeParam, std::int32_t>();
  checkMaskBits<TypeParam, std::int64_t>();
}

TYPED_TEST(SimdTest, LaneExtensions) {
  using U16 = V16<TypeParam, std::uint16_t>;
  U16 X(std::uint16_t{7});
  cxx26::simd_ext::replace_lane(X, 3, std::uint16_t{0xbeef});
  const std::uint16_t Source = 0x1234;
  cxx26::simd_ext::load_lane(X, 6, &Source);
  EXPECT_EQ(lanes(X),
            (std::vector<std::uint16_t>{7, 7, 7, 0xbeef, 7, 7, 0x1234, 7}));
}

TYPED_TEST(SimdTest, IntegerOpsMatchScalar) {
  checkIntegerOps<TypeParam, std::int8_t>();
  checkIntegerOps<TypeParam, std::uint8_t>();
  checkIntegerOps<TypeParam, std::int16_t>();
  checkIntegerOps<TypeParam, std::uint16_t>();
  checkIntegerOps<TypeParam, std::int32_t>();
  checkIntegerOps<TypeParam, std::uint32_t>();
  checkIntegerOps<TypeParam, std::int64_t>();
  checkIntegerOps<TypeParam, std::uint64_t>();
}

TYPED_TEST(SimdTest, FloatOpsMatchScalar) {
  checkFloatOps<TypeParam, float>();
  checkFloatOps<TypeParam, double>();
}

TYPED_TEST(SimdTest, ConversionsMatchScalar) {
  checkConversion<TypeParam, float, std::int32_t>();
  checkConversion<TypeParam, std::uint32_t, std::int32_t>();
  checkConversion<TypeParam, std::int32_t, std::uint32_t>();
  checkConversion<TypeParam, std::uint8_t, std::int8_t>();
  checkConversion<TypeParam, std::int64_t, std::uint64_t>();
  using VF = V16<TypeParam, float>;
  using VI = V16<TypeParam, std::int32_t>;
  const VF X([](auto I) {
    constexpr float In[] = {-2.5f, 1.5f, -0.0f, 2147483520.0f};
    return In[I];
  });
  const VI R(X);
  EXPECT_EQ(lanes(R), (std::vector<std::int32_t>{-2, 1, 0, 2147483520}));
}

TYPED_TEST(SimdTest, NativeRoundTrip) {
  using U32 = V16<TypeParam, std::uint32_t>;
  const U32 X([](auto I) { return static_cast<std::uint32_t>(I + 1) * 0x11U; });
  const auto Native = cxx26::simd_ext::to_native(X);
  EXPECT_EQ(sizeof(Native), 16U);
  EXPECT_EQ(alignof(U32), 16U);
  std::uint32_t Raw[4];
  std::memcpy(Raw, &Native, sizeof(Raw));
  EXPECT_EQ((std::vector<std::uint32_t>(Raw, Raw + 4)),
            (std::vector<std::uint32_t>{0x11, 0x22, 0x33, 0x44}));
  EXPECT_EQ(lanes(cxx26::simd_ext::from_native<U32>(Native)), lanes(X));
}

TEST(ValVariantTest, ScalarsDoNotBroadcastIntoVectorAlternatives) {
  const WasmEdge::ValVariant U8(std::uint8_t{200});
  EXPECT_EQ(U8.get<std::int32_t>(), 200);
  const WasmEdge::ValVariant I8(std::int8_t{-3});
  EXPECT_EQ(I8.get<std::int32_t>(), -3);
  const WasmEdge::ValVariant U16(std::uint16_t{0xbeef});
  EXPECT_EQ(U16.get<std::int32_t>(), 0xbeef);
  const WasmEdge::ValVariant F(1.5f);
  EXPECT_EQ(F.get<float>(), 1.5f);
  const WasmEdge::ValVariant U64(std::uint64_t{1} << 40);
  EXPECT_EQ(U64.get<std::uint64_t>(), std::uint64_t{1} << 40);
}

} // namespace
