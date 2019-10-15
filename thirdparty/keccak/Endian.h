// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include <cstdint>

// Endianness conversion inlines - These will be optimised out on platforms
// where it's not necessary, and will collapse to only the swapping code on
// other big endian platforms.

inline bool littleEndian() {
  uint16_t tn;
  uint8_t test;

  tn = 0xff00;
  test = static_cast<uint8_t>(tn);
  if (!test)
    return true;
  else
    return false;
}

inline uint16_t swapEndian(uint16_t in) {
  uint8_t b[2];
  uint16_t out[2];

  b[0] = static_cast<uint8_t>(in >> 8);
  b[1] = static_cast<uint8_t>(in);

  out[0] = ((b[1]) & 0x00FF);
  out[1] = ((b[0]) & 0x00FF);
  out[0] <<= 8;

  return (out[0] | out[1]);
}

inline uint32_t swapEndian(uint32_t in) {
  uint16_t b[2];
  uint32_t out[2];

  b[0] = static_cast<uint16_t>(in >> 16);
  b[1] = static_cast<uint16_t>(in);
  b[0] = swapEndian(b[0]);
  b[1] = swapEndian(b[1]);

  out[0] = ((b[1]) & 0x0000FFFF);
  out[1] = ((b[0]) & 0x0000FFFF);
  out[0] <<= 16;

  return (out[0] | out[1]);
}

inline uint64_t swapEndian(uint64_t in) {
  uint32_t b[2];
  uint64_t out[2];

  b[0] = static_cast<uint32_t>(in >> 32);
  b[1] = static_cast<uint32_t>(in);
  b[0] = swapEndian(b[0]);
  b[1] = swapEndian(b[1]);

  out[0] = ((b[1]) & 0x00000000FFFFFFFF);
  out[1] = ((b[0]) & 0x00000000FFFFFFFF);
  out[0] <<= 32;

  return (out[0] | out[1]);
}

inline uint16_t NativeToLittle(uint16_t in) {
  if (littleEndian()) {
    return in;
  } else {
    return swapEndian(in);
  }
}

inline uint32_t NativeToLittle(uint32_t in) {
  if (littleEndian())
    return in;
  else {
    return swapEndian(in);
  }
}

inline uint64_t NativeToLittle(uint64_t in) {
  if (littleEndian())
    return in;
  else {
    return swapEndian(in);
  }
}

inline uint16_t LittleToNative(uint16_t in) {
  if (littleEndian())
    return in;
  else {
    return swapEndian(in);
  }
}

inline uint32_t LittleToNative(uint32_t in) {
  if (littleEndian())
    return in;
  else {
    return swapEndian(in);
  }
}

inline uint64_t LittleToNative(uint64_t in) {
  if (littleEndian())
    return in;
  else {
    return swapEndian(in);
  }
}

inline uint16_t NativeToBig(uint16_t in) {
  if (littleEndian()) {
    return swapEndian(in);
  } else {
    return in;
  }
}

inline uint32_t NativeToBig(uint32_t in) {
  if (littleEndian()) {
    return swapEndian(in);
  } else {
    return in;
  }
}

inline uint64_t NativeToBig(uint64_t in) {
  if (littleEndian()) {
    return swapEndian(in);
  } else {
    return in;
  }
}

inline uint16_t BigToNative(uint16_t in) {
  if (littleEndian()) {
    return swapEndian(in);
  } else {
    return in;
  }
}

inline uint32_t BigToNative(uint32_t in) {
  if (littleEndian()) {
    return swapEndian(in);
  } else {
    return in;
  }
}

inline uint64_t BigToNative(uint64_t in) {
  if (littleEndian()) {
    return swapEndian(in);
  } else {
    return in;
  }
}
