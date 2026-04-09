#include "common/hash.h"

#include <array>
#include <cstring>

namespace {

using namespace WasmEdge;

// rapidhash v3 secrets — only the 4 used by the Nano variant.
// Indices match upstream rapid_secret[]: [0], [1], [2], [7].
static constexpr std::array<uint64_t, 4> Secret = {
    0x2d358dccaa6c78a5ull, // rapid_secret[0]
    0x8bb84b93962eacc9ull, // rapid_secret[1]
    0x4b33a62ed433d4a3ull, // rapid_secret[2]
    0xaaaaaaaaaaaaaaaaull, // rapid_secret[7]
};

static const uint64_t RandomSeed = Hash::RandEngine();

// Intentional: assumes little-endian. Trades platform-independent hash
// values for speed by skipping byte-swap on big-endian targets.
inline uint64_t read(Span<const std::byte, 8> Data) noexcept {
  uint64_t V;
  std::memcpy(&V, Data.data(), 8);
  return V;
}
inline uint64_t read(Span<const std::byte, 4> Data) noexcept {
  uint32_t V;
  std::memcpy(&V, Data.data(), 4);
  return V;
}

} // namespace

namespace WasmEdge::Hash {

WASMEDGE_EXPORT uint64_t Hash::rapidHash(Span<const std::byte> Data) noexcept {
  uint64_t Seed = RandomSeed;
  Seed ^= rapidMix(Seed ^ Secret[2], Secret[1]);
  uint64_t A = 0, B = 0;
  if (likely(Data.size() <= 16)) {
    if (Data.size() >= 4) {
      Seed ^= Data.size();
      if (Data.size() >= 8) {
        A = read(Data.first<8>());
        B = read(Data.last<8>());
      } else {
        A = read(Data.first<4>());
        B = read(Data.last<4>());
      }
    } else if (Data.size() > 0) {
      A = (static_cast<uint64_t>(Data[0]) << 45) |
          static_cast<uint64_t>(Data[Data.size() - 1]);
      B = static_cast<uint64_t>(Data[Data.size() >> 1]);
    } else {
      A = B = 0;
    }
  } else {
    if (Data.size() > 48) {
      uint64_t See1 = Seed, See2 = Seed;
      do {
        Seed = rapidMix(read(Data.first<8>()) ^ Secret[0],
                        read(Data.subspan<8>().first<8>()) ^ Seed);
        See1 = rapidMix(read(Data.subspan<16>().first<8>()) ^ Secret[1],
                        read(Data.subspan<24>().first<8>()) ^ See1);
        See2 = rapidMix(read(Data.subspan<32>().first<8>()) ^ Secret[2],
                        read(Data.subspan<40>().first<8>()) ^ See2);
        Data = Data.subspan<48>();
      } while (Data.size() > 48);
      Seed ^= See1;
      Seed ^= See2;
    }
    if (Data.size() > 16) {
      Seed = rapidMix(read(Data.first<8>()) ^ Secret[2],
                      read(Data.subspan<8>().first<8>()) ^ Seed);
      if (Data.size() > 32) {
        Seed = rapidMix(read(Data.subspan<16>().first<8>()) ^ Secret[2],
                        read(Data.subspan<24>().first<8>()) ^ Seed);
      }
    }
    A = read(Data.last<16>().first<8>()) ^ Data.size();
    B = read(Data.last<8>());
  }
  A ^= Secret[1];
  B ^= Seed;
  rapidMum(A, B);
  return rapidMix(A ^ Secret[3], B ^ Secret[1] ^ Data.size());
}

} // namespace WasmEdge::Hash
