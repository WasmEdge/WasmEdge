// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "experimental/simd.hpp"
#include "common/nan.h"
#include "common/span.h"
#include "common/types.h"
#include "experimental/bit.hpp"
#include "experimental/math.hpp"
#include "experimental/simd/ext.hpp"

#include <array>
#include <atomic>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <gtest/gtest.h>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

#if defined(_M_X64) || defined(__SSE2__)
#include <immintrin.h>
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
static_assert(!std::is_convertible_v<std::reference_wrapper<std::uint8_t>,
                                     simd::vec<std::uint8_t, 16>>);
static_assert(std::is_constructible_v<simd::vec<std::uint8_t, 16>,
                                      std::reference_wrapper<std::uint8_t>>);
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
  return cxx20::bit_cast<WasmEdge::FloatBits<T>>(X);
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

template <class V>
void expectComparesMatch(const V &A, const V &B, simd_size_type I) {
  const auto X = A[I];
  const auto Y = B[I];
  EXPECT_EQ((A == B)[I], X == Y);
  EXPECT_EQ((A != B)[I], X != Y);
  EXPECT_EQ((A < B)[I], X < Y);
  EXPECT_EQ((A <= B)[I], X <= Y);
  EXPECT_EQ((A > B)[I], X > Y);
  EXPECT_EQ((A >= B)[I], X >= Y);
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
      expectComparesMatch(A, B, I);
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
      EXPECT_EQ(bitsOf(Neg[I]), bitsOf(-X));
      expectComparesMatch(A, B, I);
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

template <class T> WasmEdge::FloatBits<T> signalingNaNBits() {
  return static_cast<WasmEdge::FloatBits<T>>(
      bitsOf(std::numeric_limits<T>::infinity()) | 1U);
}

// The values end with a signaling NaN.
template <class T> std::vector<T> roundingValues() {
  using L = std::numeric_limits<T>;
  const T Magic = static_cast<T>(1ULL << (L::digits - 1));
  const T BelowHalf = std::nextafter(T(0.5), T(0));
  std::vector<T> V{
      T(0),           T(-0.0),         T(0.5),           T(-0.5),
      T(1.5),         T(-1.5),         T(2.5),           T(-2.5),
      T(0.75),        T(-0.25),        BelowHalf,        -BelowHalf,
      T(6.25),        L::denorm_min(), -L::denorm_min(), L::min(),
      L::max(),       L::lowest(),     L::infinity(),    -L::infinity(),
      L::quiet_NaN(), -L::quiet_NaN(), T(0.001),         T(-0.001),
      T(1e-30),       T(1e30)};
  for (const T B : {Magic, -Magic, Magic / 2, -Magic / 2}) {
    V.insert(V.end(), {B - T(1), B - T(0.5), B, B + T(0.5), B + T(1),
                       std::nextafter(B, T(0))});
  }
  // A copy through an x87 register quiets a signaling NaN, so the last value
  // is written from its bits after the vector stops growing.
  const auto Signaling = signalingNaNBits<T>();
  V.emplace_back();
  std::memcpy(&V.back(), &Signaling, sizeof(T));
  return V;
}

// Tells whether the last value still holds the signaling NaN, read as bytes.
template <class T> bool endsWithSignalingNaN(const std::vector<T> &Values) {
  WasmEdge::FloatBits<T> Bits;
  std::memcpy(&Bits, &Values.back(), sizeof(T));
  return Bits == signalingNaNBits<T>();
}

template <class T> bool sameFloat(T Got, T Want) {
  if (std::isnan(Want)) {
    return std::isnan(Got) && (bitsOf(Got) & WasmEdge::QuietBit<T>) != 0;
  }
  return bitsOf(Got) == bitsOf(Want);
}

template <class F, class T> using Lanes = std::array<T, V16<F, T>::size>;
template <class F, class T> using VectorFn = V16<F, T> (*)(const V16<F, T> &);

// An operation on vectors and the scalar function that it must match in each
// lane.
template <class F, class T> struct LaneOp {
  const char *Name;
  VectorFn<F, T> Vector;
  T (*Scalar)(T);
};

template <class F, class T> constexpr LaneOp<F, T> sqrtOp() {
  using V = V16<F, T>;
  return {"sqrt", [](const V &X) -> V { return simd::sqrt(X); },
          [](T Y) -> T { return std::sqrt(Y); }};
}

template <class F, class T> constexpr std::array<LaneOp<F, T>, 4> roundOps() {
  using V = V16<F, T>;
  return {{
      {"ceil", [](const V &X) -> V { return simd::ceil(X); },
       [](T Y) -> T { return std::ceil(Y); }},
      {"floor", [](const V &X) -> V { return simd::floor(X); },
       [](T Y) -> T { return std::floor(Y); }},
      {"trunc", [](const V &X) -> V { return simd::trunc(X); },
       [](T Y) -> T { return std::trunc(Y); }},
      {"roundeven",
       [](const V &X) -> V { return cxx26::simd_ext::roundeven(X); },
       [](T Y) -> T { return cxx26::roundeven(Y); }},
  }};
}

// Fills the lanes with Count values from Values + K on, and wraps to the
// front. It copies bytes, so a signaling NaN stays signaling.
template <class F, class T>
Lanes<F, T> loadLanes(const T *Values, std::size_t Count, std::size_t K) {
  Lanes<F, T> In{};
  for (std::size_t I = 0; I < In.size(); ++I) {
    std::memcpy(&In[I], &Values[(K + I) % Count], sizeof(T));
  }
  return In;
}

template <class F, class T, std::size_t M>
Lanes<F, T> loadLanes(const std::array<T, M> &Values) {
  return loadLanes<F, T>(Values.data(), M, 0);
}

// Converts between lanes and a vector without a floating-point instruction.
template <class F, class T> V16<F, T> toVector(const Lanes<F, T> &In) {
  return cxx20::bit_cast<V16<F, T>>(In);
}
template <class F, class T> Lanes<F, T> toLanes(const V16<F, T> &X) {
  return cxx20::bit_cast<Lanes<F, T>>(X);
}

// Applies Fn to X and writes the result to Y.
template <class F, class T>
void applyVector(VectorFn<F, T> Fn, const V16<F, T> &X, V16<F, T> &Y) {
  Y = Fn(X);
}

template <class F, class T>
void expectLanes(const LaneOp<F, T> &Op, const Lanes<F, T> &In,
                 const Lanes<F, T> &Out) {
  for (std::size_t I = 0; I < In.size(); ++I) {
    const T Want = Op.Scalar(In[I]);
    EXPECT_TRUE(sameFloat(Out[I], Want))
        << std::setprecision(std::numeric_limits<T>::max_digits10) << Op.Name
        << '(' << In[I] << ") in lane " << I << " is " << Out[I] << ", not "
        << Want;
  }
}

// Fills a vector with each batch of Values in turn and passes the lanes and
// the vector to Check.
template <class F, class T, class C>
void forEachBatch(WasmEdge::Span<const T> Values, C Check) {
  for (std::size_t K = 0; K < Values.size(); K += V16<F, T>::size) {
    const auto In = loadLanes<F, T>(Values.data(), Values.size(), K);
    Check(In, toVector<F, T>(In));
    if (::testing::Test::HasFatalFailure()) {
      return;
    }
  }
}

// Lets Run apply Op to each batch of Values, and compares each lane of the
// result with the scalar reference. Run can change the floating-point
// environment around the call. The vector is ready before Run starts, and Run
// ends before the test reads the lanes.
template <class F, class T, class R>
void checkBatches(const LaneOp<F, T> &Op, WasmEdge::Span<const T> Values,
                  R Run) {
  forEachBatch<F, T>(Values,
                     [&Op, &Run](const Lanes<F, T> &In, const V16<F, T> &X) {
                       V16<F, T> Y{};
                       Run(Op, X, Y);
                       if (::testing::Test::HasFatalFailure()) {
                         return;
                       }
                       expectLanes(Op, In, toLanes<F, T>(Y));
                     });
}

template <class F, class T> void checkRounding() {
  using V = V16<F, T>;
  const auto Values = roundingValues<T>();
  ASSERT_TRUE(endsWithSignalingNaN(Values));
  const auto Apply = [](const LaneOp<F, T> &Op, const V &X, V &Y) {
    applyVector<F, T>(Op.Vector, X, Y);
  };
  checkBatches<F, T>(sqrtOp<F, T>(), Values, Apply);
  for (const auto &Op : roundOps<F, T>()) {
    checkBatches<F, T>(Op, Values, Apply);
  }
}

#if defined(_M_X64) || defined(__SSE2__)
template <class F> struct BackendOf;
template <template <simd_size_type> class Abi> struct BackendOf<Family<Abi>> {
  using type = simd::detail::simd_backend<float, Abi<4>>;
};

// The x86 rounding without SSE4.1 must not depend on the MXCSR rounding mode
// or on denormals-are-zero, and must not raise a floating-point exception. The
// m128i backend reports that it uses it, and the vector_ext backend reports it
// through its SSE2 intrinsics. The members are read by name, so a backend that
// loses one fails to compile instead of skipping the checks.
template <class F> struct IntegerRounding : std::false_type {};
template <>
struct IntegerRounding<M128iFamily>
    : std::bool_constant<BackendOf<M128iFamily>::type::integer_rounding> {};
#if defined(__GNUC__) || defined(__clang__)
template <>
struct IntegerRounding<VectorExtFamily>
    : std::bool_constant<
          BackendOf<VectorExtFamily>::type::intrinsics::integer_rounding> {};
#endif

template <class F> constexpr bool usesIntegerRounding() {
  return IntegerRounding<F>::value;
}

// The build defines WASMEDGE_SIMD_TEST_STRICT_FP when it compiles this file
// with the strict floating-point option of the compiler.
#if defined(WASMEDGE_SIMD_TEST_STRICT_FP)
constexpr bool StrictFloatingPoint = true;
#else
constexpr bool StrictFloatingPoint = false;
#endif

constexpr unsigned int MxcsrExceptionFlags = 0x003FU;
constexpr unsigned int MxcsrInexact = 0x0020U;
constexpr unsigned int MxcsrDenormalsAreZero = 0x0040U;
constexpr unsigned int MxcsrFlushToZero = 0x8000U;
// Round to nearest, all exceptions masked, no flag set, DAZ and FTZ off.
constexpr unsigned int MxcsrDefault = 0x1F80U;
// The writable MXCSR bits of a CPU that stores a zero mask with FXSAVE.
constexpr std::uint32_t MxcsrDefaultMask = 0xFFBFU;

// Early SSE2 CPUs have no denormals-are-zero bit, and a write to it faults.
// FXSAVE stores the mask of the writable MXCSR bits at byte 28.
#if defined(__GNUC__) || defined(__clang__)
__attribute__((target("fxsr")))
#endif
bool hasDenormalsAreZero() noexcept {
  alignas(16) unsigned char Area[512] = {};
  _fxsave(Area);
  std::uint32_t Mask;
  std::memcpy(&Mask, Area + 28, sizeof(Mask));
  return ((Mask != 0 ? Mask : MxcsrDefaultMask) & MxcsrDenormalsAreZero) != 0;
}

// Starts from the default floating-point environment and restores the
// original rounding mode, exception flags and MXCSR on scope exit, also when a
// fatal assertion returns early.
class FloatEnvGuard {
public:
  FloatEnvGuard() noexcept : Csr(_mm_getcsr()) {
    Saved = std::fegetenv(&Env) == 0;
    Ready = Saved && std::fesetenv(FE_DFL_ENV) == 0;
    _mm_setcsr(MxcsrDefault);
  }
  ~FloatEnvGuard() noexcept {
    if (Saved && std::fesetenv(&Env) != 0) {
      ADD_FAILURE() << "cannot restore the floating-point environment";
    }
    _mm_setcsr(Csr);
  }
  FloatEnvGuard(const FloatEnvGuard &) = delete;
  FloatEnvGuard &operator=(const FloatEnvGuard &) = delete;

  // Tells whether the original environment is saved and the default one is
  // set.
  bool ready() const noexcept { return Ready; }

private:
  std::fenv_t Env;
  unsigned int Csr;
  bool Saved = false;
  bool Ready = false;
};

// The compiler treats floating-point operations as free of side effects and
// can move them past a change to the floating-point environment. The strict
// floating-point option keeps that order. A call through a volatile function
// pointer is a second boundary: GCC and Clang do not inline it, and its loads
// and stores keep the calculation between the calls around it. C++ does not
// guarantee this, so each test first runs a control through the same call,
// which fails if the environment change does not reach the calculation.
template <class F, class T>
void applyOpaque(VectorFn<F, T> Fn, const V16<F, T> &X, V16<F, T> &Y) {
  void (*volatile Apply)(VectorFn<F, T>, const V16<F, T> &, V16<F, T> &) =
      &applyVector<F, T>;
  Apply(Fn, X, Y);
}

template <class F, class T>
Lanes<F, T> applyOpaque(VectorFn<F, T> Fn, const Lanes<F, T> &In) {
  const auto X = toVector<F, T>(In);
  V16<F, T> Y{};
  applyOpaque<F, T>(Fn, X, Y);
  return toLanes<F, T>(Y);
}

// Adds epsilon / 4 for FE_UPWARD and subtracts it for the other modes. The
// step is too small to change 1 or -1 in the rounding to nearest.
template <class F, class T> VectorFn<F, T> modeControl(int Mode) {
  using V = V16<F, T>;
  if (Mode == FE_UPWARD) {
    return [](const V &X) -> V {
      return X + V(std::numeric_limits<T>::epsilon() / 4);
    };
  }
  return [](const V &X) -> V {
    return X - V(std::numeric_limits<T>::epsilon() / 4);
  };
}

// The results of modeControl for 1 and -1 in each mode. FE_DOWNWARD moves -1
// away from zero, and FE_TOWARDZERO keeps it.
template <class T> std::array<T, 2> modeControlResults(int Mode) {
  switch (Mode) {
  case FE_UPWARD:
    return {std::nextafter(T(1), T(2)), std::nextafter(T(-1), T(0))};
  case FE_DOWNWARD:
    return {std::nextafter(T(1), T(0)), std::nextafter(T(-1), T(-2))};
  default:
    return {std::nextafter(T(1), T(0)), T(-1)};
  }
}

template <class F, class T> void checkRoundingIgnoresRoundingMode() {
  using V = V16<F, T>;
  const FloatEnvGuard Guard;
  ASSERT_TRUE(Guard.ready());
  const auto Values = roundingValues<T>();
  ASSERT_TRUE(endsWithSignalingNaN(Values));
  const auto In = loadLanes<F, T>(std::array<T, 2>{T(1), T(-1)});
  for (const int Mode : {FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}) {
    SCOPED_TRACE(Mode);
    ASSERT_EQ(std::fesetround(Mode), 0);
    const auto Moved = applyOpaque<F, T>(modeControl<F, T>(Mode), In);
    ASSERT_EQ(std::fesetround(FE_TONEAREST), 0);
    const auto Want = modeControlResults<T>(Mode);
    ASSERT_EQ(Moved[0], Want[0])
        << "the control does not show the rounding mode";
    ASSERT_EQ(Moved[1], Want[1])
        << "the control does not show the rounding mode";
    for (const auto &Op : roundOps<F, T>()) {
      checkBatches<F, T>(Op, Values,
                         [Mode](const LaneOp<F, T> &O, const V &X, V &Y) {
                           ASSERT_EQ(std::fesetround(Mode), 0);
                           applyOpaque<F, T>(O.Vector, X, Y);
                           ASSERT_EQ(std::fesetround(FE_TONEAREST), 0);
                         });
    }
  }
}

// Sets or clears denormals-are-zero, which reads a denormal input as zero, and
// flush-to-zero, which writes a denormal result as zero. Fast-math startup
// code sets both.
void setDenormalsAreZero(bool On) noexcept {
  const unsigned int Bits = MxcsrDenormalsAreZero | MxcsrFlushToZero;
  _mm_setcsr(On ? (_mm_getcsr() | Bits) : (_mm_getcsr() & ~Bits));
}

// The control multiplies a denormal by the largest value, a product that is
// normal. It must be nonzero with denormals-are-zero off, which shows that the
// input is a denormal, and zero with it on.
template <class F, class T> void checkRoundingIgnoresDenormalsAreZero() {
  using V = V16<F, T>;
  const FloatEnvGuard Guard;
  ASSERT_TRUE(Guard.ready());
  const T Tiny = std::numeric_limits<T>::denorm_min();
  const std::array<T, 2> Denormals{Tiny, -Tiny};
  const auto In = loadLanes<F, T>(Denormals);
  const VectorFn<F, T> Control = [](const V &X) -> V {
    return X * V(std::numeric_limits<T>::max());
  };
  const auto Kept = applyOpaque<F, T>(Control, In);
  ASSERT_NE(Kept[0], T(0)) << "the input is not a denormal";
  ASSERT_NE(Kept[1], T(0)) << "the input is not a denormal";
  setDenormalsAreZero(true);
  const auto Zeroed = applyOpaque<F, T>(Control, In);
  setDenormalsAreZero(false);
  ASSERT_EQ(Zeroed[0], T(0)) << "denormals-are-zero had no effect";
  ASSERT_EQ(Zeroed[1], T(0)) << "denormals-are-zero had no effect";
  for (const auto &Op : roundOps<F, T>()) {
    checkBatches<F, T>(Op, Denormals,
                       [](const LaneOp<F, T> &O, const V &X, V &Y) {
                         setDenormalsAreZero(true);
                         applyOpaque<F, T>(O.Vector, X, Y);
                         setDenormalsAreZero(false);
                       });
  }
}

// Clears the standard exception flags and the MXCSR flags. MXCSR also has a
// denormal-operand flag, which FE_ALL_EXCEPT does not include.
void clearExceptionFlags() noexcept {
  std::feclearexcept(FE_ALL_EXCEPT);
  _mm_setcsr(_mm_getcsr() & ~MxcsrExceptionFlags);
}

// Clears the flags after the input vector is ready and reads them right after
// the rounding, so only the rounding can set them. RoundingMatchesScalar
// checks the results.
template <class F, class T> void checkRoundingRaisesNoException() {
  using V = V16<F, T>;
  const FloatEnvGuard Guard;
  ASSERT_TRUE(Guard.ready());
  const auto One = toVector<F, T>(loadLanes<F, T>(std::array<T, 1>{T(1)}));
  V Sum{};
  clearExceptionFlags();
  applyOpaque<F, T>(modeControl<F, T>(FE_UPWARD), One, Sum);
  ASSERT_NE(std::fetestexcept(FE_INEXACT), 0) << "no flag was detected";
  ASSERT_NE(_mm_getcsr() & MxcsrInexact, 0U) << "no MXCSR flag was detected";
  const auto Values = roundingValues<T>();
  ASSERT_TRUE(endsWithSignalingNaN(Values));
  for (const auto &Op : roundOps<F, T>()) {
    forEachBatch<F, T>(Values, [&Op](const Lanes<F, T> &In, const V &X) {
      V Y{};
      clearExceptionFlags();
      applyOpaque<F, T>(Op.Vector, X, Y);
      const int Raised = std::fetestexcept(FE_ALL_EXCEPT);
      const unsigned int Flags = _mm_getcsr() & MxcsrExceptionFlags;
      EXPECT_EQ(Raised, 0) << Op.Name << " of " << ::testing::PrintToString(In)
                           << " raised the exceptions 0x" << std::hex << Raised;
      EXPECT_EQ(Flags, 0U) << Op.Name << " of " << ::testing::PrintToString(In)
                           << " set the MXCSR flags 0x" << std::hex << Flags;
    });
  }
}
#endif

// WasmEdge stores lane I of a 16-byte value at bytes [I * sizeof(T),
// (I + 1) * sizeof(T)), as an array of T. Each byte of the pattern is distinct,
// so a swap of lanes or of bytes in a lane changes the result.
template <class F, class T> void checkRawLanes() {
  using V = V16<F, T>;
  std::array<T, V::size> Want{};
  for (std::size_t I = 0; I < Want.size(); ++I) {
    std::uint64_t Lane = 0;
    for (std::size_t B = 0; B < sizeof(T); ++B) {
      Lane |= std::uint64_t(0x10U + I * sizeof(T) + B) << (8 * B);
    }
    Want[I] = static_cast<T>(Lane);
  }
  const V X([&](auto I) { return Want[I]; });
  EXPECT_EQ((cxx20::bit_cast<std::array<T, V::size>>(X)), Want) << sizeof(T);
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

template <class V, class = void> struct HasComplement : std::false_type {};
template <class V>
struct HasComplement<V, std::void_t<decltype(~std::declval<V>())>>
    : std::true_type {};
template <class V, class = void> struct HasBitAnd : std::false_type {};
template <class V>
struct HasBitAnd<V,
                 std::void_t<decltype(std::declval<V>() & std::declval<V>())>>
    : std::true_type {};
template <class V, class = void> struct HasShift : std::false_type {};
template <class V>
struct HasShift<V, std::void_t<decltype(std::declval<V>() << 1)>>
    : std::true_type {};

struct OnlyIndexZero {
  int operator()(std::integral_constant<simd_size_type, 0>) const;
};

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
  checkRawLanes<TypeParam, std::uint8_t>();
  checkRawLanes<TypeParam, std::uint16_t>();
  checkRawLanes<TypeParam, std::uint32_t>();
  checkRawLanes<TypeParam, std::uint64_t>();
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

TYPED_TEST(SimdTest, IntegerOperatorsNeedIntegralLanes) {
  using I32 = V16<TypeParam, std::int32_t>;
  using F32 = V16<TypeParam, float>;
  static_assert(HasComplement<I32>::value && HasBitAnd<I32>::value &&
                HasShift<I32>::value);
  static_assert(!HasComplement<F32>::value && !HasBitAnd<F32>::value &&
                !HasShift<F32>::value);
}

TYPED_TEST(SimdTest, GeneratorMustAcceptEveryIndex) {
  static_assert(
      !std::is_constructible_v<V16<TypeParam, std::int32_t>, OnlyIndexZero>);
}

TYPED_TEST(SimdTest, GeneratorExceptionPropagates) {
  using I32 = V16<TypeParam, std::int32_t>;
  EXPECT_THROW(
      I32([](auto) -> std::int32_t { throw std::runtime_error("generator"); }),
      std::runtime_error);
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
  using U32 = V16<TypeParam, std::uint32_t>;
  using I32 = V16<TypeParam, std::int32_t>;
  const U32 Y([](auto I) { return static_cast<std::uint32_t>(I + 1); });
  EXPECT_EQ(lanes(simd::permute(Y, I32(5))),
            (std::vector<std::uint32_t>{2, 2, 2, 2}));
  EXPECT_EQ(lanes(simd::permute(Y, I32(-1))),
            (std::vector<std::uint32_t>{4, 4, 4, 4}));
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

TEST(SimdMaskTest, WideMaskPacksLanesThatFit) {
  const simd::mask<char, 65> M([](auto I) { return I == 0 || I == 63; });
  EXPECT_EQ(M.to_ullong(), 0x8000000000000001ULL);
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

#if defined(_M_X64) || defined(__SSE2__)
TYPED_TEST(SimdTest, RoundingIgnoresRoundingMode) {
  if (!usesIntegerRounding<TypeParam>()) {
    GTEST_SKIP() << "only the x86 rounding without SSE4.1 is checked";
  } else if (!StrictFloatingPoint) {
    GTEST_SKIP() << "the test is built without a strict floating-point option";
  } else {
    checkRoundingIgnoresRoundingMode<TypeParam, float>();
    checkRoundingIgnoresRoundingMode<TypeParam, double>();
  }
}

TYPED_TEST(SimdTest, RoundingIgnoresDenormalsAreZero) {
  if (!usesIntegerRounding<TypeParam>()) {
    GTEST_SKIP() << "only the x86 rounding without SSE4.1 is checked";
  } else if (!StrictFloatingPoint) {
    GTEST_SKIP() << "the test is built without a strict floating-point option";
  } else {
    if (!hasDenormalsAreZero()) {
      GTEST_SKIP() << "the CPU has no denormals-are-zero bit";
    }
    checkRoundingIgnoresDenormalsAreZero<TypeParam, float>();
    checkRoundingIgnoresDenormalsAreZero<TypeParam, double>();
  }
}

TYPED_TEST(SimdTest, RoundingRaisesNoException) {
  if (!usesIntegerRounding<TypeParam>()) {
    GTEST_SKIP() << "only the x86 rounding without SSE4.1 is checked";
  } else if (!StrictFloatingPoint) {
    GTEST_SKIP() << "the test is built without a strict floating-point option";
  } else {
    checkRoundingRaisesNoException<TypeParam, float>();
    checkRoundingRaisesNoException<TypeParam, double>();
  }
}
#endif

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
  EXPECT_EQ(lanes(X),
            (std::vector<std::uint16_t>{7, 7, 7, 0xbeef, 7, 7, 7, 7}));
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

TEST(ValVariantTest, ClassTypesConvertingToScalarsPickTheScalar) {
  std::atomic<std::uint32_t> Counter{42};
  const WasmEdge::ValVariant FromAtomic(Counter);
  EXPECT_EQ(FromAtomic.get<std::uint32_t>(), 42U);
  std::uint64_t Wide = std::uint64_t{1} << 40;
  const WasmEdge::ValVariant FromRef(std::ref(Wide));
  EXPECT_EQ(FromRef.get<std::uint64_t>(), std::uint64_t{1} << 40);
}

static_assert(
    std::is_const_v<decltype(std::declval<WasmEdge::RefVariant>().getType())>,
    "a mutation of the returned copy must not compile");
#if defined(__GNUC__) || defined(__clang__) || defined(_M_X64)
static_assert(!std::is_class_v<WasmEdge::RawV128>,
              "AOT and JIT code return RawV128 in a vector register");
#endif

} // namespace
