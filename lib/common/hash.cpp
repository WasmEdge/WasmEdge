#include "common/hash.h"
#include "common/endian.h"

#include <cstdint>
#include <cstring>

namespace {

inline uint64_t read(WasmEdge::Span<const std::byte, 4> Data) noexcept {
  uint32_t V;
  std::memcpy(&V, Data.data(), 4);
  if constexpr (WasmEdge::Endian::native == WasmEdge::Endian::little) {
    return V;
  } else {
    return WasmEdge::byteswap(V);
  }
}

static const uint64_t UseSeed = WasmEdge::Hash::RandEngine();

} // namespace

namespace WasmEdge::Hash {

WASMEDGE_EXPORT uint64_t Hash::a5Hash(Span<const std::byte> Data) noexcept {
  uint64_t Val01 = UINT64_C(0x5555555555555555);
  uint64_t Val10 = UINT64_C(0xAAAAAAAAAAAAAAAA);

  // PI-mantissa seeds XORed with message length.
  uint64_t Seed1 = UINT64_C(0x243F6A8885A308D3) ^ Data.size();
  uint64_t Seed2 = UINT64_C(0x452821E638D01377) ^ Data.size();

  // Mix in the per-process seed.
  a5hashMul128(Seed2 ^ (UseSeed & Val10), Seed1 ^ (UseSeed & Val01), Seed1,
               Seed2);

  if (Data.size() > 16) {
    Val01 ^= Seed1;
    Val10 ^= Seed2;

    do {
      a5hashMul128(read(Data.first<4>()) << 32 ^
                       read(Data.subspan<4>().first<4>()) ^ Seed1,
                   read(Data.subspan<8>().first<4>()) << 32 ^
                       read(Data.subspan<12>().first<4>()) ^ Seed2,
                   Seed1, Seed2);

      Data = Data.subspan<16>();

      Seed1 += Val01;
      Seed2 += Val10;
    } while (Data.size() > 16);
  }

  if (unlikely(Data.size() > 3)) {
    size_t Mo = Data.size() >> 3;

    Seed1 ^= read(Data.first<4>()) << 32 | read(Data.last<4>());
    Seed2 ^= read(Data.subspan(Mo * 4).first<4>()) << 32 |
             read(Data.last(4 + Mo * 4).first<4>());
  } else if (unlikely(Data.size() != 0)) {
    // 1-3 bytes.
    Seed1 ^= static_cast<uint64_t>(Data[0]);
    if (Data.size() > 1) {
      Seed1 ^= static_cast<uint64_t>(Data[1]) << 8;
      if (Data.size() > 2) {
        Seed1 ^= static_cast<uint64_t>(Data[2]) << 16;
      }
    }
  }

  a5hashMul128(Seed1, Seed2, Seed1, Seed2);
  a5hashMul128(Val01 ^ Seed1, Seed2, Seed1, Seed2);
  return Seed1 ^ Seed2;
}

} // namespace WasmEdge::Hash
