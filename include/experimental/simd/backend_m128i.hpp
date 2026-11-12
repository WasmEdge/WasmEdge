// SPDX-License-Identifier: CC0-1.0
///
// simd - A C++17 implementation of a subset of the C++26 std::simd library
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication
// along with this software. If not, see
// <http://creativecommons.org/publicdomain/zero/1.0/>.
///

#pragma once

#include "experimental/simd/backend_array.hpp"
#include "experimental/simd/detail.hpp"

#if defined(_M_X64) || defined(__SSE2__)

#include <emmintrin.h>
#include <limits>
#if defined(__SSE4_1__) || defined(__AVX__)
#include <smmintrin.h>
#endif

namespace cxx26 {
namespace simd {
namespace detail {

template <class T, simd_size_type N> struct simd_backend<T, m128i_abi<N>> {
  static_assert(N * sizeof(T) == 16);

  using value_type = T;
  using storage_type = __m128i;
  using mask_storage = __m128i;

  using array_backend = simd_backend<T, array_abi<N>>;
  using array_storage_type = typename array_backend::storage_type;

  static constexpr bool is_float = std::is_same_v<T, float>;
  static constexpr bool is_double = std::is_same_v<T, double>;
  static constexpr bool is_signed_int =
      std::is_integral_v<T> && std::is_signed_v<T>;

  using signed_backend = simd_backend<integer_from_t<sizeof(T)>, m128i_abi<N>>;

  static __m128 as_ps(const storage_type &S) noexcept {
    return _mm_castsi128_ps(S);
  }
  static __m128d as_pd(const storage_type &S) noexcept {
    return _mm_castsi128_pd(S);
  }

  static array_storage_type to_array(const storage_type &S) noexcept {
    return cxx20::bit_cast<array_storage_type>(S);
  }
  template <class A> static storage_type from_array(const A &S) noexcept {
    return cxx20::bit_cast<storage_type>(S);
  }

  template <class G> static storage_type generate(G &&Gen) {
    return from_array(array_backend::generate(std::forward<G>(Gen)));
  }
  static storage_type broadcast(T Value) noexcept {
    if constexpr (is_float) {
      return _mm_castps_si128(_mm_set1_ps(Value));
    } else if constexpr (is_double) {
      return _mm_castpd_si128(_mm_set1_pd(Value));
    } else if constexpr (sizeof(T) == 1) {
      return _mm_set1_epi8(static_cast<char>(Value));
    } else if constexpr (sizeof(T) == 2) {
      return _mm_set1_epi16(static_cast<short>(Value));
    } else if constexpr (sizeof(T) == 4) {
      return _mm_set1_epi32(static_cast<int>(Value));
    } else {
      return _mm_set1_epi64x(static_cast<long long>(Value));
    }
  }
  static T get(const storage_type &S, simd_size_type I) noexcept {
    return array_backend::get(to_array(S), I);
  }
  static void set_lane(storage_type &S, simd_size_type I, T Value) noexcept {
    array_storage_type A = to_array(S);
    array_backend::set_lane(A, I, Value);
    S = from_array(A);
  }

  template <class U>
  static storage_type convert(const storage_type &S) noexcept {
    if constexpr (std::is_integral_v<T> && std::is_integral_v<U>) {
      return S;
    } else if constexpr (is_float && std::is_same_v<U, std::int32_t>) {
      return _mm_castps_si128(_mm_cvtepi32_ps(S));
    } else if constexpr (std::is_same_v<T, std::int32_t> &&
                         std::is_same_v<U, float>) {
      return _mm_cvttps_epi32(_mm_castsi128_ps(S));
    } else {
      using from_backend = simd_backend<U, m128i_abi<N>>;
      return from_array(
          array_backend::template convert<U>(from_backend::to_array(S)));
    }
  }

#define CXX26_SIMD_M128I_ADD_SUB(NAME)                                         \
  static storage_type NAME(const storage_type &A,                              \
                           const storage_type &B) noexcept {                   \
    if constexpr (is_float) {                                                  \
      return _mm_castps_si128(_mm_##NAME##_ps(as_ps(A), as_ps(B)));            \
    } else if constexpr (is_double) {                                          \
      return _mm_castpd_si128(_mm_##NAME##_pd(as_pd(A), as_pd(B)));            \
    } else if constexpr (sizeof(T) == 1) {                                     \
      return _mm_##NAME##_epi8(A, B);                                          \
    } else if constexpr (sizeof(T) == 2) {                                     \
      return _mm_##NAME##_epi16(A, B);                                         \
    } else if constexpr (sizeof(T) == 4) {                                     \
      return _mm_##NAME##_epi32(A, B);                                         \
    } else {                                                                   \
      return _mm_##NAME##_epi64(A, B);                                         \
    }                                                                          \
  }
  CXX26_SIMD_M128I_ADD_SUB(add)
  CXX26_SIMD_M128I_ADD_SUB(sub)
#undef CXX26_SIMD_M128I_ADD_SUB
  static storage_type mul(const storage_type &A,
                          const storage_type &B) noexcept {
    if constexpr (is_float) {
      return _mm_castps_si128(_mm_mul_ps(as_ps(A), as_ps(B)));
    } else if constexpr (is_double) {
      return _mm_castpd_si128(_mm_mul_pd(as_pd(A), as_pd(B)));
    } else if constexpr (sizeof(T) == 1) {
      const __m128i Even = _mm_mullo_epi16(A, B);
      const __m128i Odd =
          _mm_mullo_epi16(_mm_srli_epi16(A, 8), _mm_srli_epi16(B, 8));
      return _mm_or_si128(
          _mm_and_si128(Even, _mm_set1_epi16(static_cast<short>(0x00ff))),
          _mm_slli_epi16(Odd, 8));
    } else if constexpr (sizeof(T) == 2) {
      return _mm_mullo_epi16(A, B);
    } else if constexpr (sizeof(T) == 4) {
      const __m128i Even = _mm_mul_epu32(A, B);
      const __m128i Odd =
          _mm_mul_epu32(_mm_srli_epi64(A, 32), _mm_srli_epi64(B, 32));
      return _mm_unpacklo_epi32(
          _mm_shuffle_epi32(Even, _MM_SHUFFLE(0, 0, 2, 0)),
          _mm_shuffle_epi32(Odd, _MM_SHUFFLE(0, 0, 2, 0)));
    } else {
      const __m128i Cross =
          _mm_add_epi64(_mm_mul_epu32(_mm_srli_epi64(A, 32), B),
                        _mm_mul_epu32(A, _mm_srli_epi64(B, 32)));
      return _mm_add_epi64(_mm_mul_epu32(A, B), _mm_slli_epi64(Cross, 32));
    }
  }
  static storage_type div(const storage_type &A,
                          const storage_type &B) noexcept {
    if constexpr (is_float) {
      return _mm_castps_si128(_mm_div_ps(as_ps(A), as_ps(B)));
    } else if constexpr (is_double) {
      return _mm_castpd_si128(_mm_div_pd(as_pd(A), as_pd(B)));
    } else {
      return from_array(array_backend::div(to_array(A), to_array(B)));
    }
  }
  static storage_type bit_and(const storage_type &A,
                              const storage_type &B) noexcept {
    return _mm_and_si128(A, B);
  }
  static storage_type bit_or(const storage_type &A,
                             const storage_type &B) noexcept {
    return _mm_or_si128(A, B);
  }
  static storage_type bit_xor(const storage_type &A,
                              const storage_type &B) noexcept {
    return _mm_xor_si128(A, B);
  }
  static storage_type bit_not(const storage_type &A) noexcept {
    return _mm_xor_si128(A, _mm_set1_epi32(-1));
  }

#define CXX26_SIMD_M128I_VIA_ARRAY(NAME)                                       \
  static storage_type NAME(const storage_type &A,                              \
                           const storage_type &B) noexcept {                   \
    return from_array(array_backend::NAME(to_array(A), to_array(B)));          \
  }
  CXX26_SIMD_M128I_VIA_ARRAY(shl)
  CXX26_SIMD_M128I_VIA_ARRAY(shr)
  CXX26_SIMD_M128I_VIA_ARRAY(permute)
#undef CXX26_SIMD_M128I_VIA_ARRAY

  static storage_type shl(const storage_type &A, simd_size_type S) noexcept {
    const __m128i Count = _mm_cvtsi32_si128(S);
    if constexpr (sizeof(T) == 1) {
      return _mm_and_si128(_mm_sll_epi16(A, Count),
                           _mm_set1_epi8(static_cast<char>(0xff << S)));
    } else if constexpr (sizeof(T) == 2) {
      return _mm_sll_epi16(A, Count);
    } else if constexpr (sizeof(T) == 4) {
      return _mm_sll_epi32(A, Count);
    } else {
      return _mm_sll_epi64(A, Count);
    }
  }
  static storage_type shr(const storage_type &A, simd_size_type S) noexcept {
    const __m128i Count = _mm_cvtsi32_si128(S);
    if constexpr (sizeof(T) == 1 && is_signed_int) {
      const __m128i Wide = _mm_cvtsi32_si128(S + 8);
      return _mm_packs_epi16(_mm_sra_epi16(_mm_unpacklo_epi8(A, A), Wide),
                             _mm_sra_epi16(_mm_unpackhi_epi8(A, A), Wide));
    } else if constexpr (sizeof(T) == 1) {
      return _mm_and_si128(_mm_srl_epi16(A, Count),
                           _mm_set1_epi8(static_cast<char>(0xff >> S)));
    } else if constexpr (sizeof(T) == 2 && is_signed_int) {
      return _mm_sra_epi16(A, Count);
    } else if constexpr (sizeof(T) == 2) {
      return _mm_srl_epi16(A, Count);
    } else if constexpr (sizeof(T) == 4 && is_signed_int) {
      return _mm_sra_epi32(A, Count);
    } else if constexpr (sizeof(T) == 4) {
      return _mm_srl_epi32(A, Count);
    } else if constexpr (is_signed_int) {
      const __m128i Sign = _mm_srl_epi64(
          _mm_set1_epi64x(std::numeric_limits<long long>::min()), Count);
      return _mm_sub_epi64(_mm_xor_si128(_mm_srl_epi64(A, Count), Sign), Sign);
    } else {
      return _mm_srl_epi64(A, Count);
    }
  }
  static storage_type neg(const storage_type &A) noexcept {
    if constexpr (std::is_floating_point_v<T>) {
      return bit_xor(A, broadcast(static_cast<T>(-0.0)));
    } else {
      return sub(_mm_setzero_si128(), A);
    }
  }

  static storage_type sqrt(const storage_type &A) noexcept {
    if constexpr (is_float) {
      return _mm_castps_si128(_mm_sqrt_ps(as_ps(A)));
    } else {
      return _mm_castpd_si128(_mm_sqrt_pd(as_pd(A)));
    }
  }

  // std::round_to_nearest rounds ties to even, the IEEE default.
#if defined(__SSE4_1__) || defined(__AVX__)
  template <std::float_round_style R>
  static storage_type round(const storage_type &A) noexcept {
    constexpr int Mode =
        (R == std::round_to_nearest            ? _MM_FROUND_TO_NEAREST_INT
         : R == std::round_toward_infinity     ? _MM_FROUND_TO_POS_INF
         : R == std::round_toward_neg_infinity ? _MM_FROUND_TO_NEG_INF
                                               : _MM_FROUND_TO_ZERO) |
        _MM_FROUND_NO_EXC;
    if constexpr (is_float) {
      return _mm_castps_si128(_mm_round_ps(as_ps(A), Mode));
    } else {
      return _mm_castpd_si128(_mm_round_pd(as_pd(A), Mode));
    }
  }
#else
  // x86 has no rounding instruction before SSE4.1. The rounding works on the
  // bit pattern with integer instructions, so it does not depend on the MXCSR
  // rounding mode or on denormals-are-zero.
  using int_type = integer_from_t<sizeof(T)>;
  static constexpr int mantissa_bits = std::numeric_limits<T>::digits - 1;
  static constexpr int_type exponent_bias =
      std::numeric_limits<T>::max_exponent - 1;

  // For a biased exponent E with 0 <= E - bias < mantissa_bits, returns the
  // integer 2^(mantissa_bits - (E - bias)), which is one unit in the place of
  // the lowest integer bit of the significand. Other lanes are unspecified.
  static storage_type integer_unit(const storage_type &Exponent) noexcept {
    if constexpr (is_float) {
      // The float 2^K converts exactly to the integer 2^K.
      const __m128i Pow = _mm_slli_epi32(
          _mm_sub_epi32(_mm_set1_epi32(2 * exponent_bias + mantissa_bits),
                        Exponent),
          mantissa_bits);
      return _mm_cvttps_epi32(_mm_castsi128_ps(Pow));
    } else {
      const __m128i Shift = _mm_sub_epi64(
          _mm_set1_epi64x(exponent_bias + mantissa_bits), Exponent);
      const __m128i One = _mm_set1_epi64x(1);
      const __m128i Low = _mm_sll_epi64(One, Shift);
      const __m128i High = _mm_sll_epi64(One, _mm_unpackhi_epi64(Shift, Shift));
      return _mm_castpd_si128(
          _mm_move_sd(_mm_castsi128_pd(High), _mm_castsi128_pd(Low)));
    }
  }

  template <std::float_round_style R>
  static storage_type round(const storage_type &A) noexcept {
    using ib = signed_backend;
    const auto Splat = [](auto Value) {
      return ib::broadcast(static_cast<int_type>(Value));
    };
    const storage_type Zero = _mm_setzero_si128();
    const storage_type SignMask = Splat(std::numeric_limits<int_type>::min());
    const storage_type OneBits = Splat(exponent_bias << mantissa_bits);
    const storage_type Sign = bit_and(A, SignMask);
    const storage_type Abs = _mm_andnot_si128(SignMask, A);
    [[maybe_unused]] const storage_type Positive = ib::eq(Sign, Zero);
    [[maybe_unused]] const storage_type NonZero = bit_not(ib::eq(Abs, Zero));

    // |A| < 1 rounds to zero or to one with the sign of A.
    storage_type Small = Sign;
    if constexpr (R == std::round_toward_infinity) {
      Small = bit_or(Small, bit_and(bit_and(Positive, NonZero), OneBits));
    } else if constexpr (R == std::round_toward_neg_infinity) {
      Small =
          bit_or(Small, bit_and(_mm_andnot_si128(Positive, NonZero), OneBits));
    } else if constexpr (R == std::round_to_nearest) {
      const storage_type HalfBits = Splat((exponent_bias - 1) << mantissa_bits);
      Small = bit_or(Small, bit_and(ib::gt(Abs, HalfBits), OneBits));
    }

    // 1 <= |A| < 2^mantissa_bits clears the fraction bits, then adds one unit
    // when the rounding goes away from zero.
    const storage_type Unit = integer_unit(ib::shr(Abs, mantissa_bits));
    const storage_type FractionMask = ib::sub(Unit, Splat(1));
    const storage_type Fraction = bit_and(Abs, FractionMask);
    const storage_type Truncated = _mm_andnot_si128(FractionMask, A);
    [[maybe_unused]] const storage_type HasFraction =
        bit_not(ib::eq(Fraction, Zero));
    storage_type Up = Zero;
    if constexpr (R == std::round_toward_infinity) {
      Up = bit_and(HasFraction, Positive);
    } else if constexpr (R == std::round_toward_neg_infinity) {
      Up = _mm_andnot_si128(Positive, HasFraction);
    } else if constexpr (R == std::round_to_nearest) {
      const storage_type Half = ib::shr(Unit, 1);
      const storage_type Odd = bit_not(ib::eq(bit_and(Truncated, Unit), Zero));
      Up = bit_or(ib::gt(Fraction, Half), bit_and(ib::eq(Fraction, Half), Odd));
    }
    const storage_type Middle = ib::add(Truncated, bit_and(Up, Unit));

    // |A| >= 2^mantissa_bits is already integral, infinite or NaN. A NaN gets
    // its quiet bit set.
    const storage_type InfBits =
        Splat((2 * exponent_bias + 1) << mantissa_bits);
    const storage_type QuietBit = Splat(int_type{1} << (mantissa_bits - 1));
    const storage_type Large =
        bit_or(A, bit_and(ib::gt(Abs, InfBits), QuietBit));

    const storage_type IntegralBits =
        Splat((exponent_bias + mantissa_bits) << mantissa_bits);
    return select(ib::gt(OneBits, Abs), Small,
                  select(ib::gt(IntegralBits, Abs), Middle, Large));
  }
#endif
  static storage_type ceil(const storage_type &A) noexcept {
    return round<std::round_toward_infinity>(A);
  }
  static storage_type floor(const storage_type &A) noexcept {
    return round<std::round_toward_neg_infinity>(A);
  }
  static storage_type trunc(const storage_type &A) noexcept {
    return round<std::round_toward_zero>(A);
  }
  static storage_type roundeven(const storage_type &A) noexcept {
    return round<std::round_to_nearest>(A);
  }

  static mask_storage eq(const storage_type &A,
                         const storage_type &B) noexcept {
    if constexpr (is_float) {
      return _mm_castps_si128(_mm_cmpeq_ps(as_ps(A), as_ps(B)));
    } else if constexpr (is_double) {
      return _mm_castpd_si128(_mm_cmpeq_pd(as_pd(A), as_pd(B)));
    } else if constexpr (sizeof(T) == 1) {
      return _mm_cmpeq_epi8(A, B);
    } else if constexpr (sizeof(T) == 2) {
      return _mm_cmpeq_epi16(A, B);
    } else if constexpr (sizeof(T) == 4) {
      return _mm_cmpeq_epi32(A, B);
    } else {
      const __m128i Half = _mm_cmpeq_epi32(A, B);
      return _mm_and_si128(Half,
                           _mm_shuffle_epi32(Half, _MM_SHUFFLE(2, 3, 0, 1)));
    }
  }
  static mask_storage gt(const storage_type &A,
                         const storage_type &B) noexcept {
    if constexpr (is_float) {
      return _mm_castps_si128(_mm_cmpgt_ps(as_ps(A), as_ps(B)));
    } else if constexpr (is_double) {
      return _mm_castpd_si128(_mm_cmpgt_pd(as_pd(A), as_pd(B)));
    } else if constexpr (is_signed_int && sizeof(T) == 1) {
      return _mm_cmpgt_epi8(A, B);
    } else if constexpr (is_signed_int && sizeof(T) == 2) {
      return _mm_cmpgt_epi16(A, B);
    } else if constexpr (is_signed_int && sizeof(T) == 4) {
      return _mm_cmpgt_epi32(A, B);
    } else if constexpr (sizeof(T) < 8) {
      const __m128i Bias = signed_backend::broadcast(
          std::numeric_limits<integer_from_t<sizeof(T)>>::min());
      return signed_backend::gt(_mm_xor_si128(A, Bias), _mm_xor_si128(B, Bias));
    } else {
      constexpr int Min = std::numeric_limits<std::int32_t>::min();
      const __m128i Bias =
          is_signed_int ? _mm_set_epi32(0, Min, 0, Min) : _mm_set1_epi32(Min);
      const __m128i X = _mm_xor_si128(A, Bias);
      const __m128i Y = _mm_xor_si128(B, Bias);
      const __m128i Gt = _mm_cmpgt_epi32(X, Y);
      const __m128i HighEq =
          _mm_shuffle_epi32(_mm_cmpeq_epi32(X, Y), _MM_SHUFFLE(3, 3, 1, 1));
      return _mm_or_si128(
          _mm_shuffle_epi32(Gt, _MM_SHUFFLE(3, 3, 1, 1)),
          _mm_and_si128(HighEq,
                        _mm_shuffle_epi32(Gt, _MM_SHUFFLE(2, 2, 0, 0))));
    }
  }
  static mask_storage lt(const storage_type &A,
                         const storage_type &B) noexcept {
    return gt(B, A);
  }
#define CXX26_SIMD_M128I_COMPARE(NAME, CMP, INTEGER)                           \
  static mask_storage NAME(const storage_type &A,                              \
                           const storage_type &B) noexcept {                   \
    if constexpr (is_float) {                                                  \
      return _mm_castps_si128(_mm_cmp##CMP##_ps(as_ps(A), as_ps(B)));          \
    } else if constexpr (is_double) {                                          \
      return _mm_castpd_si128(_mm_cmp##CMP##_pd(as_pd(A), as_pd(B)));          \
    } else {                                                                   \
      return INTEGER;                                                          \
    }                                                                          \
  }
  CXX26_SIMD_M128I_COMPARE(ne, neq, bit_not(eq(A, B)))
  CXX26_SIMD_M128I_COMPARE(le, le, bit_not(gt(A, B)))
  CXX26_SIMD_M128I_COMPARE(ge, ge, bit_not(gt(B, A)))
#undef CXX26_SIMD_M128I_COMPARE

  static storage_type select(const mask_storage &M, const storage_type &A,
                             const storage_type &B) noexcept {
    return _mm_or_si128(_mm_and_si128(M, A), _mm_andnot_si128(M, B));
  }

  static unsigned long long to_ullong(const storage_type &S) noexcept {
    if constexpr (sizeof(T) == 1) {
      return static_cast<unsigned long long>(_mm_movemask_epi8(S));
    } else if constexpr (sizeof(T) == 2) {
      return static_cast<unsigned long long>(
          _mm_movemask_epi8(_mm_packs_epi16(S, _mm_setzero_si128())));
    } else if constexpr (sizeof(T) == 4) {
      return static_cast<unsigned long long>(_mm_movemask_ps(as_ps(S)));
    } else {
      return static_cast<unsigned long long>(_mm_movemask_pd(as_pd(S)));
    }
  }
};

} // namespace detail
} // namespace simd
} // namespace cxx26

#endif
