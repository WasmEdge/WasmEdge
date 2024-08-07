// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/thread/mt19937.c - MT19937 prng for testing ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains a pseudo random generator for testing purpose.
///
/// clang-13 --target=wasm32-wasi --sysroot=[[wasm sysroot]] -fno-exceptions
/// -fno-rounding-math -ftls-model=local-exec -mmutable-globals
/// -mnontrapping-fptoint -msign-ext -msimd128 -fuse-ld=/usr/bin/wasm-ld-13 -O3
/// -nodefaultlibs -nostartfiles -Wl,--entry=mt19937,--stack-first mt19937.c -o
/// mt19937.wasm
///
/// wasm-opt -all mt19937.wasm -O4 --low-memory-unused --strip-producers
/// --strip-target-features -o mt19937-opt.wasm
///
//===----------------------------------------------------------------------===//

#include <limits.h>
#include <stdint.h>

enum {
  kWordSize = 64,
  kStateSize = 312,
  kShiftSize = 156,
  kMaskBits = 31,
  kXorMask = UINT64_C(0xb5026f5aa96619e9),
  kTemperingU = 29,
  kTemperingD = UINT64_C(0x5555555555555555),
  kTemperingS = 17,
  kTemperingB = UINT64_C(0x71d67fffeda60000),
  kTemperingT = 37,
  kTemperingC = UINT64_C(0xfff7eee000000000),
  kTemperingL = 43,
  kInitializationMultiplier = UINT64_C(6364136223846793005),
  kDefaultSeed = UINT64_C(5489),
};

typedef struct mersenne_twister_state {
  uint64_t State[kStateSize];
  uint64_t Pointer;
} mersenne_twister_state;

_Static_assert(sizeof(mersenne_twister_state) == 2504);

static void seed(mersenne_twister_state *state, uint64_t value) {
  state->State[0] = value;

  for (uint64_t index = 1; index < kStateSize; ++index) {
    uint64_t x = state->State[index - 1];
    x ^= x >> (kWordSize - 2);
    x *= kInitializationMultiplier;
    x += index;
    state->State[index] = x;
  }
  state->Pointer = kStateSize;
}

static void gen_rand(mersenne_twister_state *state) {
  const uint64_t upper_mask = (~UINT64_C(0)) << kMaskBits;
  const uint64_t lower_mask = ~upper_mask;

  for (uint64_t k = 0; k < (kStateSize - kShiftSize); ++k) {
    uint64_t y =
        ((state->State[k] & upper_mask) | (state->State[k + 1] & lower_mask));
    state->State[k] =
        (state->State[k + kShiftSize] ^ (y >> 1) ^ ((y & 0x01) ? kXorMask : 0));
  }

  for (uint64_t k = (kStateSize - kShiftSize); k < (kStateSize - 1); ++k) {
    uint64_t y =
        ((state->State[k] & upper_mask) | (state->State[k + 1] & lower_mask));
    state->State[k] = (state->State[k + (kShiftSize - kStateSize)] ^ (y >> 1) ^
                       ((y & 0x01) ? kXorMask : 0));
  }

  uint64_t y = ((state->State[kStateSize - 1] & upper_mask) |
                (state->State[0] & lower_mask));
  state->State[kStateSize - 1] =
      (state->State[kShiftSize - 1] ^ (y >> 1) ^ ((y & 0x01) ? kXorMask : 0));
  state->Pointer = 0;
}

static uint64_t generate(mersenne_twister_state *state) {
  if (state->Pointer >= kStateSize) {
    gen_rand(state);
  }

  uint64_t result = state->State[state->Pointer++];
  result ^= (result >> kTemperingU) & kTemperingD;
  result ^= (result << kTemperingS) & kTemperingB;
  result ^= (result << kTemperingT) & kTemperingC;
  result ^= (result >> kTemperingL);

  return result;
}

static void discard(mersenne_twister_state *state, uint64_t z) {
  while (z > kStateSize - state->Pointer) {
    z -= kStateSize - state->Pointer;
    gen_rand(state);
  }
  state->Pointer += z;
}

uint64_t mt19937(mersenne_twister_state *state, uint64_t seed_value,
                 uint64_t index) {
  seed(state, seed_value);
  discard(state, index);
  return generate(state);
}
