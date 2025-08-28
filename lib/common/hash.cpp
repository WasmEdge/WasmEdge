#include "common/hash.h"
#include "common/endian.h"

namespace {

inline uint64_t mulMod(uint64_t A, uint64_t B, uint64_t M) noexcept {
  uint64_t R = 0;
  while (B) {
    if (B & 1) {
      uint64_t R2 = R + A;
      if (R2 < R) {
        R2 -= M;
      }
      R = R2 % M;
    }
    B >>= 1;
    if (B) {
      uint64_t A2 = A + A;
      if (A2 < A) {
        A2 -= M;
      }
      A = A2 % M;
    }
  }
  return R;
}

inline uint64_t powMod(uint64_t A, uint64_t B, uint64_t M) noexcept {
  uint64_t R = 1;
  while (B) {
    if (B & 1) {
      R = mulMod(R, A, M);
    }
    B >>= 1;
    if (B) {
      A = mulMod(A, A, M);
    }
  }
  return R;
}

inline bool sprp(uint64_t N, uint64_t A) noexcept {
  uint64_t D = N - 1;
  uint8_t S = 0;
  while (!(D & 0xff)) {
    D >>= 8;
    S += 8;
  }
  if (!(D & 0xf)) {
    D >>= 4;
    S += 4;
  }
  if (!(D & 0x3)) {
    D >>= 2;
    S += 2;
  }
  if (!(D & 0x1)) {
    D >>= 1;
    S += 1;
  }
  uint64_t B = powMod(A, D, N);
  if ((B == 1) || (B == (N - 1))) {
    return true;
  }
  uint8_t R;
  for (R = 1; R < S; R++) {
    B = mulMod(B, B, N);
    if (B <= 1) {
      return false;
    }
    if (B == (N - 1)) {
      return true;
    }
  }
  return false;
}

inline bool isPrime(uint64_t N) noexcept {
  if (N < 2 || !(N & 1)) {
    return false;
  }
  if (N < 4) {
    return true;
  }
  if (!sprp(N, 2)) {
    return false;
  }
  if (N < 2047) {
    return true;
  }
  if (!sprp(N, 3)) {
    return false;
  }
  if (!sprp(N, 5)) {
    return false;
  }
  if (!sprp(N, 7)) {
    return false;
  }
  if (!sprp(N, 11)) {
    return false;
  }
  if (!sprp(N, 13)) {
    return false;
  }
  if (!sprp(N, 17)) {
    return false;
  }
  if (!sprp(N, 19)) {
    return false;
  }
  if (!sprp(N, 23)) {
    return false;
  }
  if (!sprp(N, 29)) {
    return false;
  }
  if (!sprp(N, 31)) {
    return false;
  }
  if (!sprp(N, 37)) {
    return false;
  }
  return true;
}

inline int popcount(uint64_t X) noexcept {
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
  return __builtin_popcountll(X);
#elif defined(_MSC_VER) && defined(_WIN64)
#if defined(_M_X64)
  return static_cast<int>(_mm_popcnt_u64(X));
#else
  return static_cast<int>(_CountOneBits64(X));
#endif
#else
  X -= (X >> 1) & 0x5555555555555555;
  X = (X & 0x3333333333333333) + ((X >> 2) & 0x3333333333333333);
  X = (X + (X >> 4)) & 0x0f0f0f0f0f0f0f0f;
  X = (X * 0x0101010101010101) >> 56;
  return static_cast<int>(X);
#endif
}

std::array<uint64_t, 4> generate() noexcept {
  std::array<uint64_t, 4> Secret;
  const std::array<uint8_t, 70> C = {
      0x0f, 0x17, 0x1b, 0x1d, 0x1e, 0x27, 0x2b, 0x2d, 0x2e, 0x33, 0x35, 0x36,
      0x39, 0x3a, 0x3c, 0x47, 0x4b, 0x4d, 0x4e, 0x53, 0x55, 0x56, 0x59, 0x5a,
      0x5c, 0x63, 0x65, 0x66, 0x69, 0x6a, 0x6c, 0x71, 0x72, 0x74, 0x78, 0x87,
      0x8b, 0x8d, 0x8e, 0x93, 0x95, 0x96, 0x99, 0x9a, 0x9c, 0xa3, 0xa5, 0xa6,
      0xa9, 0xaa, 0xac, 0xb1, 0xb2, 0xb4, 0xb8, 0xc3, 0xc5, 0xc6, 0xc9, 0xca,
      0xcc, 0xd1, 0xd2, 0xd4, 0xd8, 0xe1, 0xe2, 0xe4, 0xe8, 0xf0};
  std::uniform_int_distribution<uint64_t> Dist(
      UINT64_C(0), static_cast<uint64_t>(C.size() - 1));
  for (size_t I = 0; I < 4; I++) {
    bool Ok;
    do {
      Ok = true;
      Secret[I] = 0;
      for (size_t J = 0; J < 64; J += 8) {
        Secret[I] |= static_cast<uint64_t>(C[Dist(WasmEdge::Hash::RandEngine)])
                     << J;
      }
      if (Secret[I] % 2 == 0) {
        Ok = false;
        continue;
      }
      for (size_t J = 0; J < I; J++) {
        if (popcount(Secret[J] ^ Secret[I]) != 32) {
          Ok = false;
          break;
        }
      }
      if (Ok && !isPrime(Secret[I]))
        Ok = false;
    } while (!Ok);
  }
  return Secret;
}

inline uint64_t read(WasmEdge::Span<const std::byte, 8> Data) noexcept {
  uint64_t V;
  std::memcpy(&V, Data.data(), 8);
  if constexpr (WasmEdge::Endian::native == WasmEdge::Endian::little) {
    return V;
  } else {
    return WasmEdge::byteswap(V);
  }
}
inline uint64_t read(WasmEdge::Span<const std::byte, 4> Data) noexcept {
  uint32_t V;
  std::memcpy(&V, Data.data(), 4);
  if constexpr (WasmEdge::Endian::native == WasmEdge::Endian::little) {
    return V;
  } else {
    return WasmEdge::byteswap(V);
  }
}

inline uint64_t read_small(WasmEdge::Span<const std::byte> Data) noexcept {
  return (static_cast<uint64_t>(Data[0]) << 56) |
         (static_cast<uint64_t>(Data[Data.size() >> 1]) << 32) |
         static_cast<uint64_t>(Data[Data.size() - 1]);
}

static const std::array<uint64_t, 4> Secret = generate();

} // namespace

