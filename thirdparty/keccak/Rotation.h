// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include <cstdint>

// As we're not using assembly, we can't use the native rotation instructions
// replace it with a small inline
static inline uint64_t rotateLeft(uint64_t x, int n) {
  const unsigned int mask =
      (8 * sizeof(x) - 1); // assumes width is a power of 2.

  // assert ( (c<=mask) &&"rotate by type width or more");
  n &= mask;
  return (x << n) | (x >> ((-n) & mask));
}

static inline uint64_t rotateRight(uint64_t x, int n) {
  const unsigned int mask =
      (8 * sizeof(x) - 1); // assumes width is a power of 2.

  // assert ( (c<=mask) &&"rotate by type width or more");
  n &= mask;
  return (x >> n) | (x << ((-n) & mask));
}