namespace WasmEdge::Hash {

WASMEDGE_EXPORT uint64_t Hash::rapidHash(Span<const std::byte> Data) noexcept {
  const auto Size = Data.size();
  uint64_t Seed = Secret[3];
  Seed ^= rapidMix(Seed ^ Secret[0], Secret[1]) ^ Size;
  uint64_t A, B;
  if (likely(Data.size() <= 16)) {
    if (likely(Data.size() >= 4)) {
      A = (read(Data.first<4>()) << 32) | read(Data.last<4>());
      const uint64_t delta = ((Data.size() & 24) >> (Data.size() >> 3));
      B = (read(Data.subspan(delta).first<4>()) << 32) |
          read(Data.last(4 + delta).first<4>());
    } else if (likely(Data.size() > 0)) {
      A = read_small(Data);
      B = 0;
    } else {
      A = B = 0;
    }
  } else {
    if (unlikely(Data.size() > 48)) {
      uint64_t See1 = Seed, See2 = Seed;
      while (likely(Data.size() >= 96)) {
        Seed = rapidMix(read(Data.first<8>()) ^ Secret[0],
                        read(Data.subspan<8>().first<8>()) ^ Seed);
        See1 = rapidMix(read(Data.subspan<16>().first<8>()) ^ Secret[1],
                        read(Data.subspan<24>().first<8>()) ^ See1);
        See2 = rapidMix(read(Data.subspan<32>().first<8>()) ^ Secret[2],
                        read(Data.subspan<40>().first<8>()) ^ See2);
        Seed = rapidMix(read(Data.subspan<48>().first<8>()) ^ Secret[0],
                        read(Data.subspan<56>().first<8>()) ^ Seed);
        See1 = rapidMix(read(Data.subspan<64>().first<8>()) ^ Secret[1],
                        read(Data.subspan<72>().first<8>()) ^ See1);
        See2 = rapidMix(read(Data.subspan<80>().first<8>()) ^ Secret[2],
                        read(Data.subspan<88>().first<8>()) ^ See2);
        Data = Data.subspan<96>();
      }
      if (unlikely(Data.size() >= 48)) {
        Seed = rapidMix(read(Data.first<8>()) ^ Secret[0],
                        read(Data.subspan<8>().first<8>()) ^ Seed);
        See1 = rapidMix(read(Data.subspan<16>().first<8>()) ^ Secret[1],
                        read(Data.subspan<24>().first<8>()) ^ See1);
        See2 = rapidMix(read(Data.subspan<32>().first<8>()) ^ Secret[2],
                        read(Data.subspan<40>().first<8>()) ^ See2);
        Data = Data.subspan<48>();
      }

      Seed ^= See1 ^ See2;
    }
    if (Data.size() > 16) {
      Seed = rapidMix(read(Data.first<8>()) ^ Secret[2],
                      read(Data.subspan<8>().first<8>()) ^ Seed ^ Secret[1]);
      if (Data.size() > 32)
        Seed = rapidMix(read(Data.subspan<16>().first<8>()) ^ Secret[2],
                        read(Data.subspan<24>().first<8>()) ^ Seed);
    }
    A = read(Data.last<16>().first<8>());
    B = read(Data.last<8>());
  }
  A ^= Secret[1];
  B ^= Seed;
  rapidMum(A, B);
  return rapidMix(A ^ Secret[0] ^ Size, B ^ Secret[1]);
}

} // namespace WasmEdge::Hash
